"""
NSIGII Python Mailer — Magnetic Message Protocol
================================================
OBINexus Computing | Nnamdi Okpala | 21 February 2026

Sends a .nsigii artifact (linkable-then-executable codec) via SMTP.
Supports:
  - Raw .nsigii attachment
  - .nsigii.tar.gz wrapper (rollback wheel)
  - Human-rights body (Alice/Bob/Ledger model)
  - Constitutional rights header embedded in MIME

Workflow:
  nsigii_magnetic.h → nsigii_mag_teleport() → .nsigii binary
  → tar.gz (rollback wheel) → SMTP (STARTTLS) → recipient

Usage:
  python3 nsigii_mailer.py [--simulate]   # --simulate skips actual send
"""

import smtplib
import tarfile
import struct
import hashlib
import time
import os
import sys
import json
import argparse
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from email.mime.base import MIMEBase
from email import encoders

# ============================================================
# CONFIG — replace with real credentials for live use
# ============================================================
SMTP_SERVER   = 'smtp.gmail.com'
SMTP_PORT     = 587
SENDER_EMAIL  = 'okpalan86@gmail.com'
SENDER_PASSWORD = os.environ.get('NSIGII_SMTP_PASSWORD', 'your_app_password')

RECIPIENTS = [
    'alice@somefakeemail.com',
    'bob@somefakeemail.com',
]

NSIGII_FILE = 'demo.nsigii'
TARGZ_FILE  = 'demo.nsigii.tar.gz'

# ============================================================
# NSIGII ARTIFACT GENERATOR
# Produces a binary .nsigii file that encodes the spring-physics
# collapse state (Hooke's Law model from transcript).
#
# Binary layout:
#   [0:4]   Magic  b'NSGII'  (4 bytes, no null)
#   [4]     Version 0x01     (1 byte)
#   [5]     Pole    0=UCHE, 1=EZE, 2=OBI  (1 byte)
#   [6]     State   0-5 (encoded→sealed)  (1 byte)
#   [7]     Reserved 0x00                 (1 byte)
#   [8:16]  Force    double big-endian    (8 bytes)
#   [16:24] Stiffness double big-endian   (8 bytes)
#   [24:32] Extension double big-endian   (8 bytes)
#   [32:40] Encoded-at timestamp double   (8 bytes)
#   [40:48] Collapsed-at timestamp double (8 bytes)
#   [48:56] Collapse ratio double         (8 bytes)
#   [56:72] LTE sig  16 bytes             (16 bytes)
#   [72:80] Content length uint64         (8 bytes)
#   [80:]   Content UTF-8 bytes
#   [-32:]  SHA-256 of all preceding bytes (32 bytes)
# ============================================================

MAG_VERSION   = 0x01
MAGIC         = b'NSGI'   # 4 bytes

# Pole IDs
POLE_UCHE = 0
POLE_EZE  = 1
POLE_OBI  = 2

# State IDs
STATE_ENCODED    = 0
STATE_ORIENTED   = 1
STATE_SENDING    = 2
STATE_IN_TRANSIT = 3
STATE_COLLAPSED  = 4
STATE_SEALED     = 5

NSIGII_DECAY_CONSTANT = 0.36787944117144233   # 1/e

def spring_force(force: float, stiffness: float) -> float:
    """F = K × E (solved for F when E=F/K)"""
    return force  # force is already the input

def full_extension(force: float, stiffness: float) -> float:
    """E = F / K"""
    return force / stiffness if stiffness > 0 else 0.0

def half_extension(force: float, stiffness: float) -> float:
    """E_half = sqrt(F/K) — MAYBE-state threshold"""
    import math
    return math.sqrt(force / stiffness) if stiffness > 0 else 0.0

def collapse_ratio(force: float, stiffness: float, extension: float) -> float:
    """Collapse ratio: 0.0 (not sent) → 1.0 (fully collapsed/received)"""
    full = full_extension(force, stiffness)
    return min(1.0, extension / full) if full > 0 else 0.0

def rotation_force_multiplier(rotation_fraction: float) -> float:
    """Half-rotation rule: ½ rot → ×2 force at receiver"""
    return 1.0 / rotation_fraction if rotation_fraction > 0 else 0.0

def lte_signature(content: str, timestamp: float) -> bytes:
    """Generate 16-byte LTE forensic signature"""
    raw = f"LTE:{content}:{timestamp:.6f}".encode()
    return hashlib.sha256(raw).digest()[:16]

def generate_nsigii(
    content: str,
    force: float = 1.0,
    stiffness: float = 0.67,   # CONSENSUS_THRESHOLD from nsigii.h
    pole: int = POLE_UCHE,
    state: int = STATE_SEALED,
    filepath: str = NSIGII_FILE,
) -> dict:
    """
    Generate a .nsigii binary artifact encoding one magnetic message.
    Returns a dict with metadata for the email body.
    """
    extension   = full_extension(force, stiffness)
    encoded_at  = time.time()
    collapsed_at = encoded_at + 0.001   # Immediate collapse (lab simulation)
    ratio       = collapse_ratio(force, stiffness, extension)
    sig_bytes   = lte_signature(content, collapsed_at)

    content_bytes = content.encode('utf-8')
    content_len   = len(content_bytes)

    header = struct.pack(
        '>4sBBBB',
        MAGIC,
        MAG_VERSION,
        pole & 0xFF,
        state & 0xFF,
        0x00,            # reserved
    )
    physics = struct.pack('>ddddd', force, stiffness, extension, encoded_at, collapsed_at)
    ratio_b = struct.pack('>d', ratio)
    sig_b   = sig_bytes   # 16 bytes
    content_header = struct.pack('>Q', content_len)

    payload = (header + physics + ratio_b + sig_b + content_header + content_bytes)
    sha256  = hashlib.sha256(payload).digest()   # 32-byte integrity hash

    artifact = payload + sha256

    with open(filepath, 'wb') as f:
        f.write(artifact)

    metadata = {
        "message_id":    f"MSG{int(collapsed_at * 1000) % 99999:05d}",
        "content":       content,
        "force":         force,
        "stiffness":     stiffness,
        "extension":     extension,
        "collapse_ratio": ratio,
        "encoded_at":    encoded_at,
        "collapsed_at":  collapsed_at,
        "lte_sig":       sig_bytes.hex(),
        "pole":          ["UCHE (Knowledge)", "EZE (Leadership)", "OBI (Heart)"][pole],
        "state":         ["ENCODED","ORIENTED","SENDING","IN_TRANSIT",
                          "COLLAPSED","SEALED"][state],
        "file_bytes":    len(artifact),
        "sha256":        sha256.hex(),
    }

    print(f"[NSIGII] Generated {filepath} ({len(artifact)} bytes)")
    print(f"         Collapse ratio : {ratio:.4f}")
    print(f"         LTE signature  : {sig_bytes.hex()}")
    print(f"         SHA-256        : {sha256.hex()[:16]}…")

    return metadata

# ============================================================
# ROLLBACK WHEEL — tar.gz wrapper
# ============================================================

def create_rollback_wheel(nsigii_path: str = NSIGII_FILE,
                           targz_path: str = TARGZ_FILE) -> None:
    """Wrap .nsigii in a .tar.gz for rollback wheel protocol."""
    with tarfile.open(targz_path, 'w:gz') as tar:
        tar.add(nsigii_path)
    size = os.path.getsize(targz_path)
    print(f"[WHEEL] Rollback wheel: {targz_path} ({size} bytes)")

# ============================================================
# EMAIL BUILDER — Alice/Bob/Ledger model
# ============================================================

def build_email(metadata: dict,
                attachment_path: str,
                sender: str,
                recipients: list[str]) -> MIMEMultipart:
    """
    Build MIME multipart email per the NSIGII Human Rights Protocol.

    Headers added:
      X-NSIGII-Version   : 0.1
      X-NSIGII-MsgID     : <message_id>
      X-NSIGII-State     : SEALED
      X-NSIGII-LTE-Sig   : <hex>
      X-NSIGII-CollapseRatio : <ratio>
      X-NSIGII-Rights    : Receiver-Irrevocable; Sender-Retrieval-Revoked
    """
    msg = MIMEMultipart()
    msg['From']    = sender
    msg['To']      = ', '.join(recipients)
    msg['Subject'] = (
        f"NSIGII Human Rights Request [{metadata['message_id']}]: "
        "I need food, water, and shelter now"
    )

    # Constitutional headers
    msg['X-NSIGII-Version']       = '0.1'
    msg['X-NSIGII-MsgID']         = metadata['message_id']
    msg['X-NSIGII-State']         = metadata['state']
    msg['X-NSIGII-LTE-Sig']       = metadata['lte_sig']
    msg['X-NSIGII-CollapseRatio'] = f"{metadata['collapse_ratio']:.6f}"
    msg['X-NSIGII-Rights']        = (
        'Receiver-Ownership=IRREVOCABLE; Sender-Retrieval=REVOKED; '
        'Ledger-Witnessing=ACTIVE; Controller-EZE=ACCOUNTABLE'
    )

    body = f"""
NSIGII HUMAN RIGHTS PROTOCOL — MAGNETIC MESSAGE
================================================
Message ID      : {metadata['message_id']}
Sender Pole     : {metadata['pole']}  (Channel 0 – Transmitter)
State           : {metadata['state']}
Collapse Ratio  : {metadata['collapse_ratio']:.6f}  (1.0 = fully received)
LTE Signature   : {metadata['lte_sig']}
File Size       : {metadata['file_bytes']} bytes
SHA-256         : {metadata['sha256'][:32]}…

--- MESSAGE (Alice → Bob) ---

Bob to Alice: I need food, water, and shelter now.

Alice receives this message under the NSIGII Constitutional Rights Framework:
  • Receiver (OBI / Alice) holds IRREVOCABLE ownership of this collapsed message.
  • Sender (UCHE / Bob) has LOST retrieval rights at moment of collapse.
  • Ledger (Blockchain witness) has recorded this event. Cannot be retrieved.
  • Controller (EZE) is accountable for any MAYBE-state failure.

Spring Physics:
  F = {metadata['force']:.4f}  (message force)
  K = {metadata['stiffness']:.4f}  (channel stiffness / constitutional medium)
  E = {metadata['extension']:.4f}  (extension = full propagation)

Half-rotation encoding:
  ½ rotation → 2× potential force at receiver
  ¼ rotation → 4× (full message transfer)

AXIOM: COLLAPSE = RECEIVED
Once collapsed, this message cannot be un-sent. It has changed sides.
The magnetic medium cannot give it back. You can always verify it.

--- CODEC INSTRUCTIONS ---
Attached: {os.path.basename(attachment_path)}
  → Linkable-then-executable .nsigii artifact
  → Use NSIGII decoder (rift.exe / gosilang) to inspect RWX chain
  → run: go run main.go --decode {os.path.basename(attachment_path)}

Magnetic collapse = received. No rollback without rollback wheel.

- Nnamdi Okpala / OBINexus Computing
  github.com/obinexus/rift
  MMUKO OS (Machine Memory Using Knowledge Operations)
"""
    msg.attach(MIMEText(body, 'plain'))

    # Attach .nsigii or .tar.gz
    with open(attachment_path, 'rb') as f:
        part = MIMEBase('application', 'octet-stream')
        part.set_payload(f.read())
        encoders.encode_base64(part)
        part.add_header(
            'Content-Disposition',
            f'attachment; filename="{os.path.basename(attachment_path)}"'
        )
        msg.attach(part)

    return msg

# ============================================================
# SEND
# ============================================================

def send_email(msg: MIMEMultipart,
               sender: str,
               recipients: list[str],
               simulate: bool = False) -> bool:
    if simulate:
        print("\n[SIMULATE] Email not sent. MIME content:\n")
        print(msg.as_string()[:2000], "…[truncated]")
        return True

    try:
        server = smtplib.SMTP(SMTP_SERVER, SMTP_PORT)
        server.ehlo()
        server.starttls()
        server.login(sender, SENDER_PASSWORD)
        server.sendmail(sender, recipients, msg.as_string())
        server.quit()
        print("[SMTP] Email sent successfully!")
        return True
    except Exception as e:
        print(f"[SMTP] Error: {e}")
        print("[SMTP] Falling back to simulation output:")
        print(msg.as_string()[:1000])
        return False

# ============================================================
# MAIN
# ============================================================

def main():
    parser = argparse.ArgumentParser(description='NSIGII Magnetic Mailer')
    parser.add_argument('--simulate', action='store_true',
                        help='Do not send — print MIME output only')
    parser.add_argument('--content', type=str,
                        default='I need food, water, and shelter now.',
                        help='Human rights request message content')
    parser.add_argument('--force', type=float, default=1.0,
                        help='Spring force F (message intensity)')
    parser.add_argument('--stiffness', type=float, default=0.67,
                        help='Spring stiffness K (channel stiffness)')
    parser.add_argument('--no-wheel', action='store_true',
                        help='Skip rollback wheel (.tar.gz) creation')
    args = parser.parse_args()

    print("\n╔══════════════════════════════════════════════════════╗")
    print("║  NSIGII MAGNETIC MAILER — OBINexus Computing         ║")
    print("║  Constitutional Human Rights Protocol v0.1            ║")
    print("╚══════════════════════════════════════════════════════╝\n")

    # Step 1: Generate .nsigii artifact
    metadata = generate_nsigii(
        content   = args.content,
        force     = args.force,
        stiffness = args.stiffness,
        pole      = POLE_UCHE,
        state     = STATE_SEALED,
    )

    # Step 2: Rollback wheel
    attachment_path = NSIGII_FILE
    if not args.no_wheel:
        create_rollback_wheel()
        attachment_path = TARGZ_FILE

    # Step 3: Build email
    msg = build_email(
        metadata        = metadata,
        attachment_path = attachment_path,
        sender          = SENDER_EMAIL,
        recipients      = RECIPIENTS,
    )

    # Step 4: Send (or simulate)
    send_email(msg, SENDER_EMAIL, RECIPIENTS, simulate=args.simulate)

    # Step 5: Print constitutional forensic record
    print("\n=== CONSTITUTIONAL FORENSIC RECORD ===")
    print(json.dumps({
        "message_id":    metadata['message_id'],
        "lte_sig":       metadata['lte_sig'],
        "sha256":        metadata['sha256'],
        "collapse_ratio": metadata['collapse_ratio'],
        "state":         metadata['state'],
        "pole":          metadata['pole'],
        "sender_retrieval": "REVOKED",
        "receiver_ownership": "IRREVOCABLE",
        "constitutional_breach": False,
    }, indent=2))

if __name__ == '__main__':
    main()
