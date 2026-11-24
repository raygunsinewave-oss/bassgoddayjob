#!/bin/bash
# Double-click to install BASSGODSDAYJOB (AU + VST3 + app) for your user account.
cd "$(dirname "$0")"
set -e
echo "Installing BASSGODSDAYJOB..."
mkdir -p ~/Library/Audio/Plug-Ins/Components ~/Library/Audio/Plug-Ins/VST3 ~/Applications
rm -rf ~/Library/Audio/Plug-Ins/Components/BASSGODSDAYJOB.component ~/Library/Audio/Plug-Ins/VST3/BASSGODSDAYJOB.vst3 ~/Applications/BASSGODSDAYJOB.app
cp -R "BASSGODSDAYJOB.component" ~/Library/Audio/Plug-Ins/Components/
cp -R "BASSGODSDAYJOB.vst3" ~/Library/Audio/Plug-Ins/VST3/
cp -R "BASSGODSDAYJOB.app" ~/Applications/
# these builds are not notarized by Apple: remove the download quarantine so macOS lets them load
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/BASSGODSDAYJOB.component ~/Library/Audio/Plug-Ins/VST3/BASSGODSDAYJOB.vst3 ~/Applications/BASSGODSDAYJOB.app 2>/dev/null || true
# make Logic / GarageBand pick up the new Audio Unit
killall -9 AudioComponentRegistrar 2>/dev/null || true
echo ""
echo "Done!"
echo "  AU:   ~/Library/Audio/Plug-Ins/Components/BASSGODSDAYJOB.component"
echo "  VST3: ~/Library/Audio/Plug-Ins/VST3/BASSGODSDAYJOB.vst3"
echo "  App:  ~/Applications/BASSGODSDAYJOB.app"
echo ""
echo "Restart your DAW (or rescan plug-ins). In Logic: Software Instrument > AU Instruments > Raymond Duarte (Raygun) > BASSGODSDAYJOB"
open ~/Applications/BASSGODSDAYJOB.app || true
