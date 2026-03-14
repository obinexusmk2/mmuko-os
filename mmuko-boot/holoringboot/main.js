/**
 * NSIGII/MMUKO-OS Interactive Illustration
 * Implements the Trident Signal Path and Metaphysical Login logic.
 * Uses PixelBuffer.js for aura generation and Vector.js for geometry.
 */

const canvas = document.getElementById('mmukoCanvas');
const ctx = canvas.getContext('2d');
const protocolStatus = document.getElementById('protocol-status');
const nodeStatus = document.getElementById('node-status');
const resetBtn = document.getElementById('reset-btn');

// State
let width, height;
let center;
let pixelBuffer = null;
let bgImageData = null;

// Trident State (Identity, Device, Time)
const PRONGS = [
    { id: 'identity', targetAngle: 0, currentAngle: Math.random() * Math.PI * 2, color: '#FFB8D1' }, // Pink
    { id: 'device', targetAngle: (2 * Math.PI) / 3, currentAngle: Math.random() * Math.PI * 2, color: '#84DCCF' }, // Cyan
    { id: 'time', targetAngle: (4 * Math.PI) / 3, currentAngle: Math.random() * Math.PI * 2, color: '#F09D51' }  // Orange
];

let isDragging = false;
let draggedProngIndex = -1;
let lastTime = 0;
let time = 0;

// Aura/Static Configuration
const AURA_COHERENCE = 0.954; // 95.4% coherence
const NOISE_INTENSITY = 20;

/**
 * Initialization
 */
function init() {
    resize();
    window.addEventListener('resize', resize);
    
    // Init Background Buffer
    bgImageData = ctx.createImageData(width, height);
    pixelBuffer = new PixelBuffer(bgImageData);
    
    // Start Loop
    requestAnimationFrame(loop);
    
    // Input Handling
    canvas.addEventListener('mousedown', handlePointerDown);
    canvas.addEventListener('mousemove', handlePointerMove);
    canvas.addEventListener('mouseup', handlePointerUp);
    canvas.addEventListener('touchstart', (e) => handlePointerDown(e.touches[0]));
    canvas.addEventListener('touchmove', (e) => handlePointerMove(e.touches[0]));
    canvas.addEventListener('touchend', handlePointerUp);

    resetBtn.addEventListener('click', resetProngs);
}

function resize() {
    width = window.innerWidth;
    height = window.innerHeight;
    canvas.width = width;
    canvas.height = height;
    center = new Vector.Vec2(width / 2, height / 2);
    
    // Re-init buffer on resize
    bgImageData = ctx.createImageData(width, height);
    pixelBuffer = new PixelBuffer(bgImageData);
}

function resetProngs() {
    PRONGS.forEach(p => {
        p.currentAngle = Math.random() * Math.PI * 2;
        p.locked = false;
    });
}

/**
 * Core Loop
 */
function loop(timestamp) {
    const dt = (timestamp - lastTime) / 1000;
    lastTime = timestamp;
    time += dt;

    // 1. Draw Metaphysical Static (PixelBuffer)
    generateAura(time);
    ctx.putImageData(bgImageData, 0, 0);

    // 2. Draw Protocol Layer (Vector Graphics)
    drawProtocolLayer(ctx, center);

    // 3. Draw Trident & Signal Path
    drawTrident(ctx, center);
    
    // 4. Draw Signal Trace (The "Squiggle")
    drawSignalTrace(ctx, center, time);

    // 5. Check Logic
    checkCoherence();

    requestAnimationFrame(loop);
}

/**
 * Generates the "Metaphysical Static" using PixelBuffer.js
 * Represents the 95.4% Aura-Seal Coherence.
 */
function generateAura(t) {
    // We only update a subset of pixels to simulate "sparkle" or static
    // without killing CPU performance.
    
    const len = pixelBuffer.buffer.length;
    // Fill slightly dark background
    // 0xFF050505 (ABGR Little Endian usually) -> Black/Dark Grey
    
    // Optimization: Don't clear every frame, just fade or noise overlay.
    // Let's create a dynamic noise field.
    
    for (let i = 0; i < 2000; i++) { // Update 2000 random pixels per frame
        const idx = Math.floor(Math.random() * len);
        const noise = Math.random();
        
        // If coherence is high, noise is low.
        // We want faint stars/particles.
        
        let color = 0xFF000000; // Alpha=255, Blue=0, Green=0, Red=0
        
        if (noise > AURA_COHERENCE) {
            // Glitch pixel (White/Grey)
            color = 0xFF202020;
        } else if (noise < 0.001) {
            // Spark (Gold)
            color = 0xFF00D7FF; // Gold-ish in ABGR (A=FF, B=00, G=D7, R=FF)
        } else {
             // Dark Void
             color = 0xFF050505;
        }
        
        pixelBuffer.buffer[idx] = color;
    }
}

/**
 * Draws the MMUKO-OS concentric circles and axes.
 */
function drawProtocolLayer(ctx, center) {
    ctx.strokeStyle = 'rgba(50, 50, 50, 0.5)';
    ctx.lineWidth = 1;
    
    const radius = Math.min(width, height) * 0.35;

    // Main Circle
    ctx.beginPath();
    ctx.arc(center.x, center.y, radius, 0, Math.PI * 2);
    ctx.stroke();

    // Inner Circle (The Void)
    ctx.beginPath();
    ctx.arc(center.x, center.y, radius * 0.3, 0, Math.PI * 2);
    ctx.stroke();

    // Axes
    ctx.beginPath();
    ctx.moveTo(center.x, center.y - radius * 1.2);
    ctx.lineTo(center.x, center.y + radius * 1.2);
    ctx.moveTo(center.x - radius * 1.2, center.y);
    ctx.lineTo(center.x + radius * 1.2, center.y);
    ctx.stroke();
}

/**
 * Draws the interactive Trident prongs.
 */
function drawTrident(ctx, center) {
    const radius = Math.min(width, height) * 0.35;
    const nodeRadius = 15;

    PRONGS.forEach((p, index) => {
        const x = center.x + Math.cos(p.currentAngle) * radius;
        const y = center.y + Math.sin(p.currentAngle) * radius;

        // Draw Connector Line
        ctx.beginPath();
        ctx.moveTo(center.x, center.y);
        ctx.lineTo(x, y);
        ctx.strokeStyle = p.locked ? p.color : 'rgba(100,100,100,0.5)';
        ctx.lineWidth = 2;
        ctx.stroke();

        // Draw Node
        ctx.beginPath();
        ctx.arc(x, y, nodeRadius, 0, Math.PI * 2);
        ctx.fillStyle = p.locked ? p.color : '#333';
        ctx.fill();
        ctx.strokeStyle = p.color;
        ctx.stroke();

        // Label
        ctx.fillStyle = p.color;
        ctx.font = '12px Courier New';
        ctx.fillText(p.id.toUpperCase(), x + 20, y);
    });
}

/**
 * Draws the "Signal Path" described in the text.
 * South -> East -> West -> West (Anchor) -> Loop -> North...
 */
function drawSignalTrace(ctx, center, t) {
    const scale = Math.min(width, height) * 0.1;
    ctx.save();
    ctx.translate(center.x, center.y);
    
    // The visual representation of the "Nnamdi Signal"
    // Using vector points based on the text description
    
    const points = [
        new Vector.Vec2(0, scale * 2),    // South
        new Vector.Vec2(scale * 2, 0),    // East
        new Vector.Vec2(-scale * 2, 0),   // West
        new Vector.Vec2(-scale * 1.5, -scale * 0.5), // Anchor/Loop
        new Vector.Vec2(0, -scale * 2),   // North
        new Vector.Vec2(0, -scale * 2.5), // Double Rise
        new Vector.Vec2(scale * 0.5, -scale * 2.2) // Squiggle
    ];

    ctx.beginPath();
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.2)';
    
    // Only draw if logged in or coherent
    const allLocked = PRONGS.every(p => p.locked);
    if (allLocked) {
        ctx.strokeStyle = '#00ff41'; // Terminal Green
        ctx.shadowBlur = 10;
        ctx.shadowColor = '#00ff41';
    }

    ctx.lineWidth = 3;
    
    // Animate drawing the path
    ctx.moveTo(points[0].x, points[0].y);
    for(let i=1; i<points.length; i++) {
        ctx.lineTo(points[i].x, points[i].y);
    }
    
    ctx.stroke();
    ctx.restore();
}

/**
 * Check if prongs are aligned with their targets.
 */
function checkCoherence() {
    let alignedCount = 0;
    const tolerance = 0.15; // Radians (~8 degrees)

    PRONGS.forEach(p => {
        // Normalize angles to 0-2PI
        let current = p.currentAngle % (Math.PI * 2);
        if (current < 0) current += Math.PI * 2;
        
        let target = p.targetAngle;
        
        let diff = Math.abs(current - target);
        // Handle wrap around (0 vs 2PI)
        if (diff > Math.PI) diff = (Math.PI * 2) - diff;

        if (diff < tolerance) {
            p.locked = true;
            // Snap visual
            // p.currentAngle = p.targetAngle; 
            alignedCount++;
        } else {
            p.locked = false;
        }
    });

    nodeStatus.innerText = `${alignedCount}/3 ALIGNED`;
    
    if (alignedCount === 3) {
        protocolStatus.innerText = "ACCESS GRANTED";
        protocolStatus.style.color = "#00ff41";
    } else {
        protocolStatus.innerText = "AWAITING TRIDENT";
        protocolStatus.style.color = "#e0e0e0";
    }
}

/**
 * Interaction Handlers
 */
function handlePointerDown(e) {
    const mousePos = getMousePos(e);
    const radius = Math.min(width, height) * 0.35;
    
    // Check collision with nodes
    PRONGS.forEach((p, index) => {
        const px = center.x + Math.cos(p.currentAngle) * radius;
        const py = center.y + Math.sin(p.currentAngle) * radius;
        
        const dist = Math.sqrt(Math.pow(mousePos.x - px, 2) + Math.pow(mousePos.y - py, 2));
        if (dist < 30) {
            isDragging = true;
            draggedProngIndex = index;
        }
    });
}

function handlePointerMove(e) {
    if (!isDragging || draggedProngIndex === -1) return;
    
    const mousePos = getMousePos(e);
    const dx = mousePos.x - center.x;
    const dy = mousePos.y - center.y;
    
    // Calculate new angle
    const angle = Math.atan2(dy, dx);
    PRONGS[draggedProngIndex].currentAngle = angle;
}

function handlePointerUp() {
    isDragging = false;
    draggedProngIndex = -1;
}

function getMousePos(evt) {
    const rect = canvas.getBoundingClientRect();
    // Handle Touch vs Mouse
    const clientX = evt.clientX || evt.pageX;
    const clientY = evt.clientY || evt.pageY;
    
    return {
        x: clientX - rect.left,
        y: clientY - rect.top
    };
}

// Boot
init();