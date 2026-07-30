#!/usr/bin/env python3
"""
Fetch all official XML/XSD definitions across electronicarts/CnC_Modding_Support.
"""

import os
import sys
import json
import urllib.request

REPO_API_URL = "https://api.github.com/repos/electronicarts/CnC_Modding_Support/contents"
TARGET_DIR = os.path.join(os.path.dirname(__file__), "EA_Modding_Support_Files")

def fetch_tree(path=""):
    url = f"{REPO_API_URL}/{path}" if path else REPO_API_URL
    req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
    try:
        with urllib.request.urlopen(req) as resp:
            items = json.loads(resp.read().decode('utf-8'))
            if not isinstance(items, list):
                return
            for item in items:
                item_name = item.get("name", "")
                item_type = item.get("type", "")
                item_path = item.get("path", "")
                local_dest = os.path.join(TARGET_DIR, item_path)
                
                if item_type == "dir":
                    os.makedirs(local_dest, exist_ok=True)
                    fetch_tree(item_path)
                elif item_type == "file" and (item_name.endswith(".xml") or item_name.endswith(".xsd")):
                    os.makedirs(os.path.dirname(local_dest), exist_ok=True)
                    download_url = item.get("download_url")
                    if download_url:
                        print(f"Downloading {item_path}...")
                        with urllib.request.urlopen(download_url) as file_resp, open(local_dest, "wb") as out_f:
                            out_f.write(file_resp.read())
    except Exception as e:
        print(f"Error fetching {url}: {e}")

def main():
    os.makedirs(TARGET_DIR, exist_ok=True)
    print(f"[EA Fetcher] Downloading XML/XSD schemas from electronicarts/CnC_Modding_Support...")
    fetch_tree("")
    print("[EA Fetcher] Done.")

if __name__ == "__main__":
    main()
