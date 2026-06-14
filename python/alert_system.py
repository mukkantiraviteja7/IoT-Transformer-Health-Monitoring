"""
Alert System — Email notifications for transformer faults
Author: Mukkanti Ravi Teja
"""

import smtplib
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from datetime import datetime

# ── Email Configuration ───────────────────────────────────────────────────────
SENDER_EMAIL    = "your_email@gmail.com"      # Your Gmail
SENDER_PASSWORD = "your_app_password"         # Gmail App Password
RECEIVER_EMAIL  = "mukkantiraviteja7@gmail.com"

last_alert_time = 0
ALERT_COOLDOWN  = 300  # 5 minutes between emails

def send_alert(fault_message, sensor_data):
    """Send email alert when a fault is detected."""
    global last_alert_time
    now = datetime.now().timestamp()

    if now - last_alert_time < ALERT_COOLDOWN:
        return  # Prevent spam

    try:
        msg = MIMEMultipart("alternative")
        msg["Subject"] = f"⚡ TRANSFORMER ALERT: {fault_message}"
        msg["From"]    = SENDER_EMAIL
        msg["To"]      = RECEIVER_EMAIL

        body = f"""
        <html><body style='font-family:Arial;'>
        <h2 style='color:red;'>⚠️ Transformer Fault Detected</h2>
        <p><b>Fault:</b> {fault_message}</p>
        <p><b>Time:</b> {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
        <hr>
        <h3>Current Readings:</h3>
        <table border='1' cellpadding='8'>
          <tr><th>Parameter</th><th>Value</th></tr>
          <tr><td>🌡️ Oil Temperature</td><td>{sensor_data.get('oil_temp')} °C</td></tr>
          <tr><td>⚡ Load Current</td><td>{sensor_data.get('current')} A</td></tr>
          <tr><td>🔌 Voltage</td><td>{sensor_data.get('voltage')} V</td></tr>
          <tr><td>💧 Oil Level</td><td>{sensor_data.get('oil_level')} %</td></tr>
          <tr><td>💨 Humidity</td><td>{sensor_data.get('humidity')} %</td></tr>
          <tr><td>☢️ Gas Level</td><td>{sensor_data.get('gas_level')} ppm</td></tr>
        </table>
        <p style='color:gray;'>— IoT Transformer Health Monitor</p>
        </body></html>
        """
        msg.attach(MIMEText(body, "html"))

        with smtplib.SMTP_SSL("smtp.gmail.com", 465) as server:
            server.login(SENDER_EMAIL, SENDER_PASSWORD)
            server.sendmail(SENDER_EMAIL, RECEIVER_EMAIL, msg.as_string())

        last_alert_time = now
        print(f"📧 Alert email sent: {fault_message}")

    except Exception as e:
        print(f"⚠️  Email failed: {e}")