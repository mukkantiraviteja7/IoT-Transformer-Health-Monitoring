"""
IoT Transformer Health Monitoring — Flask Web Dashboard
Author: Mukkanti Ravi Teja
Run:   python dashboard.py
Open:  http://localhost:5000
"""

from flask import Flask, render_template, jsonify, send_file
import serial
import threading
import json
import time
import csv
import os
from datetime import datetime
from alert_system import send_alert

app = Flask(__name__)

# ── Configuration ─────────────────────────────────────────────────────────────
SERIAL_PORT   = "COM3"      # Change to your port (Linux: /dev/ttyUSB0)
BAUD_RATE     = 115200
LOG_FILE      = "data/sensor_log.csv"
MAX_HISTORY   = 50          # Points kept in memory for charts

# ── Shared State ──────────────────────────────────────────────────────────────
latest_data = {
    "oil_temp": 0, "current": 0, "voltage": 0,
    "oil_level": 0, "humidity": 0, "gas_level": 0,
    "fault": False, "fault_msg": "Normal", "uptime": 0,
    "timestamp": ""
}
history = {"timestamps": [], "oil_temp": [], "current": [], "voltage": [], "oil_level": []}
alert_log = []

# ── CSV Logger ────────────────────────────────────────────────────────────────
def init_csv():
    os.makedirs("data", exist_ok=True)
    if not os.path.exists(LOG_FILE):
        with open(LOG_FILE, "w", newline="") as f:
            csv.writer(f).writerow([
                "timestamp","oil_temp","current","voltage",
                "oil_level","humidity","gas_level","status"
            ])

def log_to_csv(d):
    with open(LOG_FILE, "a", newline="") as f:
        csv.writer(f).writerow([
            d["timestamp"], d["oil_temp"], d["current"], d["voltage"],
            d["oil_level"], d["humidity"], d["gas_level"],
            "FAULT" if d["fault"] else "NORMAL"
        ])

# ── Serial Reader Thread ──────────────────────────────────────────────────────
def serial_reader():
    global latest_data, history, alert_log
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
        print(f"✅ Connected to {SERIAL_PORT}")
    except Exception as e:
        print(f"⚠️  Serial error: {e} — using demo data")
        demo_mode()
        return

    while True:
        try:
            line = ser.readline().decode("utf-8").strip()
            if line.startswith("{"):
                d = json.loads(line)
                d["timestamp"] = datetime.now().strftime("%H:%M:%S")
                latest_data.update(d)

                # Update history
                ts = d["timestamp"]
                for key in ["oil_temp","current","voltage","oil_level"]:
                    history[key].append(d.get(key, 0))
                history["timestamps"].append(ts)
                if len(history["timestamps"]) > MAX_HISTORY:
                    for k in history:
                        history[k] = history[k][-MAX_HISTORY:]

                # Alerts
                if d.get("fault"):
                    msg = d.get("fault_msg", "Unknown fault")
                    alert_log.append({"time": ts, "message": msg})
                    alert_log[:] = alert_log[-20:]
                    send_alert(msg, d)

                log_to_csv(d)
        except Exception as e:
            time.sleep(1)

# ── Demo Mode (no hardware) ───────────────────────────────────────────────────
def demo_mode():
    import random, math
    global latest_data, history
    t = 0
    while True:
        t += 1
        d = {
            "oil_temp":  round(65 + 10 * math.sin(t * 0.1) + random.uniform(-1,1), 1),
            "current":   round(15 + 5  * math.sin(t * 0.05)+ random.uniform(-0.5,0.5), 1),
            "voltage":   round(220 + 5 * math.cos(t * 0.08)+ random.uniform(-2,2), 1),
            "oil_level": round(85 - t  * 0.01 + random.uniform(-0.2,0.2), 1),
            "humidity":  round(55 + 8  * math.sin(t * 0.07)+ random.uniform(-1,1), 1),
            "gas_level": round(50  + random.uniform(0,30), 0),
            "fault":     False, "fault_msg": "Normal", "uptime": t * 5,
            "timestamp": datetime.now().strftime("%H:%M:%S")
        }
        if d["oil_temp"] > 80: d["fault"] = True; d["fault_msg"] = "WARNING: High Temp"
        latest_data.update(d)
        for key in ["oil_temp","current","voltage","oil_level"]:
            history[key].append(d[key])
        history["timestamps"].append(d["timestamp"])
        if len(history["timestamps"]) > MAX_HISTORY:
            for k in history: history[k] = history[k][-MAX_HISTORY:]
        log_to_csv(d)
        time.sleep(5)

# ── Flask Routes ──────────────────────────────────────────────────────────────
@app.route("/")
def index():
    return render_template("index.html")

@app.route("/api/data")
def api_data():
    return jsonify(latest_data)

@app.route("/api/history")
def api_history():
    return jsonify(history)

@app.route("/api/alerts")
def api_alerts():
    return jsonify(alert_log)

@app.route("/api/download")
def api_download():
    return send_file(LOG_FILE, as_attachment=True, download_name="transformer_data.csv")

# ── Main ──────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    init_csv()
    thread = threading.Thread(target=serial_reader, daemon=True)
    thread.start()
    print("🚀 Dashboard running at http://localhost:5000")
    app.run(host="0.0.0.0", port=5000, debug=False)