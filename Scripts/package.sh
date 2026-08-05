#!/bin/bash
# package.sh — Build, cook, and package RA4 as a distributable Shipping build.
#
# Usage:
#   ./Scripts/package.sh              # auto-detect UE, build for current platform
#   ./Scripts/package.sh --ue /path   # explicit UE root
#   ./Scripts/package.sh --clean      # clean DerivedDataCache before build
#   ./Scripts/package.sh --server     # build headless dedicated server only
#
# Output: Build/Staging/RA4-<version>/

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UPROJECT="$REPO_ROOT/RedAlert4.uproject"
PROJECT_NAME="RedAlert4"
VERSION="0.1.0-alpha"

# --- Defaults ---
UE_ROOT=""
CLEAN_BUILD=false
SERVER_ONLY=false

# --- Parse args ---
while [[ $# -gt 0 ]]; do
    case "$1" in
        --ue)    UE_ROOT="$2"; shift 2 ;;
        --clean) CLEAN_BUILD=true; shift ;;
        --server) SERVER_ONLY=true; shift ;;
        --help|-h)
            echo "Usage: $0 [--ue /path/to/UE] [--clean] [--server]"
            exit 0
            ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

# --- Locate Unreal Engine ---
if [[ -z "$UE_ROOT" ]]; then
    # Standard macOS locations
    for candidate in \
        "$HOME/UnrealEngine" \
        "/Users/Shared/Epic Games/UE_5.8" \
        "/Users/Shared/Epic Games/UE_5.6" \
        "$HOME/Library/Application Support/Epic Games/Launcher/Engine"; do
        if [[ -f "$candidate/Engine/Build/BatchFiles/RunUAT.sh" ]]; then
            UE_ROOT="$candidate"
            break
        fi
    done
fi

if [[ -z "$UE_ROOT" ]]; then
    echo "ERROR: Cannot find Unreal Engine. Pass --ue /path/to/UE"
    exit 1
fi

UAT="$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh"
if [[ ! -f "$UAT" ]]; then
    echo "ERROR: RunUAT.sh not found at $UAT"
    exit 1
fi

echo "=== RA4 Packaging Script ==="
echo "UE root:  $UE_ROOT"
echo "Project:  $UPROJECT"
echo "Version:  $VERSION"
echo ""

# --- Output ---
STAGING="$REPO_ROOT/Build/Staging/RA4-$VERSION"
mkdir -p "$STAGING"

# --- Clean if requested ---
if [[ "$CLEAN_BUILD" == "true" ]]; then
    echo "Cleaning DerivedDataCache..."
    rm -rf "$REPO_ROOT/DerivedDataCache"
    rm -rf "$REPO_ROOT/Intermediate"
fi

# --- Platform detection ---
PLATFORM=$(uname -s)
case "$PLATFORM" in
    Darwin) PLATFORM_NAME="Mac" ;;
    Linux)  PLATFORM_NAME="Linux" ;;
    *)      PLATFORM_NAME="Win64" ;;
esac

echo "Target platform: $PLATFORM_NAME"

# --- Build ---
if [[ "$SERVER_ONLY" == "true" ]]; then
    echo ""
    echo "Building headless dedicated server (Shipping)..."
    "$UAT" BuildCookRun \
        -project="$UPROJECT" \
        -noP4 \
        -platform="$PLATFORM_NAME" \
        -clientconfig=Shipping \
        -serverconfig=Shipping \
        -server \
        -noclient \
        -cook \
        -build \
        -stage \
        -stagingdirectory="$STAGING/Server" \
        -archive \
        -archivedirectory="$STAGING/Server" \
        -pak \
        -prereqs \
        -compressed \
        -nodebuginfo
else
    echo ""
    echo "Building game client (Shipping)..."
    "$UAT" BuildCookRun \
        -project="$UPROJECT" \
        -noP4 \
        -platform="$PLATFORM_NAME" \
        -clientconfig=Shipping \
        -cook \
        -build \
        -stage \
        -stagingdirectory="$STAGING/Client" \
        -archive \
        -archivedirectory="$STAGING/Client" \
        -pak \
        -prereqs \
        -compressed \
        -nodebuginfo \
        -utf8output
fi

EXIT_CODE=$?

echo ""
if [[ $EXIT_CODE -eq 0 ]]; then
    echo "=== Build succeeded ==="
    echo "Output: $STAGING"
    du -sh "$STAGING"
    echo ""
    echo "To run:"
    if [[ "$SERVER_ONLY" == "true" ]]; then
        echo "  $STAGING/Server/$(ls "$STAGING/Server/" | head -1)/Binaries/$PLATFORM_NAME/"
    else
        echo "  open $STAGING/Client/$(ls "$STAGING/Client/" | head -1)/Mac/$(ls "$STAGING/Client/$(ls "$STAGING/Client/" | head -1)/Mac/" 2>/dev/null | head -1).app"
    fi
else
    echo "=== Build FAILED (exit code $EXIT_CODE) ==="
    echo "Check the log above for errors."
fi

exit $EXIT_CODE
