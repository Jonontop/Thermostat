from flask import Flask, jsonify, request
from flask_cors import CORS
import sqlite3
import os
from datetime import datetime, timedelta
import random

app = Flask(__name__)
CORS(app)  # Allow frontend to call the API

DB_PATH = "thermostat.db"

# Database Setup 

def get_db():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

def init_db():
    """Create tables and seed demo data if the database doesn't exist."""
    db = get_db()
    cursor = db.cursor()

    cursor.execute("""
        CREATE TABLE IF NOT EXISTS thermostat_state (
            id          INTEGER PRIMARY KEY,
            is_on       INTEGER NOT NULL DEFAULT 1,
            temperature REAL    NOT NULL DEFAULT 21.0,
            updated_at  TEXT    NOT NULL
        )
    """)

    cursor.execute("""
        CREATE TABLE IF NOT EXISTS thermostat_log (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            is_on       INTEGER NOT NULL,
            temperature REAL    NOT NULL,
            recorded_at TEXT    NOT NULL
        )
    """)

    # Seed initial state if empty
    cursor.execute("SELECT COUNT(*) FROM thermostat_state")
    if cursor.fetchone()[0] == 0:
        cursor.execute(
            "INSERT INTO thermostat_state (id, is_on, temperature, updated_at) VALUES (1, 1, 21.0, ?)",
            (datetime.utcnow().isoformat(),)
        )

    # Seed 7 days of demo history if empty
    cursor.execute("SELECT COUNT(*) FROM thermostat_log")
    if cursor.fetchone()[0] == 0:
        now = datetime.utcnow()
        entries = []
        for hours_ago in range(168, 0, -1):   # 7 days × 24 hours
            ts = now - timedelta(hours=hours_ago)
            hour = ts.hour
            # Simulate realistic on/off pattern: off at night (23–6), on during day
            is_on = 0 if 23 <= hour or hour < 6 else 1
            temp = round(random.uniform(19.0, 24.0) if is_on else random.uniform(15.0, 18.0), 1)
            entries.append((is_on, temp, ts.isoformat()))

        cursor.executemany(
            "INSERT INTO thermostat_log (is_on, temperature, recorded_at) VALUES (?, ?, ?)",
            entries
        )

    db.commit()
    db.close()


# Routes

@app.route("/api/state", methods=["GET"])
def get_state():
    """Return current thermostat state."""
    db = get_db()
    row = db.execute("SELECT * FROM thermostat_state WHERE id = 1").fetchone()
    db.close()
    return jsonify({
        "is_on":       bool(row["is_on"]),
        "temperature": row["temperature"],
        "updated_at":  row["updated_at"],
    })


@app.route("/api/state", methods=["POST"])
def update_state():
    """Update thermostat state (is_on and/or temperature)."""
    data = request.get_json(force=True)
    db   = get_db()

    current = db.execute("SELECT * FROM thermostat_state WHERE id = 1").fetchone()
    is_on   = int(data.get("is_on",       current["is_on"]))
    temp    = float(data.get("temperature", current["temperature"]))
    temp    = max(
        10.0, min(35.0, temp))      # clamp to safe range
    now     = datetime.utcnow().isoformat()

    db.execute(
        "UPDATE thermostat_state SET is_on = ?, temperature = ?, updated_at = ? WHERE id = 1",
        (is_on, temp, now)
    )
    # Log the change
    db.execute(
        "INSERT INTO thermostat_log (is_on, temperature, recorded_at) VALUES (?, ?, ?)",
        (is_on, temp, now)
    )
    db.commit()
    db.close()

    return jsonify({"is_on": bool(is_on), "temperature": temp, "updated_at": now})


@app.route("/api/history", methods=["GET"])
def get_history():
    """Return last N log entries (default 168 = 7 days hourly)."""
    limit = request.args.get("limit", 168, type=int)
    db    = get_db()
    rows  = db.execute(
        "SELECT is_on, temperature, recorded_at FROM thermostat_log ORDER BY recorded_at DESC LIMIT ?",
        (limit,)
    ).fetchall()
    db.close()

    history = [
        {
            "is_on":       bool(r["is_on"]),
            "temperature": r["temperature"],
            "recorded_at": r["recorded_at"],
        }
        for r in reversed(rows)
    ]
    return jsonify(history)


# Entry Point

if __name__ == "__main__":
    init_db()
    print(" Thermostat API running at http://localhost:5000")
    app.run(debug=True, port=5000)
