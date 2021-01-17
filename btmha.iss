[Setup]
AppName=BTMHA
AppVerName=BTNRH BTMHA 0.12
AppPublisher=Boys Town Nationial Research Hospital
AppPublisherURL=http://www.boystownhospital.org
AppSupportURL=http://audres.org/rc/btmha/
AppUpdatesURL=http://audres.org/rc/btmha/
DefaultDirName={pf}\BTNRH\BTMHA
DefaultGroupName=BTNRH
ChangesAssociations=Yes

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"; MinVersion: 4,4

[Files]
Source: "VS16\Release\btmha.exe"; DestDir: "{app}";      Flags: ignoreversion
Source: "tst_*.lst";              DestDir: "{app}";      Flags: ignoreversion
Source: "tst_*.m";                DestDir: "{app}";      Flags: ignoreversion
Source: "test\carrots.wav";       DestDir: "{app}\test"; Flags: ignoreversion
Source: "test\cat.wav";           DestDir: "{app}\test"; Flags: ignoreversion
Source: "test\impulse.wav";       DestDir: "{app}\test"; Flags: ignoreversion
Source: "test\tone.wav";          DestDir: "{app}\test"; Flags: ignoreversion

[Icons]
Name: "{group}\BTMHA"; Filename: "{app}\btmha.exe"
Name: "{userdesktop}\BTMHA"; Filename: "{app}\btmha.exe"; MinVersion: 4,4; Tasks: desktopicon

[Run]
Filename: "{app}\btmha.exe"; Description: "Launch BTMHA?"; Flags: nowait postinstall skipifsilent

