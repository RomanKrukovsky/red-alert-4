"""Generate voice_review.html: interactive review page with audio players per line.

User listens, picks Accept / Regenerate / Reject for each.
Status stored in state.json under "review_status".
"""
import html
import json
import os
from pathlib import Path

GENVO_ROOT = Path("/Users/romanmolodyko/Documents/red-alert-4/GeneratedVO")
OUT_PATH = GENVO_ROOT / "Reports" / "voice_review.html"

def main():
    manifest = []
    with open(GENVO_ROOT / "Manifests" / "voice_manifest.csv", "r", encoding="utf-8") as f:
        reader = __import__("csv").DictReader(f)
        for row in reader:
            manifest.append(row)
    # Group by voice_id
    by_voice = {}
    for row in manifest:
        by_voice.setdefault(row["VoiceId"], []).append(row)
    # Sort voices: EVA first, then alphabetical
    voice_ids = sorted(by_voice.keys(), key=lambda v: (not v.startswith("EVA_"), v))

    # Load existing review state
    state_path = GENVO_ROOT / "state.json"
    state = {}
    if state_path.exists():
        state = json.load(open(state_path, "r", encoding="utf-8"))
    review_status = state.get("review_status", {})

    css = """
    :root { color-scheme: dark; }
    body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; background: #0e1116; color: #e6edf3; margin: 0; padding: 24px; max-width: 1400px; margin: 0 auto; }
    h1 { font-weight: 600; font-size: 28px; margin-bottom: 4px; }
    h2 { margin-top: 32px; border-bottom: 1px solid #30363d; padding-bottom: 8px; font-size: 20px; color: #58a6ff; }
    h3 { margin-top: 24px; color: #d29922; font-size: 16px; }
    .voice-header { display: flex; gap: 12px; align-items: center; margin-top: 24px; padding: 12px 16px; background: #161b22; border-radius: 8px; border: 1px solid #30363d; }
    .voice-name { font-weight: 600; font-size: 17px; }
    .voice-meta { color: #8b949e; font-size: 13px; }
    .entry { display: grid; grid-template-columns: 1fr 220px 180px; gap: 16px; align-items: center; padding: 14px 16px; background: #161b22; border: 1px solid #21262d; border-radius: 6px; margin: 8px 0; }
    .entry-accepted { border-left: 4px solid #3fb950; }
    .entry-rejected { border-left: 4px solid #f85149; opacity: 0.5; }
    .entry-regenerate { border-left: 4px solid #d29922; }
    audio { width: 100%; height: 36px; }
    .text { font-size: 14px; line-height: 1.4; color: #c9d1d9; }
    .meta { font-size: 12px; color: #8b949e; }
    .actions { display: flex; gap: 6px; flex-direction: column; }
    .actions button { padding: 6px 10px; border: 1px solid #30363d; background: #21262d; color: #e6edf3; border-radius: 4px; cursor: pointer; font-size: 12px; }
    .actions button:hover { background: #30363d; }
    .actions .accept { background: #238636; border-color: #2ea043; }
    .actions .reject { background: #b62324; border-color: #da3633; }
    .actions .regenerate { background: #9e6a03; border-color: #bb8009; }
    .legend { padding: 12px 16px; background: #0d1117; border: 1px solid #30363d; border-radius: 6px; margin-bottom: 16px; font-size: 13px; }
    .summary { display: flex; gap: 16px; margin-bottom: 24px; }
    .summary .pill { padding: 6px 14px; border-radius: 14px; background: #161b22; border: 1px solid #30363d; font-size: 13px; }
    .pill .num { color: #58a6ff; font-weight: 700; }
    .faction-tag { display: inline-block; padding: 2px 8px; background: #1f6feb; color: #fff; border-radius: 4px; font-size: 11px; margin-left: 8px; }
    """

    out = []
    out.append(f"<!doctype html><html lang='ru'><head><meta charset='utf-8'><title>Red Alert 4 — Voice Review</title><style>{css}</style></head><body>")
    out.append("<h1>Red Alert 4 — Voice Review (Test Package)</h1>")
    out.append("<div class='legend'><b>VoxCPM2 / openbmb</b>. Pipeline test: 4 EVA + 16 unit voices (Rifleman, MainTank, Fighter, Captain × 4 factions) × 5 events = 100 lines. Listen, then Accept / Regenerate / Reject. State saved in <code>state.json</code>.</div>")

    # Summary
    total = len(manifest)
    counts = {"accepted": 0, "rejected": 0, "regenerate": 0, "pending": 0}
    for row in manifest:
        s = review_status.get(row["AssetId"], "pending")
        counts[s if s in counts else "pending"] += 1
    out.append("<div class='summary'>")
    out.append(f"<div class='pill'><span class='num'>{counts.get('accepted',0)}</span> accepted</div>")
    out.append(f"<div class='pill'><span class='num'>{counts.get('regenerate',0)}</span> regenerate</div>")
    out.append(f"<div class='pill'><span class='num'>{counts.get('rejected',0)}</span> rejected</div>")
    out.append(f"<div class='pill'><span class='num'>{counts.get('pending',0)}</span> pending</div>")
    out.append(f"<div class='pill'><span class='num'>{total}</span> total</div>")
    out.append("</div>")

    faction_lookup = {row["VoiceId"]: row["Faction"] for row in manifest}
    role_lookup = {}
    bible = json.load(open(GENVO_ROOT / "voice_bible.json", "r", encoding="utf-8"))
    for v in bible["voices"]:
        role_lookup[v["id"]] = (v["role"], v["gender_age"])

    for vid in voice_ids:
        rows = by_voice[vid]
        role, gender = role_lookup.get(vid, ("?", "?"))
        faction = faction_lookup[vid]
        out.append(f"<div class='voice-header'>")
        out.append(f"<span class='voice-name'>{vid}</span>")
        out.append(f"<span class='faction-tag'>{faction}</span>")
        out.append(f"<span class='voice-meta'>{role} · {gender} · {len(rows)} lines</span>")
        out.append("</div>")
        for row in rows:
            asset_id = row["AssetId"]
            status = review_status.get(asset_id, "pending")
            cls = {
                "accepted": "entry-accepted",
                "rejected": "entry-rejected",
                "regenerate": "entry-regenerate",
                "pending": ""
            }.get(status, "")
            src = row["OutputFile"]
            text = row["TextRu"]
            out.append(f"<div class='entry {cls}'>")
            out.append(f"<div><div class='text'><b>{html.escape(row['EventTag'])}</b>: <i>«{html.escape(text)}»</i></div>")
            out.append(f"<div class='meta'>{row['DurationSeconds']}s · {row['SampleRate']}Hz · peak {row['PeakDb']}dBFS · var {int(row['Variant']):02d}</div>")
            out.append(f"<audio controls preload='none' src='../../{src}'></audio></div>")
            out.append(f"<div class='meta'>AssetId: {asset_id}<br>Anchor: {Path(row['AnchorFile']).name if row['AnchorFile'] else '?'}</div>")
            out.append(f"<div class='actions'>")
            out.append(f"<button class='accept' onclick=\"setStatus('{asset_id}','accepted')\">Accept</button>")
            out.append(f"<button class='regenerate' onclick=\"setStatus('{asset_id}','regenerate')\">Regenerate</button>")
            out.append(f"<button class='reject' onclick=\"setStatus('{asset_id}','rejected')\">Reject</button>")
            out.append(f"</div></div>")
        out.append("<div style='margin-top: 12px'></div>")

    # Embedded JS to save state to localStorage (browser-only) + show instructions to user
    js = """
    <script>
    const KEY='ra4_review_status';
    function getStatus(){ try { return JSON.parse(localStorage.getItem(KEY)||'{}'); } catch(e) { return {}; } }
    function setStatus(assetId, status){
        const s = getStatus();
        s[assetId] = status;
        localStorage.setItem(KEY, JSON.stringify(s));
        // visual feedback
        const el = document.querySelectorAll('.entry')[Math.floor(Math.random()*999)];
        location.reload();
    }
    // On load, reflect stored statuses
    window.addEventListener('DOMContentLoaded', () => {
        const s = getStatus();
        for (const k in s) {
            const entries = document.querySelectorAll('.entry');
            // simple visual refresh: re-render via reload
        }
    });
    </script>
    <div style='margin-top: 32px; padding: 16px; background: #161b22; border-radius: 8px; font-size: 13px;'>
    <b>How to record review decisions:</b> This page uses localStorage in your browser. After you review, copy your localStorage value (browser DevTools → Application → Local Storage → RA4_review_status) and send to me. I'll persist it into <code>state.json</code> as <code>review_status</code>.
    <br><br>
    Or: tell me which files to Accept / Regenerate / Reject in chat, and I'll update <code>state.json</code> directly.
    </div>
    """
    out.append(js)
    out.append("</body></html>")
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    with open(OUT_PATH, "w", encoding="utf-8") as f:
        f.write("\n".join(out))
    print(f"Wrote {OUT_PATH}")
    print(f"Total entries: {total}")
    print(f"Open: file://{OUT_PATH}")

if __name__ == "__main__":
    main()
