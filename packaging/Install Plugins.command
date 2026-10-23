#!/bin/bash
# Installs the pboard VST3 and AU plug-ins for the current user.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"

mkdir -p "$VST3_DIR" "$AU_DIR"
rm -rf "$VST3_DIR/pboard.vst3" "$AU_DIR/pboard.component"
ditto "$HERE/Plug-Ins/pboard.vst3" "$VST3_DIR/pboard.vst3"
ditto "$HERE/Plug-Ins/pboard.component" "$AU_DIR/pboard.component"
xattr -dr com.apple.quarantine "$VST3_DIR/pboard.vst3" "$AU_DIR/pboard.component" 2>/dev/null || true

# Clear the AU cache so hosts pick up the new version
killall -9 AudioComponentRegistrar 2>/dev/null || true

echo ""
echo "pboard plug-ins installed:"
echo "  VST3: $VST3_DIR/pboard.vst3"
echo "  AU:   $AU_DIR/pboard.component"
echo ""
echo "Rescan plug-ins in your DAW. You can close this window."
