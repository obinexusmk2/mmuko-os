const canvas = document.getElementById('mmukoCanvas');
const ctx = canvas.getContext('2d', { alpha: false });
const video = document.getElementById('gestureVideo');

const MODE = {
    ENROLL: 'ENROLL',
    VERIFY: 'VERIFY',
    SUCCESS: 'SUCCESS',
    LOCKED: 'LOCKED'
};

const THRESHOLD = 0.78;
const MAX_ATTEMPTS = 5;
const STORAGE_KEY = 'mmukoGestureTemplateV1';
const CREDENTIAL_KEY = 'mmukoFallbackCredentialV1';

const ui = {
    mode: document.getElementById('mode-val'),
    state: document.getElementById('state-val'),
    coherence: document.getElementById('coherence-val'),
    match: document.getElementById('match-val'),
    attempts: document.getElementById('attempts-val'),
    tracking: document.getElementById('tracking-val'),
    p1: document.getElementById('p1-status'),
    p2: document.getElementById('p2-status'),
    p3: document.getElementById('p3-status'),
    logs: document.getElementById('logs'),
    enrollBtn: document.getElementById('enroll-btn'),
    verifyBtn: document.getElementById('verify-btn'),
    fallbackBtn: document.getElementById('fallback-btn'),
    credentialInput: document.getElementById('credential-input'),
    setCredentialBtn: document.getElementById('set-credential-btn'),
    unlockBtn: document.getElementById('unlock-btn')
};

const state = {
    width: 0,
    height: 0,
    centerX: 0,
    centerY: 0,
    ringRadius: 0,
    entropy: 9.99,
    coherence: 0,
    mode: MODE.ENROLL,
    attempts: 0,
    trackingReady: false,
    lastLandmarks: null,
    draggingNode: null,
    matchedScore: null,
    template: null,
    nodes: [
        { id: 'P1', target: 0, angle: Math.PI / 2, color: '#D48C45', active: false },
        { id: 'P2', target: (5 * Math.PI) / 4, angle: Math.PI, color: '#A68B5B', active: false },
        { id: 'P3', target: (7 * Math.PI) / 4, angle: (3 * Math.PI) / 2, color: '#B8860B', active: false }
    ]
};

function log(message, type = '') {
    const entry = document.createElement('div');
    entry.className = `log-entry ${type}`;
    entry.textContent = `[${new Date().toLocaleTimeString()}] ${message}`;
    ui.logs.appendChild(entry);
    ui.logs.scrollTop = ui.logs.scrollHeight;
}

function setMode(nextMode) {
    state.mode = nextMode;
    ui.mode.textContent = nextMode;
    ui.mode.className = 'value';
    if (nextMode === MODE.SUCCESS) ui.mode.classList.add('verified');
    if (nextMode === MODE.LOCKED) ui.mode.classList.add('locked');
    ui.state.textContent = nextMode === MODE.SUCCESS ? 'VERIFIED' : nextMode;
    ui.state.className = `value ${nextMode === MODE.SUCCESS ? 'verified' : nextMode === MODE.LOCKED ? 'locked' : ''}`;
}

function resize() {
    state.width = canvas.width = window.innerWidth;
    state.height = canvas.height = window.innerHeight;
    state.centerX = state.width / 2;
    state.centerY = state.height / 2;
    state.ringRadius = Math.min(state.width, state.height) * 0.3;
}

function normalizeAngle(angle) {
    let a = angle % (Math.PI * 2);
    if (a < 0) a += Math.PI * 2;
    return a;
}

function updateCoherence() {
    let totalDelta = 0;
    state.nodes.forEach((node, idx) => {
        let delta = Math.abs(normalizeAngle(node.angle) - normalizeAngle(node.target));
        if (delta > Math.PI) delta = (Math.PI * 2) - delta;
        const nodeCoherence = Math.max(0, 1 - delta / 0.4);
        node.active = nodeCoherence > 0.96;
        totalDelta += delta;

        const pStatus = [ui.p1, ui.p2, ui.p3][idx];
        pStatus.textContent = node.active ? 'ALIGNED' : 'PENDING';
        pStatus.className = `value ${node.active ? 'verified' : ''}`;
    });

    const avgCoherence = 1 - totalDelta / (Math.PI * 1.2);
    state.coherence = Math.max(0, Math.min(100, avgCoherence * 100));
    state.entropy = Math.max(0, 9.99 * (1 - avgCoherence));

    ui.coherence.textContent = `${state.coherence.toFixed(1)}%`;
}

function drawNoise() {
    const intensity = state.mode === MODE.SUCCESS ? 4 : Math.floor(state.entropy * 20);
    ctx.fillStyle = '#2C2A28';
    ctx.fillRect(0, 0, state.width, state.height);
    const particles = state.mode === MODE.SUCCESS ? 400 : 1600;

    for (let i = 0; i < particles; i++) {
        const x = Math.random() * state.width;
        const y = Math.random() * state.height;
        const n = Math.random() * intensity;
        const c = Math.floor(44 + n);
        ctx.fillStyle = `rgba(${c},${c - 2},${c - 4},0.28)`;
        ctx.fillRect(x, y, 1, 1);
    }
}

function drawRing() {
    ctx.beginPath();
    ctx.arc(state.centerX, state.centerY, state.ringRadius, 0, Math.PI * 2);
    ctx.strokeStyle = state.mode === MODE.SUCCESS ? 'rgba(0,255,65,0.45)' : 'rgba(166, 139, 91, 0.25)';
    ctx.lineWidth = 1;
    ctx.setLineDash([10, 5]);
    ctx.stroke();
    ctx.setLineDash([]);

    state.nodes.forEach(node => {
        const x = state.centerX + Math.cos(node.angle) * state.ringRadius;
        const y = state.centerY + Math.sin(node.angle) * state.ringRadius;

        ctx.beginPath();
        ctx.moveTo(state.centerX, state.centerY);
        ctx.lineTo(x, y);
        ctx.strokeStyle = node.active ? 'rgba(0, 255, 65, 0.7)' : node.color;
        ctx.lineWidth = node.active ? 2 : 1;
        ctx.stroke();

        ctx.beginPath();
        ctx.arc(x, y, 12, 0, Math.PI * 2);
        ctx.fillStyle = node.active ? '#00ff41' : node.color;
        ctx.fill();

        ctx.fillStyle = '#EAE6E1';
        ctx.font = 'bold 11px Courier New';
        ctx.textAlign = 'center';
        ctx.fillText(node.id, x, y + 26);
    });

    ctx.beginPath();
    ctx.arc(state.centerX, state.centerY, 6, 0, Math.PI * 2);
    ctx.fillStyle = state.mode === MODE.SUCCESS ? '#00ff41' : '#D48C45';
    ctx.fill();
}

function frame() {
    drawNoise();
    drawRing();
    updateCoherence();
    requestAnimationFrame(frame);
}

function mapLandmarksToNodes(landmarks) {
    if (!landmarks || landmarks.length < 21) return;
    const wrist = landmarks[0];
    const points = [landmarks[8], landmarks[12], landmarks[16]]; // index, middle, ring

    state.nodes.forEach((node, idx) => {
        const p = points[idx];
        node.angle = Math.atan2(p.y - wrist.y, p.x - wrist.x);
    });
}

function buildSignatureFromLandmarks(landmarks) {
    const wrist = landmarks[0];
    const tips = [landmarks[4], landmarks[8], landmarks[12], landmarks[16], landmarks[20]];
    const base = [landmarks[1], landmarks[5], landmarks[9], landmarks[13], landmarks[17]];

    const features = [];
    for (let i = 0; i < tips.length; i++) {
        const dx = tips[i].x - wrist.x;
        const dy = tips[i].y - wrist.y;
        const bx = base[i].x - wrist.x;
        const by = base[i].y - wrist.y;
        const tipDist = Math.sqrt(dx * dx + dy * dy);
        const baseDist = Math.sqrt(bx * bx + by * by) || 1e-6;
        features.push(Math.min(2, Math.max(0, tipDist / baseDist)));
        features.push((Math.atan2(dy, dx) + Math.PI) / (2 * Math.PI));
    }

    const quantized = features.map(v => Math.round(v * 1023));
    return quantized;
}

function hammingSimilarity(a, b) {
    const len = Math.min(a.length, b.length);
    if (!len) return 0;
    let same = 0;
    for (let i = 0; i < len; i++) {
        const diff = Math.abs(a[i] - b[i]);
        if (diff <= 40) same++;
    }
    return same / len;
}

function randomSalt() {
    const bytes = new Uint8Array(16);
    crypto.getRandomValues(bytes);
    return Array.from(bytes).map(b => b.toString(16).padStart(2, '0')).join('');
}

async function sha256Hex(text) {
    const data = new TextEncoder().encode(text);
    const digest = await crypto.subtle.digest('SHA-256', data);
    return Array.from(new Uint8Array(digest)).map(b => b.toString(16).padStart(2, '0')).join('');
}

async function pbkdf2Hex(secret, salt) {
    const keyMaterial = await crypto.subtle.importKey('raw', new TextEncoder().encode(secret), 'PBKDF2', false, ['deriveBits']);
    const bits = await crypto.subtle.deriveBits({ name: 'PBKDF2', salt: new TextEncoder().encode(salt), iterations: 120000, hash: 'SHA-256' }, keyMaterial, 256);
    return Array.from(new Uint8Array(bits)).map(b => b.toString(16).padStart(2, '0')).join('');
}

async function enrollGesture() {
    if (!state.lastLandmarks) {
        log('Enrollment failed: no hand landmarks detected.', 'error');
        return;
    }

    const signature = buildSignatureFromLandmarks(state.lastLandmarks);
    const salt = randomSalt();
    const hash = await sha256Hex(`${salt}:${signature.join(',')}`);

    state.template = { signature, salt, hash, createdAt: Date.now(), threshold: THRESHOLD };
    localStorage.setItem(STORAGE_KEY, JSON.stringify(state.template));

    setMode(MODE.VERIFY);
    log('Gesture template enrolled (derived + hashed, raw landmarks discarded).', 'success');
}

async function verifyGesture() {
    if (state.mode === MODE.LOCKED) {
        log('Verification blocked: protocol is LOCKED.', 'error');
        return;
    }
    if (!state.template) {
        log('No gesture template found. Enroll first.', 'warn');
        setMode(MODE.ENROLL);
        return;
    }
    if (!state.lastLandmarks) {
        log('No live hand detected. Use fallback credential flow.', 'warn');
        return;
    }

    const integrity = await sha256Hex(`${state.template.salt}:${state.template.signature.join(',')}`);
    if (integrity !== state.template.hash) {
        log('Stored template integrity check failed. Re-enroll required.', 'error');
        setMode(MODE.ENROLL);
        localStorage.removeItem(STORAGE_KEY);
        state.template = null;
        return;
    }

    const liveSignature = buildSignatureFromLandmarks(state.lastLandmarks);
    const similarity = hammingSimilarity(liveSignature, state.template.signature);
    state.matchedScore = similarity;
    ui.match.textContent = `${(similarity * 100).toFixed(1)}%`;

    if (similarity >= state.template.threshold && state.coherence >= 95.4) {
        setMode(MODE.SUCCESS);
        log(`Gesture verified: ${(similarity * 100).toFixed(1)}% match, coherence ${state.coherence.toFixed(1)}%.`, 'success');
        return;
    }

    state.attempts += 1;
    ui.attempts.textContent = `${state.attempts}/${MAX_ATTEMPTS}`;
    log(`Verify failed (${(similarity * 100).toFixed(1)}% match). Attempt ${state.attempts}/${MAX_ATTEMPTS}.`, 'warn');
    if (state.attempts >= MAX_ATTEMPTS) {
        setMode(MODE.LOCKED);
        log('Too many failed attempts. Protocol LOCKED. Use fallback credential.', 'error');
    } else {
        setMode(MODE.VERIFY);
    }
}

async function setFallbackCredential() {
    const secret = ui.credentialInput.value.trim();
    if (secret.length < 8) {
        log('Credential must be at least 8 characters.', 'warn');
        return;
    }
    const salt = randomSalt();
    const hash = await pbkdf2Hex(secret, salt);
    localStorage.setItem(CREDENTIAL_KEY, JSON.stringify({ salt, hash, updatedAt: Date.now() }));
    ui.credentialInput.value = '';
    log('Fallback credential stored via PBKDF2-derived hash.', 'success');
}

async function unlockWithCredential() {
    const payload = localStorage.getItem(CREDENTIAL_KEY);
    if (!payload) {
        log('No fallback credential configured.', 'warn');
        return;
    }
    const secret = ui.credentialInput.value.trim();
    if (!secret) {
        log('Enter fallback credential first.', 'warn');
        return;
    }
    const record = JSON.parse(payload);
    const hash = await pbkdf2Hex(secret, record.salt);
    if (hash === record.hash) {
        state.attempts = 0;
        ui.attempts.textContent = `${state.attempts}/${MAX_ATTEMPTS}`;
        ui.match.textContent = 'CREDENTIAL';
        setMode(MODE.SUCCESS);
        log('Fallback credential accepted. ACCESS GRANTED.', 'success');
    } else {
        log('Fallback credential rejected.', 'error');
    }
    ui.credentialInput.value = '';
}

function enableFallbackMode() {
    if (state.mode !== MODE.SUCCESS) {
        log('Switched to fallback credential flow.', 'phase');
        setMode(MODE.VERIFY);
    }
}

function onHandsResults(results) {
    const landmarks = results.multiHandLandmarks && results.multiHandLandmarks[0];
    if (landmarks) {
        state.lastLandmarks = landmarks;
        mapLandmarksToNodes(landmarks);
        ui.tracking.textContent = 'HAND LOCK';
        ui.tracking.className = 'value verified';
    } else {
        state.lastLandmarks = null;
        ui.tracking.textContent = 'NO HAND';
        ui.tracking.className = 'value warn';
    }
}

async function initHandTracking() {
    if (!window.Hands || !window.Camera) {
        ui.tracking.textContent = 'UNAVAILABLE';
        ui.tracking.className = 'value locked';
        log('Hand-tracking provider unavailable. Fallback-only mode.', 'warn');
        return;
    }

    try {
        const hands = new Hands({
            locateFile: (file) => `https://cdn.jsdelivr.net/npm/@mediapipe/hands/${file}`
        });
        hands.setOptions({
            maxNumHands: 1,
            modelComplexity: 1,
            minDetectionConfidence: 0.65,
            minTrackingConfidence: 0.6
        });
        hands.onResults(onHandsResults);

        const camera = new Camera(video, {
            onFrame: async () => {
                await hands.send({ image: video });
            },
            width: 640,
            height: 480
        });
        await camera.start();
        state.trackingReady = true;
        ui.tracking.textContent = 'READY';
        ui.tracking.className = 'value verified';
        log('MediaPipe Hands initialized. Landmarks now drive trident angles.', 'phase');
    } catch (err) {
        ui.tracking.textContent = 'ERROR';
        ui.tracking.className = 'value locked';
        log(`Tracking init failed: ${err.message}.`, 'error');
    }
}

function wireInteractions() {
    ui.enrollBtn.addEventListener('click', enrollGesture);
    ui.verifyBtn.addEventListener('click', verifyGesture);
    ui.fallbackBtn.addEventListener('click', enableFallbackMode);
    ui.setCredentialBtn.addEventListener('click', setFallbackCredential);
    ui.unlockBtn.addEventListener('click', unlockWithCredential);

    window.addEventListener('mousedown', (e) => {
        const mouse = { x: e.clientX, y: e.clientY };
        state.nodes.forEach(node => {
            const nx = state.centerX + Math.cos(node.angle) * state.ringRadius;
            const ny = state.centerY + Math.sin(node.angle) * state.ringRadius;
            const d = Math.hypot(mouse.x - nx, mouse.y - ny);
            if (d < 25) state.draggingNode = node;
        });
    });

    window.addEventListener('mousemove', (e) => {
        if (!state.draggingNode || state.lastLandmarks) return;
        state.draggingNode.angle = Math.atan2(e.clientY - state.centerY, e.clientX - state.centerX);
    });
    window.addEventListener('mouseup', () => { state.draggingNode = null; });
    window.addEventListener('resize', resize);
}

function restoreState() {
    const templateRaw = localStorage.getItem(STORAGE_KEY);
    if (templateRaw) {
        try {
            state.template = JSON.parse(templateRaw);
            setMode(MODE.VERIFY);
            log('Loaded stored gesture template.', 'phase');
        } catch {
            localStorage.removeItem(STORAGE_KEY);
            log('Corrupt template removed; re-enroll required.', 'warn');
        }
    } else {
        setMode(MODE.ENROLL);
    }
}

function boot() {
    resize();
    wireInteractions();
    restoreState();
    frame();
    initHandTracking();
    log('Gesture-auth protocol online. States: ENROLL, VERIFY, SUCCESS, LOCKED.', 'phase');
}

boot();
