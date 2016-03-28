[Setup]
AppName=Bytescout Lossless Video Codec
AppVerName=Bytescout Lossless Video Codec 1.00.187 (FREEWARE)
AppPublisher=Bytescout Software
AppPublisherURL=http://www.bytescout.com
AppSupportURL=http://www.bytescout.com
AppUpdatesURL=http://www.bytescout.com
DefaultDirName={pf}\Bytescout Lossless Video Codec
DefaultGroupName=Bytescout Lossless Video Codec
LicenseFile=License.txt
InfoBeforeFile=Readme.txt
OutputBaseFilename=BytescoutLosslessVideoCodec1.00.187
Compression=lzma
SolidCompression=yes
SignedUninstaller=yes
VersionInfoVersion=1.00.187.0
ArchitecturesAllowed=x86 x64 ia64
ArchitecturesInstallIn64BitMode=x64 ia64
[Tasks]
[Files]
Source: "files\x86\BytescoutLosslessCodec1.00.dll"; DestName: BytescoutLosslessCodec.dll ;DestDir: "{app}\x86";
Source: "files\x86\BytescoutLosslessCodecInstaller1.00.exe"; DestName: BytescoutLosslessCodecInstaller.exe; DestDir: "{app}\x86";
Source: "files\x86\BytescoutLosslessCodec1.00.inf"; DestName: BytescoutLosslessCodec.inf; DestDir: "{app}\x86";
Source: "files\x64\BytescoutLosslessCodec1.00.dll"; DestName: BytescoutLosslessCodec.dll; DestDir: "{app}\x64";
Source: "files\x64\BytescoutLosslessCodecInstaller1.00.exe"; DestName: BytescoutLosslessCodecInstaller.exe; DestDir: "{app}\x64";
Source: "files\x64\BytescoutLosslessCodec1.00.inf"; DestName: BytescoutLosslessCodec.inf; DestDir: "{app}\x64";
Source: "files\WebPage.url"; DestName: WebPage.url; DestDir: "{app}"; Flags: ignoreversion
[Icons]
Name: "{group}\{cm:UninstallProgram,Bytescout Lossless Video Codec}"; Filename: "{uninstallexe}"
[Run]
Filename: {app}\x86\BytescoutLosslessCodecInstaller.exe; WorkingDir: {app}\x86; Parameters: /silent /install; StatusMsg: Installing codec (x86)...; Flags: runhidden;
Filename: {app}\x64\BytescoutLosslessCodecInstaller.exe; WorkingDir: {app}\x64; Parameters: /silent /install; StatusMsg: Installing codec (x86)...; Flags: runhidden; Check: IsX64
[UninstallRun]
Filename: {app}\x86\BytescoutLosslessCodecInstaller.exe; WorkingDir: {app}\x86; Parameters: /silent /uninstall; StatusMsg: Installing codec (x86)...; Flags: runhidden;
Filename: {app}\x64\BytescoutLosslessCodecInstaller.exe; WorkingDir: {app}\x64; Parameters: /silent /uninstall; StatusMsg: Installing codec (x86)...; Flags: runhidden; Check: IsX64
[Registry]
[Code]
function IsX64Arch: Boolean;
begin
Result := Is64BitInstallMode and (ProcessorArchitecture = paX64);
end;
function IsIA64Arch: Boolean;
begin
Result := Is64BitInstallMode and (ProcessorArchitecture = paIA64);
end;
function IsX64: Boolean;
begin
Result := IsX64Arch or IsIA64Arch;
end;
function IsNotX64: Boolean;
begin
Result := not IsX64Arch and not IsIA64Arch;
end;

procedure DeinitializeUninstall();
var
ResultCode: Integer;
begin
if MsgBox('Thank you very much for trying Bytescout Lossless Video Codec.'+#10+
'-----------------------------------------------------------------'+#10+'We REALLY need your feedback to improve our software!'
+#10#13+'Would you please fill a VERY SHORT feedback form on our web-site?'+#10+#10+'Thanks in advance for your cooperation!', mbConfirmation, MB_YESNO or MB_DEFBUTTON1) = IDYES then
ShellExec('open', 'http://www.bytescout.com/uninstall.php?product=Bytescout%20Lossless%20Video%20Codec&version=1.00.187', '', '', SW_SHOW, ewNoWait, ResultCode)
end;

