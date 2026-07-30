#!/usr/bin/env python3
"""
Fetch official EA Red Alert 3 XML files directly from GitHub:
electronicarts/CnC_Modding_Support (Red Alert 3 folder)
"""

import os
import sys
import json
import urllib.request

REPO_API_URL = "https://api.github.com/repos/electronicarts/CnC_Modding_Support/contents/Red%20Alert%203"
RAW_BASE_URL = "https://raw.githubusercontent.com/electronicarts/CnC_Modding_Support/main/Red%20Alert%203"
TARGET_DIR = os.path.join(os.path.dirname(__file__), "RA3_XML_Source")

def fetch_directory(path=""):
    url = f"{REPO_API_URL}/{path}" if path else REPO_API_URL
    req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
    try:
        with urllib.request.urlopen(req) as resp:
            items = json.loads(resp.read().decode('utf-8'))
            for item in items:
                item_name = item.get("name", "")
                item_type = item.get("type", "")
                item_path = item.get("path", "")
                rel_path = item_path.replace("Red Alert 3/", "").replace("Red Alert 3", "")
                local_dest = os.path.join(TARGET_DIR, rel_path)
                
                if item_type == "dir":
                    os.makedirs(local_dest, exist_ok=True)
                    fetch_directory(rel_path)
                elif item_type == "file" and (item_name.endswith(".xml") or item_name.endswith(".xsd") or item_name.endswith(".h")):
                    os.makedirs(os.path.dirname(local_dest), exist_ok=True)
                    print(f"Downloading {rel_path}...")
                    download_url = item.get("download_url")
                    if download_url:
                        with urllib.request.urlopen(download_url) as file_resp, open(local_dest, "wb") as out_f:
                            out_f.write(file_resp.read())
    except Exception as e:
        print(f"Error fetching {url}: {e}")

def main():
    os.makedirs(TARGET_DIR, exist_ok=True)
    print(f"[RA3 Fetcher] Fetching Red Alert 3 XML assets into {TARGET_DIR}...")
    fetch_directory("")
    print("[RA3 Fetcher] Fetch complete.")

if __name__ == "__main__":
    main()
