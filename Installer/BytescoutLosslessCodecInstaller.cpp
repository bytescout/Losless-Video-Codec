//*******************************************************************
//       ByteScout Lossless Video Codec		                                     
//                                                                   
//       Copyright © 2016 ByteScout - http://www.bytescout.com       
//       ALL RIGHTS RESERVED                                         
//                                                                   
//*******************************************************************

// BytescoutLosslessCodecInstaller.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "version_number.h"

//////////////////////////////////////////////////////////////////////////
// changes to registry
/*
[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\BytescoutLosslessCodec]
@=""
"DisplayName"="Bytescout Lossless Codec"
"UninstallString"="rundll32.exe setupapi,InstallHinfSection DefaultUninstall 132 C:\\Windows\\system32\\DRIVERS\\BytescoutLosslessCodec.inf"
"DsiplayIcon"="C:\\program.exe,0"
"DisplayVersion"="1.0.161"
"NoModify"=dword:00000000
"NoRepair"=dword:00000000
"HelpLink"="http://www.bytescout.com"
"Publisher"="Bytescout"
"URLInfoAbout"="http://www.bytescout.com"
"URLUpdateInfo"="http://www.bytescout.com"

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Drivers32]
"VIDC.BLVC"="BytescoutLosslessCodec.dll"

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\MediaResources\icm\VIDC.BLVC]
"Description"="Bytescout Lossless Codec 1.0.161. [BLVC]"
"Driver"="BytescoutLosslessCodec.dll"
"FriendlyName"="Bytescout Lossless Codec 1.0.161. [BLVC]"
*/

//////////////////////////////////////////////////////////////////////////
// changes to file system
/*
C:\Windows\System32
  BytescoutLosslessCodec.dll

C:\Windows\SysWOW64 (win32 on x64)
  BytescoutLosslessCodec.dll

C:\Windows\System32\drivers
  BytescoutLosslessCodec.inf
*/

#ifdef X64
#define g_sCaption      L"Bytescout Lossless Codec Installer (x64)"
#else
#define g_sCaption      L"Bytescout Lossless Codec Installer"
#endif

#define g_sSwitches     L"\n\nSwitches are:\n/install - Install codec\n/uninstall - Uninstall codec\n/silent - Silent mode [no user interaction]."

#ifdef X64
#define g_sDisplayName              L"Bytescout Lossless Codec (x64)"
#else
#define g_sDisplayName              L"Bytescout Lossless Codec"
#endif

#define g_sUninstallString          L"rundll32.exe setupapi,InstallHinfSection DefaultUninstall 132 %s%s"
#define g_sDsiplayIcon              L"C:\\program.exe,0"
#define g_sDisplayVersion           T_VERSION_STRING
#define g_NoModify                  0
#define g_NoRepair                  0
#define g_sHelpLink                 L"http://www.bytescout.com"
#define g_sPublisher                L"Bytescout"
#define g_sURLInfoAbout             g_sHelpLink
#define g_sURLUpdateInfo            g_sHelpLink
#define g_DriverID                  L"VIDC.BLVC"
#define g_sDescription              L"Bytescout Lossless Codec [BLVC]"
#define g_sFriendlyName             g_sDescription

#define g_sDriver                   L"BytescoutLosslessCodec.dll"
#define g_sInf                      L"BytescoutLosslessCodec.inf"

#define g_sUninstallKey             L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\BytescoutLosslessCodec\\"
#define g_sDriversKey               L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32\\"
#define g_sICMKey                   L"SYSTEM\\CurrentControlSet\\Control\\MediaResources\\icm"

bool install(bool silent);
bool uninstall(bool silent);

bool getPaths(CString& dllPath, CString& infPath);
void writeToRegistry(LPCTSTR key, LPCTSTR subKey, CString value, HKEY hive);
void writeToRegistry(LPCTSTR key, LPCTSTR subKey, DWORD value, HKEY hive);
void removeFromRegistry(LPCTSTR key, HKEY hive, bool deleteKey);
bool executeExternalFile(CString csExeName, CString csArguments);

int _tmain(int argc, _TCHAR* argv[])
{
    bool gotInstall = false;
    bool gotUninstall = false;
    bool gotSilent = false;

    for (int i = 1; i < argc; i++)
    {
        CString s = argv[i];
        s.MakeLower();

        if (s == L"/install")
            gotInstall = true;
        else if (s == L"/uninstall")
            gotUninstall = true;
        else if (s == L"/silent")
            gotSilent = true;
    }

    if (gotInstall && gotUninstall)
    {
        CString message = L"Both /install and /uninstall specified. Don't know what to do.";
        message += g_sSwitches;
        MessageBox(NULL, message, g_sCaption, MB_ICONINFORMATION | MB_OK);
        return -1;
    }

    if (!gotUninstall)
    {
        // perform install if no switches specified
        gotInstall = true;
    }

    bool res = false;
    if (gotInstall)
        res = install(gotSilent);
    else if (gotUninstall)
        res = uninstall(gotSilent);

    CString message;
    if (gotInstall)
        message = "Install ";
    else 
        message = "Uninstall ";

    if (res == true && !gotSilent)
    {
        message += "completed successfully.";
        MessageBox(NULL, message, g_sCaption, MB_ICONINFORMATION | MB_OK);
	    return 0;
    }

    if (!gotSilent)
    {
        message += "failed.";
        MessageBox(NULL, message, g_sCaption, MB_ICONERROR | MB_OK);
    }

    return -2;
}

bool install(bool silent)
{
    int res = IDYES;
    if (!silent)
        res = MessageBox(NULL, L"Do you want to install Bytescout Lossless Video Codec?", g_sCaption, MB_YESNO | MB_ICONINFORMATION | MB_DEFBUTTON2);

    if (res == IDYES)
    {
        CString dllPath;
        CString infPath;
        if (!getPaths(dllPath, infPath))
            return false;

        // make changes to file system

        CString path = dllPath + g_sDriver;
        BOOL b = CopyFile(g_sDriver, path, FALSE);
        if (b == FALSE)
            return false;

        b = executeExternalFile("regsvr32", "/s " + path);
        if (b == FALSE)
            return false;

        path = infPath + g_sInf;
        b = CopyFile(g_sInf, path, FALSE);
        if (b == FALSE)
            return false;

        // make changes to registry

        writeToRegistry(g_sUninstallKey, L"DisplayName", g_sDisplayName, HKEY_LOCAL_MACHINE);

        CString value;
        value.Format(g_sUninstallString, infPath, g_sInf);
        writeToRegistry(g_sUninstallKey, L"UninstallString", value, HKEY_LOCAL_MACHINE);

        writeToRegistry(g_sUninstallKey, L"DsiplayIcon", g_sDsiplayIcon, HKEY_LOCAL_MACHINE);
        writeToRegistry(g_sUninstallKey, L"DisplayVersion", g_sDisplayVersion, HKEY_LOCAL_MACHINE);
        writeToRegistry(g_sUninstallKey, L"HelpLink", g_sHelpLink, HKEY_LOCAL_MACHINE);
        writeToRegistry(g_sUninstallKey, L"Publisher", L"Bytescout", HKEY_LOCAL_MACHINE);
        writeToRegistry(g_sUninstallKey, L"URLInfoAbout", g_sURLInfoAbout, HKEY_LOCAL_MACHINE);
        writeToRegistry(g_sUninstallKey, L"URLUpdateInfo", g_sURLUpdateInfo, HKEY_LOCAL_MACHINE);
        writeToRegistry(g_sUninstallKey, L"NoModify", g_NoModify, HKEY_LOCAL_MACHINE);
        writeToRegistry(g_sUninstallKey, L"NoRepair", g_NoRepair, HKEY_LOCAL_MACHINE);

        writeToRegistry(g_sDriversKey, g_DriverID, g_sDriver, HKEY_LOCAL_MACHINE);

        CString fullICMKey = g_sICMKey;
        fullICMKey += "\\";
        fullICMKey += g_DriverID;
        fullICMKey += "\\";

        writeToRegistry(fullICMKey, L"Description", g_sDescription, HKEY_LOCAL_MACHINE);
        writeToRegistry(fullICMKey, L"Driver", g_sDriver, HKEY_LOCAL_MACHINE);
        writeToRegistry(fullICMKey, L"FriendlyName", g_sFriendlyName, HKEY_LOCAL_MACHINE);

        return true;
    }

    return false;
}

bool uninstall(bool silent)
{
    int res = IDYES;

    if (!silent)
        res = MessageBox(NULL, L"Do you want to uninstall Bytescout Lossless Video Codec?", g_sCaption, MB_YESNO | MB_ICONINFORMATION | MB_DEFBUTTON2);

    if (res == IDYES)
    {
        // make changes to registry
        CString key = g_sUninstallKey;
        key += "DisplayName";

        removeFromRegistry(key, HKEY_LOCAL_MACHINE, true);

        key = g_sDriversKey;
        key += g_DriverID;
        removeFromRegistry(key, HKEY_LOCAL_MACHINE, false);

        key = g_sICMKey;
        key += "\\";
        key += g_DriverID;
        key += "\\";
        key += "Description";
        removeFromRegistry(key, HKEY_LOCAL_MACHINE, true);

        // make changes to file system

        CString dllPath;
        CString infPath;
        if (!getPaths(dllPath, infPath))
            return false;

        CString path = dllPath + g_sDriver;

        BOOL b = executeExternalFile("regsvr32", "/u /s " + path);
        if (b == FALSE)
            return false;
        
        b = DeleteFile(path);
        if (b == FALSE)
            return false;

        path = infPath + g_sInf;
        b = DeleteFile(path);
        if (b == FALSE)
            return false;

        return true;
    }

    return false;
}

bool getPaths(CString& dllPath, CString& infPath)
{
    TCHAR system32[MAX_PATH];
    HRESULT hr = SHGetFolderPath(NULL, CSIDL_SYSTEM, NULL, SHGFP_TYPE_CURRENT, system32);
    if (hr != S_OK)
        return false;

    dllPath = system32;
    if (dllPath[dllPath.GetLength() - 1] != '\\')
        dllPath += '\\';

    infPath = dllPath;
    infPath += "drivers\\";
    return true;
}

void writeToRegistry(LPCTSTR key, LPCTSTR subKey, CString value, HKEY hive)
{
    CString fullKeyName = key;
    fullKeyName += subKey;

    CRegString keyObj(fullKeyName, L"", FALSE, hive);
    keyObj = value;
}

void writeToRegistry(LPCTSTR key, LPCTSTR subKey, DWORD value, HKEY hive)
{
    CString fullKeyName = key;
    fullKeyName += subKey;

    CRegDWORD keyObj(fullKeyName, (DWORD)-1, TRUE, hive);
    keyObj = value;
}

void removeFromRegistry(LPCTSTR key, HKEY hive, bool deleteKey)
{
    CRegString keyObj(key, L"", FALSE, hive);

    if (deleteKey)
        keyObj.removeKey();
    else
        keyObj.removeValue();
}

bool executeExternalFile(CString csExeName, CString csArguments)
{
#pragma warning(push)
#pragma warning(disable: 4311)
    // warning C4311: 'type cast' : pointer truncation from 'HINSTANCE' to 'int'
    int res = (int)ShellExecute(NULL, L"open", csExeName, csArguments, NULL, SW_SHOWNORMAL);
#pragma warning(pop)
    return (res > 32);
}
