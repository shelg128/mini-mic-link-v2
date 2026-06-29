#define MyAppName "MiniMic Link"
#define MyAppVersion "0.1.0"
#define MyAppPublisher "MiniMic Project"
#define MyAppExeName "MiniMic Link.exe"
#define SourceRoot "..\.."

[Setup]
AppId={{B53F1411-95DB-45D7-9F2E-39F95F783A2D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\MiniMic Link
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile={#SourceRoot}\LICENSE
OutputDir={#SourceRoot}\dist
OutputBaseFilename=MiniMic-Link-Windows-x64-Setup-{#MyAppVersion}
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "firewall"; Description: "Allow MiniMic Link through Windows Firewall"; GroupDescription: "Network access:"; Flags: checkedonce

[Files]
Source: "{#SourceRoot}\build\SonoBus_artefacts\Release\Standalone\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceRoot}\README_MINIMIC.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceRoot}\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{sys}\netsh.exe"; Parameters: "advfirewall firewall delete rule name=""MiniMic Link"" program=""{app}\{#MyAppExeName}"""; Flags: runhidden; Tasks: firewall
Filename: "{sys}\netsh.exe"; Parameters: "advfirewall firewall add rule name=""MiniMic Link"" dir=in action=allow program=""{app}\{#MyAppExeName}"" enable=yes profile=any"; Flags: runhidden; Tasks: firewall
Filename: "{sys}\netsh.exe"; Parameters: "advfirewall firewall add rule name=""MiniMic Link"" dir=out action=allow program=""{app}\{#MyAppExeName}"" enable=yes profile=any"; Flags: runhidden; Tasks: firewall
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
