# NSIGII / MMUKO-OS Trident Protocol Visualizer

This build adds a **gesture-auth mode** on top of the original trident coherence boot ritual.

## What is new

- **ENROLL**: capture a hand-gesture template from tracked landmarks.
- **VERIFY**: compare live gesture to enrolled template using a match threshold + trident coherence gate.
- **SUCCESS**: gesture or fallback credential accepted.
- **LOCKED**: too many failed attempts (5/5), requiring fallback credential.
- **Fallback credential flow**: password-style unlock path for camera/model failure or gesture mismatch.

## Gesture pipeline

1. Browser loads **MediaPipe Hands** (`@mediapipe/hands` + `camera_utils`) from CDN.
2. Single-hand landmarks are tracked in real time.
3. Landmark mapping to trident nodes:
   - `P1` angle <- index fingertip (landmark 8) relative to wrist (0)
   - `P2` angle <- middle fingertip (12) relative to wrist
   - `P3` angle <- ring fingertip (16) relative to wrist
4. Existing trident coherence logic computes alignment and % coherence.
5. Verification requires:
   - `similarity >= threshold (0.78)`
   - `coherence >= 95.4%`

## Security and privacy constraints

### Data minimization

- Raw landmark arrays are **not persisted**.
- Enrollment stores only a **derived signature** (quantized geometric feature vector), plus:
  - random salt
  - SHA-256 integrity hash of (`salt + signature`)

### Fallback credential storage

- Fallback credential is never stored in plaintext.
- Stored as `PBKDF2(SHA-256, 120k iterations, random salt)` derived hash in localStorage.

### Local-only processing

- Hand landmarks are processed in-browser.
- No backend transmission is implemented by default.
- Camera stream stays local to the browser runtime.

### Operational caveats

- CDN dependency: MediaPipe scripts must be reachable.
- `localStorage` is origin-scoped and not a hardware secure enclave.
- This is a prototype UX/security model, not a certified biometric authenticator.

## Setup

1. Serve `holoringboot/` via any static web server (required for camera APIs in most browsers).
2. Open the page in a browser with camera permission enabled.
3. On first run:
   - Set fallback credential (recommended)
   - Position hand in frame
   - Click **Capture Enrollment**
4. Click **Run Verify** to authenticate with live gesture.
5. If gesture flow fails/unavailable, use **Unlock via Credential**.

## Controls

- **Capture Enrollment**: records a new derived template.
- **Run Verify**: evaluates live hand against template.
- **Use Fallback**: switches operator attention to passphrase unlock flow.
- **Set/Update Credential**: writes PBKDF2-derived credential hash.
- **Unlock via Credential**: verifies fallback credential and enters SUCCESS.

## Notes

- Mouse dragging is still available as a manual trident control when hand tracking is unavailable.
- If template integrity check fails, the app deletes the template and forces re-enrollment.
