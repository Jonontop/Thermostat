from flask import Flask, jsonify, request
from flask_cors import CORS
import sqlite3
from datetime import datetime

app = Flask(__name__)
CORS(app)

DB_PATH = "thermostat.db"

def get_db():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

def init_db():
    db = get_db()
    db.execute("""
        CREATE TABLE IF NOT EXISTS thermostat_state (
            id INTEGER PRIMARY KEY,
            is_on INTEGER DEFAULT 1,
            current_temp REAL DEFAULT 21.0,
            target_temp REAL DEFAULT 22.0,
            updated_at TEXT
        )
    """)
    db.execute("""
        CREATE TABLE IF NOT EXISTS thermostat_log (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            is_on INTEGER,
            current_temp REAL,
            target_temp REAL,
            recorded_at TEXT
        )
    """)
    if db.execute("SELECT COUNT(*) FROM thermostat_state").fetchone()[0] == 0:
        db.execute("INSERT INTO thermostat_state (id, is_on, current_temp, target_temp, updated_at) VALUES (1, 1, 21.0, 22.0, ?)", (datetime.utcnow().isoformat(),))
    db.commit()
    db.close()

@app.route("/api/state", methods=["GET"])
def get_state():
    db = get_db()
    row = db.execute("SELECT * FROM thermostat_state WHERE id = 1").fetchone()
    db.close()
    return jsonify(dict(row))

@app.route("/api/state", methods=["POST"])
def update_state():
    data = request.get_json(force=True)
    db = get_db()
    row = db.execute("SELECT * FROM thermostat_state WHERE id = 1").fetchone()
    
    db_target = float(row["target_temp"])
    db_is_on = int(row["is_on"])

    # 1. Determine Target Temp
    if "target_temp" in data: # Request from WEB
        final_target = float(data["target_temp"])
    elif "target" in data: # Request from ESP32
        esp_val = float(data["target"])
        # If ESP value differs significantly from DB, the Knob was turned
        final_target = esp_val if abs(esp_val - db_target) > 0.1 else db_target
    else:
        final_target = db_target

    # 2. Get Actual Temp (ESP only)
    final_actual = data.get("temperature", row["current_temp"])

    # 3. Determine Power
    if "is_on" in data:
        final_is_on = 1 if data["is_on"] else 0
    elif "relay_on" in data:
        final_is_on = 1 if data["relay_on"] else 0
    else:
        final_is_on = db_is_on

    now = datetime.utcnow().isoformat()
    db.execute("UPDATE thermostat_state SET is_on=?, current_temp=?, target_temp=?, updated_at=? WHERE id=1",
               (final_is_on, final_actual, final_target, now))
    db.execute("INSERT INTO thermostat_log (is_on, current_temp, target_temp, recorded_at) VALUES (?, ?, ?, ?)",
               (final_is_on, final_actual, final_target, now))
    db.commit()
    db.close()

    return jsonify({"status": "success", "target_temp": final_target, "is_on": bool(final_is_on)})

@app.route("/api/history", methods=["GET"])
def get_history():
    """Return last N log entries (default 168 = 7 days hourly)."""
    limit = request.args.get("limit", 168, type=int)
    db    = get_db()
    
    # Changed 'temperature' to 'current_temp' to match your database columns
    rows  = db.execute(
        "SELECT is_on, current_temp, recorded_at FROM thermostat_log ORDER BY recorded_at DESC LIMIT ?",
        (limit,)
    ).fetchall()
    db.close()

    history = [
        {
            "is_on":        bool(r["is_on"]),
            "current_temp": r["current_temp"],
            "recorded_at":  r["recorded_at"],
        }
        for r in reversed(rows)
    ]
    return jsonify(history)


if __name__ == "__main__":
    init_db()
    app.run(debug=True, port=5000, host="0.0.0.0")
