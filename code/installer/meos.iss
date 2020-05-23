; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{5A8ACFDC-9E0A-4B59-9D8F-4013251EB156}
AppName=MEOS-OZ
AppVersion=3.7.1188.0
AppPublisher=undy
AppPublisherURL=https://github.com/undyau/meos-oz
AppSupportURL=https://github.com/undyau/meos-oz
AppUpdatesURL=https://github.com/undyau/meos-oz
DefaultDirName={pf}\MEOS-OZ
DefaultGroupName=MEOS-OZ
LicenseFile=license.txt
OutputDir=.
OutputBaseFilename=meos-setup
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\Release\meos.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "libHaru.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "libmySQL.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "license.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "*.html"; DestDir: "{app}"; Flags: ignoreversion
Source: "*.meos"; DestDir: "{app}"; Flags: ignoreversion
Source: "*.brules"; DestDir: "{app}"; Flags: ignoreversion
Source: "mysqlpp.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "thirdpartylicense.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "baseclass.xml"; DestDir: "{app}"; Flags: ignoreversion
Source: "database.wclubs"; DestDir: "{app}"; Flags: ignoreversion
Source: "database.wpersons"; DestDir: "{app}"; Flags: ignoreversion
Source: "*.lxml"; DestDir: "{app}"; Flags: ignoreversion
Source: "*.listdef"; DestDir: "{app}"; Flags: ignoreversion
Source: "ind_courseresult.lxml"; DestDir: "{app}"; Flags: ignoreversion
Source: "sss201230.xml"; DestDir: "{app}"; Flags: ignoreversion
Source: "classcourse.lxml"; DestDir: "{app}"; Flags: ignoreversion
Source: "SSS Receipt Results.xml"; DestDir: "{app}"; Flags: ignoreversion
Source: "SSS Results.xml"; DestDir: "{app}"; Flags: ignoreversion
Source: "vc_redist.x86.exe"; DestDir: {tmp}; Flags: deleteafterinstall
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\MEOS-OZ"; Filename: "{app}\meos.exe"
Name: "{commondesktop}\MEOS-OZ"; Filename: "{app}\meos.exe"; Tasks: desktopicon

[Run]
Filename: "{tmp}\vc_redist.x86.exe"; Check: VCRedistNeedsInstall
Filename: "{app}\meos.exe"; Parameters: "-s"
Filename: "{app}\meos.exe"; Description: "{cm:LaunchProgram,MEOS-OZ}"; Flags: nowait postinstall skipifsilent

[Code]
function VCRedistNeedsInstall: Boolean;
 // Function for Inno Setup Compiler
 // Returns True if same or later Microsoft Visual C++ 2015 Redistributable is installed, otherwise False.
 var
  major: Cardinal;
  minor: Cardinal;
  bld: Cardinal;
  rbld: Cardinal;
  key: String;
 begin
  Result := True;
  key := 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x86';
  if RegQueryDWordValue(HKEY_LOCAL_MACHINE, key, 'Major', major) then begin
    if RegQueryDWordValue(HKEY_LOCAL_MACHINE, key, 'Minor', minor) then begin
      if RegQueryDWordValue(HKEY_LOCAL_MACHINE, key, 'Bld', bld) then begin
        if RegQueryDWordValue(HKEY_LOCAL_MACHINE, key, 'RBld', rbld) then begin
            Log('VC 2015 Redist Major is: ' + IntToStr(major) + ' Minor is: ' + IntToStr(minor) + ' Bld is: ' + IntToStr(bld) + ' Rbld is: ' + IntToStr(rbld));
            // Version info was found. Return true if major is not 14 - should be if we get here
            Result := (major <> 14)
        end;
      end;
    end;
  end;
 end;

