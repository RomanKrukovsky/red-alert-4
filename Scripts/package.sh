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
    set +e
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
    EXIT_CODE=$?
    set -e
else
    echo ""
    echo "Building game client (Shipping)..."
    set +e
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
    EXIT_CODE=$?
    set -e
fi

# The UAT calls above are wrapped in `set +e` / `set -e` and their status is
# captured immediately. Without that, `set -e` aborts the script the moment UAT
# fails, so the "Build FAILED" branch never runs and the script dies before
# reporting anything - the failure mode that let an empty RedAlert4.app be treated
# as a successful package.
echo ""
if [[ $EXIT_CODE -eq 0 ]]; then
    # UAT exiting 0 is not proof that a runnable artifact exists. Binaries/Mac/
    # RedAlert4.app in this repo is a single empty Contents/ directory, 0 bytes,
    # and was reported as a shipped build. Verify the payload before claiming
    # success.
    echo "UAT reported success; verifying the staged payload..."

    FOUND_APP=""
    if [[ "$SERVER_ONLY" == "true" ]]; then
        FOUND_BIN=$(find "$STAGING" -type f -perm +111 -name "${PROJECT_NAME}Server*" 2>/dev/null | head -1)
        [[ -z "$FOUND_BIN" ]] && FOUND_BIN=$(find "$STAGING" -type f -perm +111 -name "$PROJECT_NAME*" 2>/dev/null | head -1)
        if [[ -n "$FOUND_BIN" ]]; then
            echo "  server executable: $FOUND_BIN ($(du -h "$FOUND_BIN" | cut -f1))"
        else
            echo "ERROR: no server executable under $STAGING"
            echo "       UAT exited 0 but produced nothing runnable."
            exit 1
        fi
    else
        FOUND_APP=$(find "$STAGING" -maxdepth 4 -name "*.app" -type d 2>/dev/null | head -1)
        if [[ -z "$FOUND_APP" ]]; then
            echo "ERROR: no .app bundle under $STAGING"
            echo "       UAT exited 0 but produced nothing runnable."
            exit 1
        fi
        APP_SIZE_KB=$(du -sk "$FOUND_APP" | cut -f1)
        # A real packaged client is hundreds of MB. Anything under 50 MB is a shell.
        if [[ "$APP_SIZE_KB" -lt 51200 ]]; then
            echo "ERROR: $FOUND_APP is only $(du -sh "$FOUND_APP" | cut -f1)"
            echo "       That is a stub, not a packaged game. Treating as failure."
            exit 1
        fi
        EXEC=$(find "$FOUND_APP/Contents/MacOS" -type f -perm +111 2>/dev/null | head -1)
        if [[ -z "$EXEC" ]]; then
            echo "ERROR: $FOUND_APP has no executable in Contents/MacOS"
            exit 1
        fi
        echo "  app:        $FOUND_APP ($(du -sh "$FOUND_APP" | cut -f1))"
        echo "  executable: $EXEC"
        # A cooked build must ship content; a pak-less client cannot load a map.
        PAK_COUNT=$(find "$STAGING" -name "*.pak" 2>/dev/null | wc -l | tr -d ' ')
        echo "  pak files:  $PAK_COUNT"
        if [[ "$PAK_COUNT" -eq 0 ]]; then
            echo "ERROR: no .pak files staged; the client has no cooked content to load."
            exit 1
        fi
    fi

    echo ""
    echo "=== Build succeeded and payload verified ==="
    echo "Output: $STAGING"
    du -sh "$STAGING"
    echo ""
    echo "To run:"
    if [[ "$SERVER_ONLY" == "true" ]]; then
        echo "  $FOUND_BIN"
    else
        echo "  open \"$FOUND_APP\""
    fi
else
    echo "=== Build FAILED (exit code $EXIT_CODE) ==="
    echo "Check the log above for errors."
fi

exit $EXIT_CODE
