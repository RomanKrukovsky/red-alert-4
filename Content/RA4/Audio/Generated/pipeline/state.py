"""Pipeline state tracking for resume capability."""
import json
import os
import time
from pathlib import Path

STATE_PATH = "/Users/romanmolodyko/Documents/red-alert-4/GeneratedVO/state.json"

def load_state():
    if os.path.exists(STATE_PATH):
        with open(STATE_PATH, "r", encoding="utf-8") as f:
            return json.load(f)
    return {
        "phase": "init",
        "completed": [],
        "failed": [],
        "review_status": {},
        "last_processed": None,
        "timestamp": time.time()
    }

def save_state(state):
    state["timestamp"] = time.time()
    Path(STATE_PATH).parent.mkdir(parents=True, exist_ok=True)
    with open(STATE_PATH, "w", encoding="utf-8") as f:
        json.dump(state, f, ensure_ascii=False, indent=2)

def mark_completed(state, asset_id):
    if asset_id not in state["completed"]:
        state["completed"].append(asset_id)
    state["last_processed"] = asset_id
    save_state(state)

def mark_failed(state, asset_id, reason, retries):
    state["failed"].append({
        "asset": asset_id,
        "reason": reason,
        "retries": retries
    })
    save_state(state)

def is_completed(state, asset_id):
    return asset_id in state["completed"]
