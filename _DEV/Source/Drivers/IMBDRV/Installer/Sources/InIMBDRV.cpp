// InIMBDRV.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <tchar.h>
#include <Msiquery.h>

//#include "..\..\Driver\Sources\driver.h"
#include "FSCIoctl.h"

#include <new.h>

#define DRIVER_VERSION 1

#define ROOT_DEVICE_ID "*imbdrv"
#define DLL_NAME "InIMBDRV.dll"
#define INF_NAME "IMBDrv.inf"
#define DEVICE_NAME "\\\\.\\Imb"

#include <tchar.h>
#include <Msiquery.h>
#include <stdio.h>
#include <string.h>
#include <winioctl.h>
#include <devguid.h>
#include <setupapi.h>
#include "CCOpusTrace.h"

#define MAX_CLASS_NAME_LEN	32

#pragma warning ( disable : 4100)	// unreferenced formal parameter

// forward prototypes
static void IncrementShareCount();
static void DeleteShareCount();
static UINT UnconditionalUnInstall(MSIHANDLE hMsi);
static LPSTR FindExistingDevice(LPCSTR Id);

typedef UINT (__cdecl InstallerEntry_T)(MSIHANDLE,TCHAR*version,TCHAR*dvdriver,BOOL);

typedef MSIHANDLE (__stdcall MsiCreateRecord_T)(unsigned);
typedef UINT (__stdcall MsiRecordSetStringA_T)(MSIHANDLE, unsigned, LPCTSTR);
typedef	int (__stdcall MsiProcessMessage_T)(MSIHANDLE, INSTALLMESSAGE, MSIHANDLE);
typedef	UINT (__stdcall MsiCloseHandle_T)(MSIHANDLE);


// global variables
bool Is64bit = false;

static TCHAR * VersionNT64 = NULL; 
static TCHAR * DVDRIVERPATH = _T(".");

static void SetGlobals(TCHAR *pVersionNT64, TCHAR *pDVDRIVERPATH)
{
	if (pVersionNT64 && _tcslen(pVersionNT64))
		VersionNT64 = pVersionNT64;
	if (pDVDRIVERPATH && _tcslen(pDVDRIVERPATH))
		DVDRIVERPATH = pDVDRIVERPATH;

	// When the property exists then it holds an OS version number so it is not empty.
	Is64bit = VersionNT64 ? true : false;
}

#ifdef _MANAGED
#pragma managed(push, off)
#endif

// enable tracing
TRC_DECLARE_ENABLE_AUTO;


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FENTER, _T(""), _T("ul_reason_for_call: %d, lpReserved: 0x%08p"), ul_reason_for_call, lpReserved));
	TRC_BUILD_INFO((TRC_LEVEL_INFO, _T(""), _T("")));

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T(""), _T("%d"), TRUE));
    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif

static void LogString(MSIHANDLE hMsi, char *szString)
{
	if (hMsi)
	{
		HMODULE hMsiLib = LoadLibrary("msi.dll");
		if (hMsiLib)
		{
			MsiCreateRecord_T *pfMsiCreateRecord = (MsiCreateRecord_T*)GetProcAddress(hMsiLib, _T("MsiCreateRecord"));
			MsiRecordSetStringA_T *pfMsiRecordSetStringA = (MsiRecordSetStringA_T*)GetProcAddress(hMsiLib, _T("MsiRecordSetStringA"));
			MsiProcessMessage_T *pfMsiProcessMessage = (MsiProcessMessage_T*)GetProcAddress(hMsiLib, _T("MsiProcessMessage"));
			MsiCloseHandle_T *pfMsiCloseHandle = (MsiCloseHandle_T*)GetProcAddress(hMsiLib, _T("MsiCloseHandle"));

			if (pfMsiCreateRecord && pfMsiRecordSetStringA && pfMsiProcessMessage && pfMsiCloseHandle)
			{
				MSIHANDLE newHandle = pfMsiCreateRecord(2);
				char szTemp[MAX_PATH*2];
				sprintf_s(szTemp, sizeof(szTemp), "-DRIVER_LOG-  %s", szString);
				pfMsiRecordSetStringA(newHandle, 0, szTemp);
				pfMsiProcessMessage(hMsi, INSTALLMESSAGE(INSTALLMESSAGE_INFO), newHandle);
				pfMsiCloseHandle(newHandle);
			}
			FreeLibrary(hMsiLib);
		}
	}
	else
	{
		TRC_OUTTV2((TRC_LEVEL_VERBOSE, TRC_TYPE_FINTERNAL, _T("LogString:"), _T("%s"), szString));
	}
}

static DWORD GetDriverVersion(void)
{
	DWORD DriverVersion = 0;
	
	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FENTER, _T(""), _T("")));	

	// check for server OS with IPMI support is done in InDrivers.dll
	
	/*
	 *	Open device
	 */
	HANDLE hF = CreateFile(_T(DEVICE_NAME),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hF != INVALID_HANDLE_VALUE) {

		DriverVersion = DRIVER_VERSION;

		CloseHandle(hF);
	}
	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T(""), _T("DriverVersion=%d"), DriverVersion));

	return DriverVersion;
}

static UINT RunMyUninstallProcess(MSIHANDLE hMsi, TCHAR *ExePath)
{
	DWORD rv;

	TCHAR cmd[MAX_PATH*2 + 100];

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FENTER, _T(""), _T("hMsi=0x%08x, ExePath=%s"), hMsi, ExePath));

	// The documentation of CreateProcess is !!$#@%$# (censored)
	_tcscpy_s(cmd, sizeof(cmd), _T("\""));	// enclose in ""
	_tcscat_s(cmd, sizeof(cmd), ExePath);
	_tcscat_s(cmd, sizeof(cmd), _T("\" "));

	_tcscat_s(cmd, sizeof(cmd), _T(" "));
	_tcscat_s(cmd, sizeof(cmd), ROOT_DEVICE_ID);

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );

#if 1
	char s[999];
	sprintf_s(s, sizeof(s), "RunMyUninstallProcess: cmd = %s", cmd);
	LogString(hMsi, s);
#endif

	if (CreateProcess(NULL, cmd, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
	{
		// Wait until child process exits.
		WaitForSingleObject( pi.hProcess, INFINITE );
		GetExitCodeProcess(pi.hProcess, &rv);

		// Close process and thread handles. 
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );

		TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T(""), _T("%d"), rv));
		return rv;
	}
	LogString(hMsi, DLL_NAME": Could not run PnP uninstaller");
	rv = GetLastError();

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T(""), _T("rv=0x%08x"), rv ? rv : ERROR_INTERNAL_ERROR));
	return rv ? rv : ERROR_INTERNAL_ERROR;
}
// Should return 0		Success	(ErrorCode must be 0)
//				 1		Reboot required
//				 Other	Failed.
//
static UINT RunMyInstallProcess(MSIHANDLE hMsi, TCHAR *ExePath, BOOL force, TCHAR *InfFilePath)
{
	DWORD rv;

	TCHAR cmd[MAX_PATH*2 + 100];
	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FENTER, _T(""), _T("hMsi=0x%08x, ExePath=%s, force: %d, InfFilePath=%s"), hMsi, ExePath, force, InfFilePath));

	// The documentation of CreateProcess is !!$#@%$# (censored)
	_tcscpy_s(cmd, sizeof(cmd), _T("\""));	// enclose in ""
	_tcscat_s(cmd, sizeof(cmd), ExePath);
	_tcscat_s(cmd, sizeof(cmd), _T("\" "));

	// Debug flag for UpdPnPDr.exe, used when not called from MSI
	if (!hMsi) _tcscat_s(cmd, sizeof(cmd), _T("/L "));
	if (force) _tcscat_s(cmd, sizeof(cmd), _T("/F "));

	_tcscat_s(cmd, sizeof(cmd), _T("\""));	// enclose in ""
	_tcscat_s(cmd, sizeof(cmd), InfFilePath);
	_tcscat_s(cmd, sizeof(cmd), _T("\""));

	_tcscat_s(cmd, sizeof(cmd), _T(" "));
	_tcscat_s(cmd, sizeof(cmd), ROOT_DEVICE_ID);

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );

#if 1
	char s[999];
	sprintf_s(s, sizeof(s), "RunMyInstallProcess: cmd = %s", cmd);
	LogString(hMsi, s);
#endif

	if (CreateProcess(NULL, cmd, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
	{
		// Wait until child process exits.
		WaitForSingleObject( pi.hProcess, INFINITE );
		GetExitCodeProcess(pi.hProcess, &rv);

		// Close process and thread handles. 
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );

		TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T(""), _T("%d"), rv));
		return rv;
	}
	LogString(hMsi, DLL_NAME": Could not run PnP installer");
	rv = GetLastError();

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T(""), _T("rv=0x%08x"), rv ? rv : ERROR_INTERNAL_ERROR));
	return rv ? rv : ERROR_INTERNAL_ERROR;
}

// Install and Update are the same
static UINT Install_or_Update(MSIHANDLE hMsi, BOOL bUpdate )
{
	UINT rv = ERROR_SUCCESS;
	BOOL DoVersionCheck = TRUE;
	DWORD CurrentVersion = 0;

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FENTER, _T(""), _T("hMsi=0x%08x, bUpdate: %d"), hMsi, bUpdate));

#if HAS_WHQL_CAT_FILE_AMD64
	if (Is64bit) DoVersionCheck = FALSE;
#endif
#if HAS_WHQL_CAT_FILE_I386
	if (!Is64bit) DoVersionCheck = FALSE;
#endif

	CurrentVersion = GetDriverVersion();

	if (CurrentVersion == 0)
	{
		TCHAR ImagePath[MAX_PATH + 100];
		(void)GetWindowsDirectory(ImagePath, MAX_PATH);
		_tcscat_s(ImagePath, sizeof(ImagePath), _T("\\system32\\drivers\\imbdrv.sys"));
		DeleteFile(ImagePath);
	}

	// Need to update driver ?
	if (!DoVersionCheck || CurrentVersion < DRIVER_VERSION)
	{
		char s[MAX_PATH + 1024];
		LogString(hMsi, DLL_NAME ": doing UpdPnpDr.exe " INF_NAME);

		TCHAR exe[MAX_PATH+100];
		_tcscpy_s(exe, sizeof(exe), DVDRIVERPATH);
		_tcscat_s(exe, sizeof(exe), (Is64bit) ? _T("\\amd64") : _T("\\i386"));
		_tcscat_s(exe, sizeof(exe), _T("\\UpdPnpDr.exe"));

		TCHAR inf[MAX_PATH+100];
		_tcscpy_s(inf, sizeof(inf), DVDRIVERPATH);
		_tcscat_s(inf, sizeof(inf), (Is64bit) ? _T("\\amd64") : _T("\\i386"));
		_tcscat_s(inf, sizeof(inf), _T("\\" INF_NAME));

		HANDLE hF = CreateFile(
			inf,
            GENERIC_READ,              // open for reading
            0,                         // no share for reading
            NULL,                      // default security
            OPEN_EXISTING,             // existing file only
            FILE_ATTRIBUTE_NORMAL,     // normal file
            NULL                       // no attr. template
		);

		if (hF == INVALID_HANDLE_VALUE)
		{
			rv = ERROR_FILE_NOT_FOUND;
			sprintf_s(s, sizeof(s), DLL_NAME": cannot open inf file %s", inf);
			LogString(hMsi, s);
		}
		else
		{
			sprintf_s(s, sizeof(s), DLL_NAME": inf file %s exists", inf);
			LogString(hMsi, s);
			CloseHandle(hF);

			rv = RunMyInstallProcess(hMsi, exe, DoVersionCheck, inf);

			sprintf_s(s, sizeof(s), DLL_NAME": UpdPnpDr.exe returned %08x", rv);
			LogString(hMsi, s);
		}
	}
	else
		LogString(hMsi, DLL_NAME": GetDriverVersion() indicates driver is uptodate");

	if (rv == ERROR_SUCCESS || rv == ERROR_SUCCESS_REBOOT_REQUIRED)
	{
		if (GetDriverVersion() == 0)
		{
			LogString(hMsi, DLL_NAME": driver finds no hardware, calling UnconditionalUnInstall()");
			// No hardware or install failed 
			UnconditionalUnInstall(hMsi);
			rv = ERROR_DEVICE_NOT_CONNECTED;
		}
	}
	if (GetDriverVersion() && !bUpdate)
	{
		// driver is installed on *imbdrv and finds its HW and this is not an update 
		IncrementShareCount();
	}

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T(""), _T("rv=0x%08x"), rv));
	return rv;
}

static void IncrementShareCount()
{
	DWORD value;
	DWORD cnt = sizeof(value);;
	HKEY hKey;
	TCHAR ImagePath[MAX_PATH + 100];

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FENTER, _T(""), _T("")));

	(void)GetWindowsDirectory(ImagePath, MAX_PATH);
	_tcscat_s(ImagePath, sizeof(ImagePath), _T("\\system32\\drivers\\imbdrv.sys"));

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDlls"), 0, KEY_ALL_ACCESS | KEY_WOW64_32KEY, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hKey, ImagePath, NULL, NULL, (LPBYTE)&value, &cnt) == ERROR_SUCCESS)
			value++;
		else
			value = 1;
		RegSetValueEx(hKey, ImagePath, 0, REG_DWORD, (const BYTE *)&value, sizeof(value));
		RegCloseKey(hKey);
	}

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T(""), _T("")));
}

static unsigned DecrementShareCount()
{
	DWORD value = 1;
	DWORD cnt = sizeof(value);;
	HKEY hKey;
	TCHAR ImagePath[MAX_PATH + 100];

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FENTER, _T(""), _T("")));

	(void)GetWindowsDirectory(ImagePath, MAX_PATH);
	_tcscat_s(ImagePath, sizeof(ImagePath), _T("\\system32\\drivers\\imbdrv.sys"));

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDlls"), 0, KEY_ALL_ACCESS | KEY_WOW64_32KEY, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hKey, ImagePath, NULL, NULL, (LPBYTE)&value, &cnt) == ERROR_SUCCESS)
		{
			if (value) value--;
			if (value)
				RegSetValueEx(hKey, ImagePath, 0, REG_DWORD, (const BYTE *)&value, sizeof(value));
			else
				RegDeleteValue(hKey, ImagePath);
		}
		else
		{
			// No entry in registry, better not remove the driver
			value = 1;
		}
		RegCloseKey(hKey);
	}

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T(""), _T("")));
	return value;
}
static void DeleteShareCount()
{
	HKEY hKey;

	TCHAR ImagePath[MAX_PATH + 100];
	(void)GetWindowsDirectory(ImagePath, MAX_PATH);
	_tcscat_s(ImagePath, sizeof(ImagePath), _T("\\system32\\drivers\\imbdrv.sys"));

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDlls"), 0, KEY_ALL_ACCESS | KEY_WOW64_32KEY, &hKey) == ERROR_SUCCESS)
	{
		RegDeleteValue(hKey, ImagePath);
		RegCloseKey(hKey);
	}
}

static UINT UnconditionalUnInstall(MSIHANDLE hMsi)
{
	UINT rv;

	// When the property exists then it holds an OS version number so it is not empty.
	Is64bit = VersionNT64 ? true : false;

	TCHAR exe[MAX_PATH+100];

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FENTER, _T(""), _T("")));

	_tcscpy_s(exe, sizeof(exe), DVDRIVERPATH);
	_tcscat_s(exe, sizeof(exe), (Is64bit) ? _T("\\amd64") : _T("\\i386"));
	_tcscat_s(exe, sizeof(exe), _T("\\RmPnpDr.exe"));

	LogString(hMsi, "InIMBDRV.cpp: calling RunMyUninstallProcess()");
	rv = RunMyUninstallProcess(hMsi, exe);

	char s[120];
	if (rv )
	{
		sprintf_s(s, sizeof(s), DLL_NAME": RmPnpDr.exe returned %08x", rv);
		LogString(hMsi, s);
	}
#if 1
	else
	{
		TCHAR ImagePath[MAX_PATH + 100];
		(void)GetWindowsDirectory(ImagePath, MAX_PATH);
		_tcscat_s(ImagePath, sizeof(ImagePath), _T("\\system32\\drivers\\imbdrv.sys"));
		DeleteFile(ImagePath);
		DeleteShareCount();
	}
#endif
	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T(""), _T("rv=0x%08x"), rv));
	return rv;
}

#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllexport) UINT GetDllParameterVersion()
{
	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FENTER, _T(""), _T("")));
	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T(""), _T("ret=%d"), 1));
	return 1;
}
__declspec(dllexport) UINT Install(MSIHANDLE hMsi, TCHAR *pVersionNT64, TCHAR *pDVDRIVERPATH, BOOL bSystemguard)
{
	UINT rv = ERROR_SUCCESS;
	char s[120];

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FENTER, _T(""), _T("bSystemguard: %d"), bSystemguard));

	SetGlobals(pVersionNT64, pDVDRIVERPATH);

	rv = Install_or_Update(hMsi, false);
	sprintf_s(s, sizeof(s), DLL_NAME": Install(): return value = %08x ", rv);
	LogString(hMsi, s);

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T(""), _T("rv=0x%08x"), rv));
	return rv;
}
__declspec(dllexport) UINT Update(MSIHANDLE hMsi, TCHAR *pVersionNT64, TCHAR *pDVDRIVERPATH, BOOL bSystemguard)
{
	UINT rv = ERROR_SUCCESS;
	char s[120];
	
	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FENTER, _T(""), _T("bSystemguard: %d"), bSystemguard));

	SetGlobals(pVersionNT64, pDVDRIVERPATH);

	rv = Install_or_Update(hMsi, true);
	sprintf_s(s, sizeof(s), DLL_NAME": Update(): return value = %08x ", rv);
	LogString(hMsi, s);

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T(""), _T("rv=0x%08x"), rv));
	return rv;
}
__declspec(dllexport) UINT UnInstall(MSIHANDLE hMsi, TCHAR *pVersionNT64, TCHAR *pDVDRIVERPATH, BOOL bSystemguard)
{
	UINT rv = ERROR_SUCCESS;
	char s[120];

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FENTER, _T(""), _T("bSystemguard: %d"), bSystemguard));

#ifdef ROOT_DEVICE_ID
	SetGlobals(pVersionNT64, pDVDRIVERPATH);
	unsigned count = DecrementShareCount();
	sprintf_s(s, sizeof(s), DLL_NAME": UnInstall(): DecrementShareCount() returned %d ", count);
	LogString(hMsi, s);
	if (count == 0)
	{
		rv = UnconditionalUnInstall(hMsi);
	}
#endif
	sprintf_s(s, sizeof(s), DLL_NAME": UnInstall(): return value = %08x ", rv);
	LogString(hMsi, s);

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T(""), _T("rv=0x%08x"), rv));
	return rv;
}

#ifdef __cplusplus
}
#endif
