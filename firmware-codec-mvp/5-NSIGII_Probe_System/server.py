# ============================================================================
# NSIGII FLASK SERVER - Command and Control Interface
# Trident Topology: Transmitter | Receiver | Verifier
# ============================================================================

from flask import Flask, jsonify, render_template, request
from flask_cors import CORS
import time
import random
from probe import (
    NSIGIICommandControl, 
    ProbeOperator, 
    NeedType, 
    LMACAddress,
    BipartiteState,
    compute_discriminant,
    cisco_execute,
    rotate_rational_wheel,
    NSIGII_VERSION
)

app = Flask(__name__, 
    template_folder='templates',
    static_folder='static'
)
CORS(app)

# Global system instance
nsigii_system = NSIGIICommandControl()
probe_operator = ProbeOperator()
system_start_time = time.time()

# ============================================================================
# ROUTES - HTML PAGES
# ============================================================================

@app.route("/")
def index():
    """Main dashboard with NSIGII framework documentation"""
    return render_template("index.html")

# ============================================================================
# API ENDPOINTS - PROBE SYSTEM
# ============================================================================

@app.route("/api/probe", methods=["GET", "POST"])
def get_probe_data():
    """
    Run system probe and return measurements
    
    Probe operator: P: S → M
    Extracts measurable structure from system state
    """
    need_type = None
    if request.method == "POST":
        data = request.get_json()
        need_str = data.get("need", "")
        if need_str in ["food", "water", "shelter"]:
            need_type = NeedType(need_str)
    
    probe_operator.randomize_state(need_type)
    probe_result = probe_operator.probe()
    
    return jsonify({
        "status": "success",
        "probe": probe_result,
        "timestamp": time.time(),
        "nsigii_version": NSIGII_VERSION
    })

@app.route("/api/probe/continuous", methods=["GET"])
def continuous_probe():
    """Get continuous probe history"""
    return jsonify({
        "status": "success",
        "history": probe_operator.probe_history[-10:],  # Last 10 probes
        "count": len(probe_operator.probe_history)
    })

# ============================================================================
# API ENDPOINTS - LMAC (LOOPBACK MAC)
# ============================================================================

@app.route("/api/lmac", methods=["GET"])
def get_lmac():
    """
    Get Loopback MAC Address status
    
    LMAC provides real-time verification of system location
    Inverse relay system for "here and now" protocol
    """
    lmac = probe_operator.lmac
    return jsonify({
        "status": "success",
        "lmac": {
            "physical_mac": lmac.physical_mac,
            "loopback_mac": lmac.loopback_mac,
            "ipv4": lmac.ipv4,
            "ipv6": lmac.ipv6,
            "geospatial": {
                "latitude": lmac.geospatial[0],
                "longitude": lmac.geospatial[1]
            },
            "timestamp": lmac.timestamp,
            "age_seconds": time.time() - lmac.timestamp
        }
    })

@app.route("/api/lmac/update", methods=["POST"])
def update_lmac_location():
    """Update LMAC geospatial location"""
    data = request.get_json()
    lat = data.get("latitude", 0.0)
    lon = data.get("longitude", 0.0)
    
    # Create new LMAC with updated location
    new_lmac = LMACAddress(
        ipv4=probe_operator.lmac.ipv4,
        geospatial=(lat, lon),
        timestamp=time.time()
    )
    probe_operator.lmac = new_lmac
    
    return jsonify({
        "status": "success",
        "message": "LMAC location updated",
        "new_loopback_mac": new_lmac.loopback_mac,
        "geospatial": {"latitude": lat, "longitude": lon}
    })

# ============================================================================
# API ENDPOINTS - TRIDENT CONSENSUS
# ============================================================================

@app.route("/api/trident/vote", methods=["POST"])
def trident_vote():
    """
    Compute trident consensus from three votes
    
    Trident rule: 2/3 majority required for consensus
    Yes + Yes + Maybe = Yes (better safe than sorry)
    """
    data = request.get_json()
    votes = data.get("votes", ["maybe", "maybe", "maybe"])
    
    if len(votes) != 3:
        return jsonify({
            "status": "error",
            "message": "Exactly 3 votes required for trident consensus"
        }), 400
    
    consensus, confidence = probe_operator.compute_trident_consensus(votes)
    
    return jsonify({
        "status": "success",
        "trident_result": {
            "consensus": consensus,
            "confidence": confidence,
            "votes": votes,
            "threshold": 0.67,
            "threshold_met": confidence >= 0.67,
            "interpretation": _interpret_consensus(consensus)
        }
    })

def _interpret_consensus(consensus: str) -> str:
    """Human-readable interpretation of consensus"""
    interpretations = {
        "YES": "Request approved - 2/3 majority achieved",
        "NO": "Request denied - majority rejection",
        "MAYBE": "Superposition state - requires more information",
        "NO_CONSENSUS": "No clear consensus - system deadlock"
    }
    return interpretations.get(consensus, "Unknown consensus state")

# ============================================================================
# API ENDPOINTS - NEED REQUESTS (FOOD/WATER/SHELTER)
# ============================================================================

@app.route("/api/need/request", methods=["POST"])
def request_need():
    """
    Process a need request through trident topology
    
    Phase 1: Transmitter encodes
    Phase 2: Receiver decodes
    Phase 3: Verifier validates
    """
    data = request.get_json()
    need_type_str = data.get("need", "food")
    location = data.get("location", {"lat": 0.0, "lon": 0.0, "address": "Unknown"})
    
    try:
        need_type = NeedType(need_type_str)
    except ValueError:
        return jsonify({
            "status": "error",
            "message": f"Invalid need type: {need_type_str}. Use: food, water, shelter"
        }), 400
    
    # Process through trident
    result = nsigii_system.process_need_request(need_type, location)
    
    # Add human-readable message
    if result["status"] == "SUCCESS":
        result["message"] = f"{need_type_str.upper()} request verified and approved via trident consensus"
        result["delivery_status"] = "PENDING_FULFILLMENT"
    else:
        result["message"] = f"{need_type_str.upper()} request failed verification: {result.get('error', 'Unknown error')}"
    
    return jsonify(result)

@app.route("/api/need/status", methods=["GET"])
def get_need_status():
    """Get status of all need requests"""
    return jsonify({
        "status": "success",
        "verification_count": nsigii_system.verification_counter,
        "failure_count": len(nsigii_system.failure_log),
        "system_uptime": time.time() - system_start_time,
        "consensus_log": nsigii_system.verifier.consensus_log[-5:]  # Last 5
    })

# ============================================================================
# API ENDPOINTS - BIPARTITE STATE & DISCRIMINANTS
# ============================================================================

@app.route("/api/bipartite", methods=["GET", "POST"])
def bipartite_state():
    """
    Get or update bipartite state
    
    Bipartite relationships track power distribution between parties
    Discriminant Δ = coherence² - 4αβ detects system degradation
    """
    if request.method == "POST":
        data = request.get_json()
        alpha = data.get("power_alpha", 0.5)
        beta = data.get("power_beta", 0.5)
        
        bipartite = BipartiteState(power_alpha=alpha, power_beta=beta)
        
        return jsonify({
            "status": "success",
            "bipartite": {
                "power_alpha": bipartite.power_alpha,
                "power_beta": bipartite.power_beta,
                "coherence": bipartite.coherence,
                "discriminant": bipartite.discriminant,
                "bipartite_intact": bipartite.is_bipartite_intact(),
                "consensus_state": bipartite.get_consensus_state(),
                "interpretation": _interpret_discriminant(bipartite.discriminant)
            }
        })
    
    # GET - return current state
    current = probe_operator.bipartite
    return jsonify({
        "status": "success",
        "bipartite": {
            "power_alpha": current.power_alpha,
            "power_beta": current.power_beta,
            "coherence": current.coherence,
            "discriminant": current.discriminant,
            "bipartite_intact": current.is_bipartite_intact(),
            "consensus_state": current.get_consensus_state()
        }
    })

def _interpret_discriminant(delta: float) -> str:
    """Interpret discriminant value"""
    if delta > 0:
        return "Δ > 0: Two real solutions - System is stable and separable"
    elif delta == 0:
        return "Δ = 0: Critical point - System at threshold"
    else:
        return "Δ < 0: Complex solutions - System entangled, constitutional violation detected"

@app.route("/api/discriminant/calculate", methods=["POST"])
def calculate_discriminant():
    """Calculate discriminant from quadratic coefficients"""
    data = request.get_json()
    a = data.get("a", 1.0)
    b = data.get("b", 0.0)
    c = data.get("c", 0.0)
    
    delta = compute_discriminant(a, b, c)
    
    return jsonify({
        "status": "success",
        "discriminant": delta,
        "coefficients": {"a": a, "b": b, "c": c},
        "interpretation": _interpret_discriminant(delta),
        "roots": _compute_roots(a, b, delta)
    })

def _compute_roots(a: float, b: float, delta: float) -> dict:
    """Compute roots based on discriminant"""
    if delta < 0:
        real_part = -b / (2*a)
        imag_part = abs(delta)**0.5 / (2*a)
        return {
            "type": "complex_conjugate",
            "root1": f"{real_part} + {imag_part}i",
            "root2": f"{real_part} - {imag_part}i"
        }
    elif delta == 0:
        root = -b / (2*a)
        return {
            "type": "real_repeated",
            "root": root
        }
    else:
        root1 = (-b + delta**0.5) / (2*a)
        root2 = (-b - delta**0.5) / (2*a)
        return {
            "type": "real_distinct",
            "root1": root1,
            "root2": root2
        }

# ============================================================================
# API ENDPOINTS - CISCO RISC EXECUTION
# ============================================================================

@app.route("/api/cisco/execute", methods=["POST"])
def cisco_execute_endpoint():
    """
    Execute instruction sequence using Cisco RISC model
    
    Cisco: Command Instruction Sequence Control Operation
    Bottom-up execution (left-bit encoding)
    """
    data = request.get_json()
    instructions = data.get("instructions", [1, 0, 1, 1, 0])
    
    result = cisco_execute(instructions)
    
    return jsonify({
        "status": "success",
        "cisco_execution": result,
        "model": "CISCO_RISC",
        "direction": "BOTTOM_UP"
    })

# ============================================================================
# API ENDPOINTS - RATIONAL WHEEL
# ============================================================================

@app.route("/api/wheel/rotate", methods=["POST"])
def rotate_wheel():
    """Rotate the Rational Wheel"""
    data = request.get_json()
    current = data.get("current_position", 0)
    degrees = data.get("degrees", 1)
    
    new_position = rotate_rational_wheel(current, degrees)
    
    # Calculate channel positions
    channel_positions = {
        "transmitter": (new_position) % 360,  # 0 degrees
        "receiver": (new_position + 120) % 360,  # 120 degrees
        "verifier": (new_position + 240) % 360   # 240 degrees
    }
    
    return jsonify({
        "status": "success",
        "wheel": {
            "previous_position": current,
            "new_position": new_position,
            "degrees_rotated": degrees,
            "channel_positions": channel_positions,
            "radians": new_position * 3.14159 / 180
        }
    })

# ============================================================================
# API ENDPOINTS - SYSTEM STATUS
# ============================================================================

@app.route("/api/system/status", methods=["GET"])
def system_status():
    """Get complete system status"""
    return jsonify({
        "status": "success",
        "nsigii": {
            "version": NSIGII_VERSION,
            "topology": "Trident (Transmitter | Receiver | Verifier)",
            "channels": 3,
            "consensus_threshold": 0.67
        },
        "system": {
            "uptime_seconds": time.time() - system_start_time,
            "verification_count": nsigii_system.verification_counter,
            "failure_count": len(nsigii_system.failure_log),
            "probe_count": len(probe_operator.probe_history)
        },
        "protocols": {
            "here_and_now": "ACTIVE",
            "lmac_verification": "ACTIVE",
            "bipartite_consensus": "ACTIVE",
            "cisco_risc": "ACTIVE"
        },
        "human_rights": {
            "status": "VERIFIED",
            "tag": "NSIGII_HR_ENABLED",
            "needs_supported": ["food", "water", "shelter"]
        }
    })

@app.route("/api/system/channels", methods=["GET"])
def get_channels():
    """Get trident channel information"""
    return jsonify({
        "status": "success",
        "channels": [
            {
                "id": 0,
                "name": "Transmitter",
                "type": "CHANNEL_TRANSMITTER",
                "loopback": "127.0.0.1",
                "codec_ratio": "1/3",
                "rwx": "WRITE",
                "wheel_position": "0°"
            },
            {
                "id": 1,
                "name": "Receiver",
                "type": "CHANNEL_RECEIVER",
                "loopback": "127.0.0.2",
                "codec_ratio": "2/3",
                "rwx": "READ",
                "wheel_position": "120°"
            },
            {
                "id": 2,
                "name": "Verifier",
                "type": "CHANNEL_VERIFIER",
                "loopback": "127.0.0.3",
                "codec_ratio": "3/3",
                "rwx": "EXECUTE",
                "wheel_position": "240°"
            }
        ]
    })

# ============================================================================
# ERROR HANDLERS
# ============================================================================

@app.errorhandler(404)
def not_found(error):
    return jsonify({
        "status": "error",
        "message": "Endpoint not found",
        "available_endpoints": [
            "/api/probe",
            "/api/lmac",
            "/api/trident/vote",
            "/api/need/request",
            "/api/bipartite",
            "/api/system/status"
        ]
    }), 404

@app.errorhandler(500)
def internal_error(error):
    return jsonify({
        "status": "error",
        "message": "Internal server error",
        "error_type": "SYSTEM_FAILURE"
    }), 500

# ============================================================================
# MAIN ENTRY POINT
# ============================================================================

if __name__ == "__main__":
    print("=" * 60)
    print("NSIGII COMMAND AND CONTROL SERVER")
    print(f"Version: {NSIGII_VERSION}")
    print("=" * 60)
    print("\nStarting Flask server...")
    print("Available endpoints:")
    print("  - GET  /                    : Dashboard")
    print("  - GET  /api/system/status   : System status")
    print("  - GET  /api/probe           : Run system probe")
    print("  - GET  /api/lmac            : Get LMAC status")
    print("  - POST /api/trident/vote    : Submit trident votes")
    print("  - POST /api/need/request    : Request food/water/shelter")
    print("  - POST /api/bipartite       : Update bipartite state")
    print("\n" + "=" * 60)
    
    app.run(host='0.0.0.0', port=5000, debug=True)
