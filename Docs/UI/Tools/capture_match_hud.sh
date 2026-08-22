#!/bin/bash
# Capture the canonical in-match HUD (URA4SidebarWidget) at reference resolution.
# The showcase screens are captured by capture.sh; this one needs a real match.
set -u
PROJ=/Users/romanmolodyko/Documents/Scarlet-Horizon
UE="/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor"
OUTDIR=${1:-$PROJ/Docs/UI/VisualDiff/current}
SHOT="$PROJ/Saved/Screenshots/MacEditor/RA4_UI_QA_MatchHUD.png"
rm -f "$SHOT"
"$UE" "$PROJ/RedAlert4.uproject" /Game/Maps/RA4_Skirmish_Production \
  -game -ResX=1672 -ResY=941 -windowed -nosplash -unattended \
  -ExecCmds="DisableAllScreenMessages" -RA4CaptureUI \
  > /tmp/ra4_match_hud.log 2>&1 &
UEPID=$!
for _ in $(seq 1 60); do sleep 2; [ -f "$SHOT" ] && break; done
sleep 3; kill "$UEPID" 2>/dev/null
mkdir -p "$OUTDIR"
if [ -f "$SHOT" ]; then cp "$SHOT" "$OUTDIR/match_hud.png"; echo "OK -> $OUTDIR/match_hud.png";
else echo "MISSING (see /tmp/ra4_match_hud.log)"; fi
