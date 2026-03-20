import smtplib
import tarfile
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from email.mime.base import MIMEBase
from email import encoders
import os

# Config: Replace with your real SMTP details for testing
SMTP_SERVER = 'smtp.gmail.com'  # For Gmail; use 'localhost' for local testing with smtp4dev
SMTP_PORT = 587  # TLS port
SENDER_EMAIL = 'okpalan86@gmail.com'  # Your sender email
SENDER_PASSWORD = 'your_app_password'  # Gmail App Password (not regular password)
RECIPIENTS = ['alice@somefakeemail.com', 'bob@somefakeemail.com']  # Fake for demo; use real for testing

# NSIGII file paths
NSIGII_FILE = 'demo.nsigii'  # Your encoded .nsigii file
TARGZ_FILE = 'demo.nsigii.tar.gz'  # Optional tar.gz for rollback wheel

# Step 1: Optionally create tar.gz archive (for protocol integrity)
if not os.path.exists(TARGZ_FILE):
    with tarfile.open(TARGZ_FILE, 'w:gz') as tar:
        tar.add(NSIGII_FILE)
    print(f"Created {TARGZ_FILE} for rollback wheel.")

# Step 2: Prepare email
msg = MIMEMultipart()
msg['From'] = SENDER_EMAIL
msg['To'] = ', '.join(RECIPIENTS)
msg['Subject'] = 'NSIGII Human Rights Request: I need food, water, and shelter now'

# Body: Alice/Bob style message from transcripts
body = """
Dear Alice and Bob,

This is a demo under the NSIGII Human Rights Protocol.
I need food, water, and shelter now.

Attached is the verified .nsigii artifact (or .tar.gz wrapper) for consensual codec verification.
Use the NSIGII decoder to inspect RWX chain and discriminant consensus.

Magnetic collapse = received. No rollback without wheel.

- Bob
"""
msg.attach(MIMEText(body, 'plain'))

# Attach the file (.tar.gz or raw .nsigii)
attachment_path = TARGZ_FILE if os.path.exists(TARGZ_FILE) else NSIGII_FILE
with open(attachment_path, 'rb') as attachment:
    part = MIMEBase('application', 'octet-stream')
    part.set_payload(attachment.read())
    encoders.encode_base64(part)
    part.add_header('Content-Disposition', f'attachment; filename={os.path.basename(attachment_path)}')
    msg.attach(part)

# Step 3: Send email (or simulate)
try:
    server = smtplib.SMTP(SMTP_SERVER, SMTP_PORT)
    server.starttls()
    server.login(SENDER_EMAIL, SENDER_PASSWORD)
    server.sendmail(SENDER_EMAIL, RECIPIENTS, msg.as_string())
    server.quit()
    print("Email sent successfully!")
except Exception as e:
    print(f"Error sending email: {e}")
    print("Simulated email content:")
    print(msg.as_string())  # Print for demo if send fails
