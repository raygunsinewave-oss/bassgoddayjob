; Inno Setup script for BASSGODSDAYJOB (Windows 10+, x64)
; Build: iscc /DArtefacts="C:\path\to\build\BASSGODSDAYJOB_artefacts\Release" BASSGODSDAYJOB.iss

#define AppName "BASSGODSDAYJOB"
#define AppVersion "1.0.0"
#define AppPublisher "Raymond Duarte (Raygun)"
#ifndef Artefacts
  #define Artefacts "..\..\build\BASSGODSDAYJOB_artefacts\Release"
#endif

[Setup]
AppId={{6C1B9B7E-2B9F-4C7C-9C55-8A9F0B6D2A11}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
OutputBaseFilename={#AppName}-{#AppVersion}-Windows-x64-Setup
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
PrivilegesRequired=admin
WizardStyle=modern
LicenseFile=..\..\LICENSE.md
UninstallDisplayName={#AppName}

[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plug-in"; Types: full custom; Flags: fixed
Name: "standalone"; Description: "Standalone application"; Types: full custom
Name: "docs"; Description: "User manual"; Types: full custom

[Files]
Source: "{#Artefacts}\VST3\{#AppName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#AppName}.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Artefacts}\Standalone\{#AppName}.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "..\..\docs\USER_MANUAL.md"; DestDir: "{app}\docs"; Components: docs; Flags: ignoreversion
Source: "..\..\README.md"; DestDir: "{app}\docs"; Components: docs; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppName}.exe"; Components: standalone
Name: "{group}\User Manual"; Filename: "{app}\docs\USER_MANUAL.md"; Components: docs
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\{#AppName}.exe"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent; Components: standalone
