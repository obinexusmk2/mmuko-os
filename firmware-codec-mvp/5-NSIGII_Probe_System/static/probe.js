// ============================================================================
// NSIGII PROBE SYSTEM - Frontend JavaScript
// Trident Topology: Transmitter | Receiver | Verifier
// ============================================================================

// Global state
const NSIGII_STATE = {
    version: "7.0.0",
    probeHistory: [],
    lmac: null,
    bipartite: null,
    verificationCount: 0,
    wheelPosition: 0,
    channels: {
        transmitter: { active: true, position: 0 },
        receiver: { active: true, position: 120 },
        verifier: { active: true, position: 240 }
    }
};

// ============================================================================
// INITIALIZATION
// ============================================================================

document.addEventListener('DOMContentLoaded', function() {
    console.log('NSIGII System Initializing...');
    
    // Initialize system
    initializeSystem();
    
    // Setup periodic updates
    setInterval(updateSystemStatus, 5000);
    setInterval(rotateRationalWheel, 1000);
    
    // Initial probe
    setTimeout(runProbe, 500);
    
    console.log('NSIGII System Ready');
});

async function initializeSystem() {
    try {
        // Get system status
        const response = await fetch('/api/system/status');
        const data = await response.json();
        
        if (data.status === 'success') {
            NSIGII_STATE.verificationCount = data.system.verification_count;
            logMessage('System', `NSIGII v${data.nsigii.version} initialized`);
        }
        
        // Get LMAC status
        await updateLMACStatus();
        
        // Get bipartite state
        await updateBipartiteState();
        
    } catch (error) {
        console.error('Initialization error:', error);
        logMessage('Error', 'Failed to initialize system');
    }
}

// ============================================================================
// PROBE OPERATIONS
// ============================================================================

async function runProbe(needType = null) {
    try {
        const options = {
            method: needType ? 'POST' : 'GET',
            headers: { 'Content-Type': 'application/json' }
        };
        
        if (needType) {
            options.body = JSON.stringify({ need: needType });
        }
        
        const response = await fetch('/api/probe', options);
        const data = await response.json();
        
        if (data.status === 'success') {
            const probe = data.probe;
            NSIGII_STATE.probeHistory.push(probe);
            
            // Update display
            updateProbeDisplay(probe);
            
            // Update canvas
            drawProbeVisualization(probe);
            
            // Update bipartite status
            updateBipartiteFromProbe(probe);
            
            logMessage('Probe', `Probe #${probe.sequence} complete. Discriminant: ${probe.discriminant.toFixed(4)}`);
            
            return probe;
        }
    } catch (error) {
        console.error('Probe error:', error);
        logMessage('Error', 'Probe failed');
    }
}

function updateProbeDisplay(probe) {
    // Update metrics
    document.getElementById('magnitude').textContent = probe.magnitude.toFixed(2);
    document.getElementById('angle').textContent = (probe.angle * 180 / Math.PI).toFixed(2) + '°';
    document.getElementById('discriminant').textContent = probe.discriminant.toFixed(4);
    
    // Color-code discriminant
    const discElement = document.getElementById('discriminant');
    if (probe.discriminant > 0) {
        discElement.className = 'value positive';
    } else if (probe.discriminant === 0) {
        discElement.className = 'value neutral';
    } else {
        discElement.className = 'value negative';
    }
}

function drawProbeVisualization(probe) {
    const canvas = document.getElementById('probeCanvas');
    if (!canvas) return;
    
    const ctx = canvas.getContext('2d');
    const centerX = canvas.width / 2;
    const centerY = canvas.height / 2;
    const scale = 8; // Scale factor for visualization
    
    // Clear canvas
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    
    // Draw coordinate axes
    ctx.strokeStyle = '#333';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(centerX, 0);
    ctx.lineTo(centerX, canvas.height);
    ctx.moveTo(0, centerY);
    ctx.lineTo(canvas.width, centerY);
    ctx.stroke();
    
    // Draw quadrants
    ctx.fillStyle = 'rgba(0, 255, 0, 0.1)';
    ctx.fillRect(centerX, 0, centerX, centerY); // Quadrant I
    ctx.fillStyle = 'rgba(255, 0, 0, 0.1)';
    ctx.fillRect(0, 0, centerX, centerY); // Quadrant II
    ctx.fillStyle = 'rgba(255, 0, 255, 0.1)';
    ctx.fillRect(0, centerY, centerX, centerY); // Quadrant III
    ctx.fillStyle = 'rgba(0, 0, 255, 0.1)';
    ctx.fillRect(centerX, centerY, centerX, centerY); // Quadrant IV
    
    // Draw unit circle
    ctx.strokeStyle = '#666';
    ctx.setLineDash([2, 2]);
    ctx.beginPath();
    ctx.arc(centerX, centerY, 10 * scale, 0, 2 * Math.PI);
    ctx.stroke();
    ctx.setLineDash([]);
    
    // Draw probe point
    const px = centerX + probe.x * scale;
    const py = centerY - probe.y * scale; // Invert Y for canvas
    
    // Draw line from center to point
    ctx.strokeStyle = probe.bipartite_intact ? '#00ff00' : '#ff0000';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(centerX, centerY);
    ctx.lineTo(px, py);
    ctx.stroke();
    
    // Draw point
    ctx.fillStyle = probe.bipartite_intact ? '#00ff00' : '#ff0000';
    ctx.beginPath();
    ctx.arc(px, py, 6, 0, 2 * Math.PI);
    ctx.fill();
    
    // Draw magnitude circle
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.3)';
    ctx.beginPath();
    ctx.arc(centerX, centerY, probe.magnitude * scale, 0, 2 * Math.PI);
    ctx.stroke();
    
    // Labels
    ctx.fillStyle = '#fff';
    ctx.font = '10px monospace';
    ctx.fillText('I', canvas.width - 15, 15);
    ctx.fillText('II', 5, 15);
    ctx.fillText('III', 5, canvas.height - 5);
    ctx.fillText('IV', canvas.width - 20, canvas.height - 5);
}

// ============================================================================
// LMAC OPERATIONS
// ============================================================================

async function updateLMACStatus() {
    try {
        const response = await fetch('/api/lmac');
        const data = await response.json();
        
        if (data.status === 'success') {
            NSIGII_STATE.lmac = data.lmac;
            
            document.getElementById('physicalMac').textContent = data.lmac.physical_mac;
            document.getElementById('loopbackMac').textContent = data.lmac.loopback_mac;
            document.getElementById('geospatial').textContent = 
                `${data.lmac.geospatial.latitude.toFixed(4)}, ${data.lmac.geospatial.longitude.toFixed(4)}`;
        }
    } catch (error) {
        console.error('LMAC update error:', error);
    }
}

async function updateLocation() {
    if (!navigator.geolocation) {
        logMessage('Error', 'Geolocation not supported');
        return;
    }
    
    navigator.geolocation.getCurrentPosition(
        async (position) => {
            try {
                const response = await fetch('/api/lmac/update', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({
                        latitude: position.coords.latitude,
                        longitude: position.coords.longitude
                    })
                });
                
                const data = await response.json();
                
                if (data.status === 'success') {
                    logMessage('LMAC', `Location updated: ${position.coords.latitude.toFixed(4)}, ${position.coords.longitude.toFixed(4)}`);
                    await updateLMACStatus();
                    await runProbe();
                }
            } catch (error) {
                console.error('Location update error:', error);
                logMessage('Error', 'Failed to update location');
            }
        },
        (error) => {
            logMessage('Error', `Geolocation error: ${error.message}`);
        }
    );
}

// ============================================================================
// BIPARTITE STATE
// ============================================================================

async function updateBipartiteState() {
    try {
        const response = await fetch('/api/bipartite');
        const data = await response.json();
        
        if (data.status === 'success') {
            NSIGII_STATE.bipartite = data.bipartite;
            updateBipartiteDisplay(data.bipartite);
        }
    } catch (error) {
        console.error('Bipartite update error:', error);
    }
}

function updateBipartiteFromProbe(probe) {
    const statusElement = document.getElementById('bipartiteStatus');
    
    if (probe.bipartite_intact) {
        statusElement.textContent = 'INTACT';
        statusElement.className = 'value status-active';
    } else {
        statusElement.textContent = 'VIOLATION';
        statusElement.className = 'value status-error';
    }
}

function updateBipartiteDisplay(bipartite) {
    const statusElement = document.getElementById('bipartiteStatus');
    
    if (bipartite.bipartite_intact) {
        statusElement.textContent = 'INTACT';
        statusElement.className = 'value status-active';
    } else {
        statusElement.textContent = 'VIOLATION';
        statusElement.className = 'value status-error';
    }
}

// ============================================================================
// NEED REQUESTS
// ============================================================================

async function requestNeed(needType) {
    const statusElement = document.getElementById(`${needType}Status`);
    statusElement.textContent = 'Processing...';
    statusElement.className = 'need-status processing';
    
    // Run probe first
    await runProbe(needType);
    
    // Get current location
    let location = { lat: 0.0, lon: 0.0, address: 'Unknown' };
    
    if (NSIGII_STATE.lmac) {
        location.lat = NSIGII_STATE.lmac.geospatial.latitude;
        location.lon = NSIGII_STATE.lmac.geospatial.longitude;
    }
    
    try {
        const response = await fetch('/api/need/request', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                need: needType,
                location: location
            })
        });
        
        const data = await response.json();
        
        if (data.status === 'SUCCESS') {
            statusElement.textContent = 'Verified ✓';
            statusElement.className = 'need-status verified';
            
            logMessage('Request', `${needType.toUpperCase()} request verified via trident consensus`);
            
            // Show discriminant info
            if (data.discriminant !== undefined) {
                logMessage('Discriminant', `Δ = ${data.discriminant.toFixed(4)} - ${data.bipartite_intact ? 'System stable' : 'Constitutional violation detected'}`);
            }
        } else {
            statusElement.textContent = 'Failed ✗';
            statusElement.className = 'need-status failed';
            logMessage('Error', `${needType.toUpperCase()} request failed: ${data.error}`);
        }
        
        // Reset status after delay
        setTimeout(() => {
            statusElement.textContent = 'Ready';
            statusElement.className = 'need-status';
        }, 5000);
        
    } catch (error) {
        console.error('Request error:', error);
        statusElement.textContent = 'Error';
        statusElement.className = 'need-status failed';
        logMessage('Error', `Failed to process ${needType} request`);
    }
}

// ============================================================================
// TRIDENT VOTING
// ============================================================================

async function submitVote() {
    const vote1 = document.getElementById('vote1').value;
    const vote2 = document.getElementById('vote2').value;
    const vote3 = document.getElementById('vote3').value;
    
    const votes = [vote1, vote2, vote3];
    
    try {
        const response = await fetch('/api/trident/vote', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ votes: votes })
        });
        
        const data = await response.json();
        
        if (data.status === 'success') {
            const result = data.trident_result;
            const resultElement = document.getElementById('voteResult');
            
            let resultClass = 'pending';
            let resultIcon = '?';
            
            if (result.consensus === 'YES') {
                resultClass = 'approved';
                resultIcon = '✓';
            } else if (result.consensus === 'NO') {
                resultClass = 'rejected';
                resultIcon = '✗';
            } else if (result.consensus === 'MAYBE') {
                resultClass = 'uncertain';
                resultIcon = '?';
            }
            
            resultElement.innerHTML = `
                <div class="result-box ${resultClass}">
                    <div class="result-icon">${resultIcon}</div>
                    <div class="result-text">${result.consensus}</div>
                    <div class="result-confidence">Confidence: ${(result.confidence * 100).toFixed(1)}%</div>
                    <div class="result-threshold">${result.threshold_met ? 'Threshold met' : 'Threshold not met'}</div>
                    <div class="result-interpretation">${result.interpretation}</div>
                </div>
            `;
            
            logMessage('Vote', `Trident consensus: ${result.consensus} (${(result.confidence * 100).toFixed(1)}%)`);
        }
    } catch (error) {
        console.error('Vote error:', error);
        logMessage('Error', 'Failed to submit vote');
    }
}

// ============================================================================
// RATIONAL WHEEL
// ============================================================================

async function rotateRationalWheel() {
    NSIGII_STATE.wheelPosition = (NSIGII_STATE.wheelPosition + 1) % 360;
    
    // Update channel positions
    NSIGII_STATE.channels.transmitter.position = NSIGII_STATE.wheelPosition;
    NSIGII_STATE.channels.receiver.position = (NSIGII_STATE.wheelPosition + 120) % 360;
    NSIGII_STATE.channels.verifier.position = (NSIGII_STATE.wheelPosition + 240) % 360;
}

// ============================================================================
// SYSTEM STATUS
// ============================================================================

async function updateSystemStatus() {
    try {
        const response = await fetch('/api/system/status');
        const data = await response.json();
        
        if (data.status === 'success') {
            NSIGII_STATE.verificationCount = data.system.verification_count;
        }
    } catch (error) {
        console.error('Status update error:', error);
    }
}

// ============================================================================
// LOGGING
// ============================================================================

function logMessage(source, message) {
    const logContainer = document.getElementById('requestLog');
    const timestamp = new Date().toLocaleTimeString();
    
    const entry = document.createElement('div');
    entry.className = `log-entry ${source.toLowerCase()}`;
    entry.innerHTML = `
        <span class="timestamp">${timestamp}</span>
        <span class="source">[${source}]</span>
        <span class="message">${message}</span>
    `;
    
    logContainer.insertBefore(entry, logContainer.firstChild);
    
    // Keep only last 50 entries
    while (logContainer.children.length > 50) {
        logContainer.removeChild(logContainer.lastChild);
    }
}

// ============================================================================
// DOCUMENTATION UI
// ============================================================================

function toggleSection(header) {
    const content = header.nextElementSibling;
    const isExpanded = content.style.display === 'block';
    
    // Close all sections
    document.querySelectorAll('.doc-content').forEach(c => {
        c.style.display = 'none';
    });
    document.querySelectorAll('.doc-section h3').forEach(h => {
        h.innerHTML = h.innerHTML.replace('&#9660;', '&#9654;');
    });
    
    // Toggle clicked section
    if (!isExpanded) {
        content.style.display = 'block';
        header.innerHTML = header.innerHTML.replace('&#9654;', '&#9660;');
    }
}

// Initialize first section as open
document.addEventListener('DOMContentLoaded', function() {
    const firstSection = document.querySelector('.doc-content');
    if (firstSection) {
        firstSection.style.display = 'block';
        const firstHeader = document.querySelector('.doc-section h3');
        if (firstHeader) {
            firstHeader.innerHTML = firstHeader.innerHTML.replace('&#9654;', '&#9660;');
        }
    }
});

// ============================================================================
// KEYBOARD SHORTCUTS
// ============================================================================

document.addEventListener('keydown', function(e) {
    // Konami code detection
    if (window.konamiSequence === undefined) {
        window.konamiSequence = [];
    }
    
    const konamiCode = ['ArrowUp', 'ArrowUp', 'ArrowDown', 'ArrowDown', 
                        'ArrowLeft', 'ArrowRight', 'ArrowLeft', 'ArrowRight', 
                        'b', 'a'];
    
    window.konamiSequence.push(e.key);
    window.konamiSequence = window.konamiSequence.slice(-10);
    
    if (window.konamiSequence.join(',') === konamiCode.join(',')) {
        logMessage('System', 'Konami code activated - Auxiliary stop sequence');
        runProbe();
    }
    
    // Quick keys
    if (e.key === 'p' || e.key === 'P') {
        runProbe();
    }
    if (e.key === 'f' || e.key === 'F') {
        requestNeed('food');
    }
    if (e.key === 'w' || e.key === 'W') {
        requestNeed('water');
    }
    if (e.key === 's' || e.key === 'S') {
        requestNeed('shelter');
    }
});

// ============================================================================
// EXPORT FUNCTIONS FOR CONSOLE USE
// ============================================================================

window.NSIGII = {
    state: NSIGII_STATE,
    runProbe,
    requestNeed,
    submitVote,
    updateLocation,
    logMessage
};

console.log('NSIGII Console API available. Try: NSIGII.runProbe()');
