#!/usr/bin/env python3
"""Send Python to the Unreal editor over its built-in MCP HTTP bridge.

The bridge needs an `initialize` handshake to hand back an Mcp-Session-Id, which
every later `tools/call` must carry.

The 1800s default matters: the first version used 60s, and any pass that made
more than a few hundred RPCs (layer painting, foliage scatter) had curl cut the
connection mid-operation. The editor kept working but the result was lost, which
looked like a silent failure rather than a timeout.
"""
import json
import subprocess
import sys

URL = "http://127.0.0.1:8000/mcp"


def get_session():
    payload = json.dumps({
        "jsonrpc": "2.0", "id": 0, "method": "initialize",
        "params": {"protocolVersion": "2024-11-05", "capabilities": {},
                   "clientInfo": {"name": "claude", "version": "1.0"}},
    })
    result = subprocess.run(
        ["curl", "-s", "-m", "30", "-X", "POST", URL,
         "-H", "Content-Type: application/json",
         "-d", payload, "-D", "/dev/stderr"],
        capture_output=True, text=True,
    )
    for line in result.stderr.split("\n"):
        if "Mcp-Session-Id" in line:
            return line.split(":", 1)[1].strip()
    return None


def run_code(session_id, code, timeout=1800):
    payload = json.dumps({
        "jsonrpc": "2.0", "id": 1, "method": "tools/call",
        "params": {"name": "execute_python_code", "arguments": {"code": code}},
    })
    result = subprocess.run(
        ["curl", "-s", "-m", str(timeout), "-X", "POST", URL,
         "-H", "Content-Type: application/json",
         "-H", f"Mcp-Session-Id: {session_id}",
         "-d", payload],
        capture_output=True, text=True,
    )
    out = result.stdout.strip()
    if not out:
        return {"error": {"message": f"empty response (curl rc={result.returncode}); "
                                     "editor may have died or the call timed out"}}
    try:
        return json.loads(out)
    except json.JSONDecodeError:
        return {"error": {"message": "non-JSON response: " + out[:300]}}


def main():
    session = get_session()
    if not session:
        print("ERROR: no MCP session; is the editor running with the bridge on :8000?")
        return 1

    response = run_code(session, sys.stdin.read())

    if "result" in response and "content" in response["result"]:
        for item in response["result"]["content"]:
            if item.get("type") != "text":
                continue
            try:
                inner = json.loads(item["text"])
            except json.JSONDecodeError:
                print(item["text"])
                continue
            if inner.get("success"):
                print(inner.get("output", ""))
            else:
                print("ERROR: " + str(inner.get("error_message", "unknown")))
                return 1
    elif "error" in response:
        print("MCP ERROR: " + str(response["error"].get("message", response["error"])))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
