#!/bin/bash
# Capture one reference screen from the real game UI at the reference resolution.
# Usage: capture.sh <screenNumber> [outDir]
set -u
SCREEN=$1
STEP=${RA4STEP:-0}
BLOC=${RA4BLOC:-0}
COUNTRY=${RA4COUNTRY:-0}
OUTDIR=${2:-/Users/romanmolodyko/Documents/Scarlet-Horizon/Docs/UI/VisualDiff/current}
PROJ=/Users/romanmolodyko/Documents/Scarlet-Horizon
UE="/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor"
SUFFIX=""
[ "$STEP" != "0" ] && SUFFIX="${SUFFIX}_s$STEP"
[ "$BLOC" != "0" ] && SUFFIX="${SUFFIX}_b$BLOC"
SHOT="$PROJ/Saved/Screenshots/MacEditor/RA4_UI_Reference_$(printf '%02d' "$SCREEN").png"
rm -f "$SHOT"
"$UE" "$PROJ/RedAlert4.uproject" \
  "/Game/Maps/RA4_MainMenu?game=/Script/RedAlert4.RA4UIShowcaseGameMode" \
  -game -ResX=1672 -ResY=941 -windowed -nosplash -unattended \
  -ExecCmds="DisableAllScreenMessages" -RA4CaptureUI -RA4ExitAfterCapture -RA4Screen="$SCREEN" -RA4Step="$STEP" -RA4Bloc="$BLOC" -RA4Country="$COUNTRY" \
  > "/tmp/ra4_capture_$SCREEN.log" 2>&1
mkdir -p "$OUTDIR"
if [ -f "$SHOT" ]; then
  cp "$SHOT" "$OUTDIR/$(printf '%02d' "$SCREEN")$SUFFIX.png"
  echo "OK $SCREEN step=$STEP -> $OUTDIR/$(printf '%02d' "$SCREEN")$SUFFIX.png"
else
  echo "MISSING $SCREEN (see /tmp/ra4_capture_$SCREEN.log)"
fi
