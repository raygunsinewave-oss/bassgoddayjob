BASSGODSDAYJOB - by Raymond Duarte (Raygun)

EASY INSTALL
  1. Right-click "Install BASSGODSDAYJOB.command" and choose Open (the first time macOS asks, click Open).
  2. Restart your DAW.

MANUAL INSTALL
  AU instrument (Logic, GarageBand, Ableton, etc.): copy BASSGODSDAYJOB.component to
      ~/Library/Audio/Plug-Ins/Components
  VST3 (Ableton, Bitwig, Reaper, etc.): copy BASSGODSDAYJOB.vst3 to
      ~/Library/Audio/Plug-Ins/VST3
  Standalone: drag BASSGODSDAYJOB.app to Applications and open it.

IF macOS SAYS IT'S "DAMAGED" OR WON'T LOAD IT
  These builds aren't notarized by Apple yet. In Terminal run:
      xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/BASSGODSDAYJOB.component ~/Library/Audio/Plug-Ins/VST3/BASSGODSDAYJOB.vst3 /Applications/BASSGODSDAYJOB.app

Works on macOS 11+ (Apple Silicon and Intel).
