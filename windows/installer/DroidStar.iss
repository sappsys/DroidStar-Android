; Inno Setup script for the self-contained Windows folder produced by
; scripts/build-windows.ps1. Adjust AppVersion to match DroidStar.pro.
; Compile after the dist\DroidStar-*-windows-x64 folder exists.

#define MyAppName "DroidStar"
#define MyAppVersion "0.45.07b"
#define MyAppPublisher "M8NLR"
#define MyAppURL "https://github.com/sappsys/DroidStar-Android"
#define MyAppExeName "DroidStar.exe"
#define MyBuildDir "..\..\dist\DroidStar-0.45.07b-windows-x64"

[Setup]
AppId={{A7C3E9D1-4B2F-4E8A-9C1D-DroidStarWin01}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir=..\..\dist
OutputBaseFilename=DroidStar-{#MyAppVersion}-windows-x64-setup
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "{#MyBuildDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
