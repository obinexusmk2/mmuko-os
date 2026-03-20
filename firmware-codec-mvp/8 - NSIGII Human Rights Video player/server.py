"""
RIFT Player — Python API Server
Bridges the Symbol Analytic Engine to the HTML5 player via REST + SSE
"""

import json
import time
from flask import Flask, request, jsonify, Response, send_from_directory
from engine.symbol_engine import SymbolInterpreter, TriState, DiscriminantState

app = Flask(__name__, static_folder="static", template_folder="templates")
interpreter = SymbolInterpreter()


@app.route("/")
def index():
    return send_from_directory("templates", "index.html")


@app.route("/api/interpret", methods=["POST"])
def interpret():
    """
    POST /api/interpret
    Body: { "text": "...", "frame_index": 0 }
    Returns: full interpretation result with symbol table
    """
    data = request.get_json(force=True)
    text = data.get("text", "")
    if not text.strip():
        return jsonify({"error": "empty text"}), 400

    result = interpreter.interpret(text)

    return jsonify({
        "frame_index": result.frame_index,
        "dominant_state": result.dominant_state.value,
        "tristate_summary": result.tristate_summary,
        "semantic_label": result.semantic_label,
        "caption": result.accessibility_caption,
        "confidence": round(result.confidence_score, 4),
        "symbol_table": interpreter.get_symbol_table_json(),
        "vtt_cue": result.to_vtt_cue(
            result.frame_index * 3000,
            result.frame_index * 3000 + 3000
        ),
        "color": {
            DiscriminantState.ORDER.value:     "#00ff88",
            DiscriminantState.CONSENSUS.value: "#4488ff",
            DiscriminantState.CHAOS.value:     "#ff4444",
        }.get(result.dominant_state.value, "#ffffff"),
    })


@app.route("/api/transcript", methods=["GET"])
def transcript():
    """Export full WebVTT transcript"""
    vtt = interpreter.export_vtt()
    return Response(vtt, mimetype="text/vtt",
                    headers={"Content-Disposition": "attachment; filename=rift_transcript.vtt"})


@app.route("/api/symbol_table", methods=["GET"])
def symbol_table():
    """Current symbol table state"""
    return jsonify(interpreter.get_symbol_table_json())


@app.route("/api/reset", methods=["POST"])
def reset():
    """Reset interpreter state"""
    global interpreter
    interpreter = SymbolInterpreter()
    return jsonify({"status": "reset"})


@app.route("/api/status", methods=["GET"])
def status():
    return jsonify({
        "engine": "OBINexus RIFT Symbol Analytic Engine",
        "version": "1.0.0",
        "frames_processed": interpreter.frame_count,
        "symbols_indexed": len(interpreter.trident.symbol_table),
    })


if __name__ == "__main__":
    print("RIFT Symbol Engine — OBINexus Computing")
    print("Tripartite Tristate: YES(1) / NO(0) / MAYBE(-∞→1)")
    print("Server: http://localhost:5000")
    app.run(debug=False, host="0.0.0.0", port=5000)
