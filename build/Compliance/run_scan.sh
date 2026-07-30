#!/bin/bash
set -euo pipefail
python3 "$(dirname "$0")/compliance_scan.py" --scope both
