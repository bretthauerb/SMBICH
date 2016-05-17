#include <tchar.h>
#include <Msiquery.h>
#include <stdio.h>
#include <string.h>
#include <winioctl.h>
#include <devguid.h>
#include <setupapi.h>
#include "CCOpusTrace.h"
#include <new>
using namespace std;

#define MAX_CLASS_NAME_LEN	32

#ifndef HAS_CAT_FILE_I386
#ifdef NO_VERSION_CHECK_I386
#define HAS_CAT_FILE_I386 NO_VERSION_CHECK_I386
#endif
#endif

#ifndef HAS_CAT_FILE_AMD64
#ifdef NO_VERSION_CHECK_AMD64
#define HAS_CAT_FILE_AMD64 NO_VERSION_CHECK_AMD64
#endif
#endif

#pragma warning ( disable : 4100)	// unreferenced formal parameter

static void IncrementShareCount();
static UINT UnconditionalUnInstall(MSIHANDLE hMsi);

typedef UINT (__cdecl InstallerEntry_T)(MSIHANDLE,TCHAR*version,TCHAR*dvdriver,BOOL);

typedef MSIHANDLE (__stdcall MsiCreateRecord_T)(unsigned);
typedef UINT (__stdcall MsiRecordSetStringA_T)(MSIHANDLE, unsigned, LPCTSTR);
typedef	int (__stdcall MsiProcessMessage_T)(MSIHANDLE, INSTALLMESSAGE, MSIHANDLE);
typedef	UINT (__stdcall MsiCloseHandle_T)(MSIHANDLE);


static TCHAR * VersionNT64 = NULL; 
static TCHAR * DVDRIVERPATH = _T(".");

static void SetGlobals(TCHAR *pVersionNT64, TCHAR *pDVDRIVERPATH)
{
	if (pVersionNT64 && _tcslen(pVersionNT64))
		VersionNT64 = pVersionNT64;
	if (pDVDRIVERPATH && _tcslen(pDVDRIVERPATH))
		DVDRIVERPATH = pDVDRIVERPATH;
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

	/*
	 *	Open device, test hardware available and get version
	 */
	HANDLE hF = CreateFile(_T(DEVICE_NAME),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hF != INVALID_HANDLE_VALUE) {
		ULONG ulPresent = 0;
		DWORD dwBytesCount;

// When MS decides that I cannot pass a sizeof(blah) to DeviceIocontrol
// then I refuse to accept this warning.
#pragma warning (disable:4245) // conversion from 'int' to 'DWORD', signed/unsigned mismatch
		DeviceIoControl(hF, IOCTL_FSC_HARDWARE_PRESENT,
								NULL, 0,
								&ulPresent, sizeof(ulPresent),
								&dwBytesCount,
								NULL);
		if (ulPresent)
			DeviceIoControl(hF, IOCTL_FSC_GET_DRIVER_VERSION,
									NULL, 0,
									&DriverVersion, sizeof(DriverVersion),
									&dwBytesCount,
									NULL);
		CloseHandle(hF);
	}
	else
	{
		TRC_OUTTV2((TRC_LEVEL_ERROR, TRC_TYPE_FINTERNAL, _T("CreateFile failed:"), _T("err=%x"), GetLastError()));
	}

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T(""), _T("DriverVersion=%d"), DriverVersion));
	return DriverVersion;
}

#ifdef ROOT_DEVICE_ID
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
#endif

// Should return 0		Success
//				 ERROR_SUCCESS_REBOOT_REQUIRED
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

#ifdef ROOT_DEVICE_ID
	_tcscat_s(cmd, sizeof(cmd), _T(" "));
	_tcscat_s(cmd, sizeof(cmd), ROOT_DEVICE_ID);
#endif

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

	// When the property exists then it holds an OS version number so it is not empty.
	BOOL Is64bit = VersionNT64 ? true : false;

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FENTER, _T(""), _T("hMsi=0x%08x, bUpdate: %d"), hMsi, bUpdate));

#if HAS_CAT_FILE_AMD64
	if (Is64bit) DoVersionCheck = FALSE;
	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FENTER, _T("No VersionCheck for 64bit needed"), _T("")));
#endif
#if HAS_CAT_FILE_I386
	if (!Is64bit) DoVersionCheck = FALSE;
	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FENTER, _T("No VersionCheck for 32bit needed"), _T("")));
#endif

	CurrentVersion = GetDriverVersion();
	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FINTERNAL, _T("CurrentVersion"), _T("%d"), CurrentVersion));

#ifdef SHARE_NAME
	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FINTERNAL, _T("SHARE_NAME"), _T("%s"), SHARE_NAME));
	if (CurrentVersion == 0)
	{
		TCHAR ImagePath[MAX_PATH + 100];
		(void)GetWindowsDirectory(ImagePath, MAX_PATH);
		_tcscat_s(ImagePath, sizeof(ImagePath), _T("\\system32\\drivers\\"));
		_tcscat_s(ImagePath, sizeof(ImagePath), _T(SHARE_NAME));
		TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FINTERNAL, _T("ImagePath"), _T("%s"), ImagePath));
		if (!DeleteFile(ImagePath))
			TRC_OUTTV2((TRC_LEVEL_ERROR, TRC_TYPE_FINTERNAL, _T("DeleteFile failed"), _T("%s %d"), ImagePath, GetLastError()));

	}
#endif

	// Need to update driver ?
	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FINTERNAL, _T("MAJOR_VERSION"), _T("%d"), MAJOR_VERSION));
	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FINTERNAL, _T("MINOR_VERSION"), _T("%d"), MINOR_VERSION));
	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FINTERNAL, _T("RELEASE"), _T("%d"), RELEASE));
	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FINTERNAL, _T("DRIVER_VERSION"), _T("%d"), DRIVER_VERSION));
	if (!DoVersionCheck || CurrentVersion < DRIVER_VERSION)
	{
        TCHAR s[MAX_PATH + 1024];
        _tcscpy_s(s, _countof(s), DLL_NAME);
        _tcscat_s(s, _countof(s), _T(": doing UpdPnpDr.exe "));
        _tcscat_s(s, _countof(s), INF_NAME);

        LogString(hMsi, s);
		TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FINTERNAL, s, _T("")));
		
		TCHAR exe[MAX_PATH+100];
		_tcscpy_s(exe, _countof(exe), DVDRIVERPATH);
		_tcscat_s(exe, sizeof(exe), (Is64bit) ? _T("\\amd64") : _T("\\i386"));
		_tcscat_s(exe, sizeof(exe), _T("\\UpdPnpDr.exe"));

		TCHAR inf[MAX_PATH+100];
		_tcscpy_s(inf, sizeof(inf), DVDRIVERPATH);
		_tcscat_s(inf, sizeof(inf), (Is64bit) ? _T("\\amd64") : _T("\\i386"));
		_tcscat_s(inf, sizeof(inf), _T("\\"INF_NAME));

#ifdef SHARE_NAME
		TCHAR sys[MAX_PATH+100];
		_tcscpy_s(sys, sizeof(sys), DVDRIVERPATH);
		_tcscat_s(sys, sizeof(sys), (Is64bit) ? _T("\\amd64") : _T("\\i386"));
		_tcscat_s(sys, sizeof(sys), _T("\\"SHARE_NAME));
#endif
		TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FINTERNAL, _T("INF file "), _T("%s"), inf));
		HANDLE hF = CreateFile(inf,
                   GENERIC_READ,          // open for reading
                   0,       // no share for reading
//                   FILE_SHARE_READ,       // share for reading
                   NULL,                  // default security
                   OPEN_EXISTING,         // existing file only
                   FILE_ATTRIBUTE_NORMAL, // normal file
                   NULL);                 // no attr. template
		if (hF == INVALID_HANDLE_VALUE)
		{
			rv = ERROR_FILE_NOT_FOUND;
			sprintf_s(s, sizeof(s), DLL_NAME": cannot open inf file %s", inf);
			LogString(hMsi, s);
			TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FINTERNAL, _T("DLL file: cannot open inf file"), _T("%s::%s"), DLL_NAME, inf));
		}
		else
		{
			sprintf_s(s, sizeof(s), DLL_NAME": inf file %s exists", inf);
			LogString(hMsi, s);
			TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FINTERNAL, _T("DLL file: inf file exists"), _T("%s::%s"), DLL_NAME, inf));
// lousy programming style
#ifdef SHARE_NAME
			CloseHandle(hF);
			hF = CreateFile(sys,
                   GENERIC_READ,          // open for reading
                   0,       // no share for reading
//                   FILE_SHARE_READ,       // share for reading
                   NULL,                  // default security
                   OPEN_EXISTING,         // existing file only
                   FILE_ATTRIBUTE_NORMAL, // normal file
                   NULL);                 // no attr. template
			if (hF == INVALID_HANDLE_VALUE)
			{
				rv = ERROR_FILE_NOT_FOUND;
				sprintf_s(s, sizeof(s), DLL_NAME": cannot open sys file %s", sys);
				LogString(hMsi, s);
				TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FINTERNAL, _T("DLL file: cannot open sys file"), _T("%s::%s"), DLL_NAME, sys));
			}
			else
			{
				sprintf_s(s, sizeof(s), DLL_NAME": sys file %s exists", sys);
				LogString(hMsi, s);
				TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FINTERNAL, _T("DLL file: sys file exists"), _T("%s::%s"), DLL_NAME, sys));
#else
			{
#endif
				CloseHandle(hF);

				rv = RunMyInstallProcess(hMsi, exe, DoVersionCheck, inf);

				sprintf_s(s, sizeof(s), DLL_NAME": UpdPnpDr.exe returned %08x", rv);
				LogString(hMsi, s);
			}
		}
	}
	else
		LogString(hMsi, DLL_NAME": GetDriverVersion() indicates driver is uptodate");

#ifdef ROOT_DEVICE_ID
	if (rv == ERROR_SUCCESS || rv == ERROR_SUCCESS_REBOOT_REQUIRED)
	{
		if (GetDriverVersion() == 0)
		{
			LogString(hMsi, DLL_NAME": driver finds no hardware, calling UnconditionalUnInstall()");
			// No hardware or install failed, cannot remove it here when reboot required 
			if (rv != ERROR_SUCCESS_REBOOT_REQUIRED) UnconditionalUnInstall(hMsi);
			rv = ERROR_DEVICE_NOT_CONNECTED;
		}
	}
	if (GetDriverVersion() && !bUpdate)
	{
		// driver is present (for whatever reason) and finds its HW and this is not an update 
		IncrementShareCount();
	}
#endif

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T("returns"), _T("%d"), rv));
	return rv;
}

#ifndef ROOT_DEVICE_ID
static bool Migrate(MSIHANDLE, bool)
{
	return false;
}
#endif

#ifdef ROOT_DEVICE_ID

// Stuff to migrate from old driver installed with CreateService to new driver installed with inf file.
// The old driver is removed and the share counter renamed

//
// Control service to reach STATUS_STOPPED or STATUS_RUNNING
//
static DWORD ChangeServiceStatus(SC_HANDLE hS, SERVICE_STATUS_PROCESS *pStatus, DWORD want)
{
	DWORD cnt;
	int loop = 100;

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FENTER, _T(""), _T("hS=0x%08x, pStatus=%p, want: %d"), hS, pStatus, want));

	while (loop && QueryServiceStatusEx(hS, SC_STATUS_PROCESS_INFO, (LPBYTE)pStatus, sizeof(*pStatus), &cnt))
	{
		if (pStatus->dwCurrentState == want)
		{
			TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T(""), _T("%d"), ERROR_SUCCESS));
			return ERROR_SUCCESS;
		}
		if (want == SERVICE_STOPPED && pStatus->dwCurrentState == SERVICE_RUNNING)
		{
			SERVICE_STATUS x;
			if (!ControlService(hS, SERVICE_CONTROL_STOP, &x)) return GetLastError();
			(void)QueryServiceStatusEx(hS, SC_STATUS_PROCESS_INFO, (LPBYTE)pStatus, sizeof(*pStatus), &cnt);
		}
		if (want == SERVICE_RUNNING && pStatus->dwCurrentState == SERVICE_STOPPED)
		{
			if (!StartService(hS, NULL, NULL)) return GetLastError();
			(void)QueryServiceStatusEx(hS, SC_STATUS_PROCESS_INFO, (LPBYTE)pStatus, sizeof(*pStatus), &cnt);
		}
		loop--;
		Sleep(100);
	}

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T(""), _T("%d"), ERROR_INTERNAL_ERROR));
	return ERROR_INTERNAL_ERROR;
}

static void IncrementShareCount()
{
	DWORD value;
	DWORD cnt = sizeof(value);;
	HKEY hKey;
	TCHAR ImagePath[MAX_PATH + 100];

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FENTER, _T(""), _T("")));

	(void)GetWindowsDirectory(ImagePath, MAX_PATH);
	_tcscat_s(ImagePath, sizeof(ImagePath), _T("\\system32\\drivers\\"));
	_tcscat_s(ImagePath, sizeof(ImagePath), _T(SHARE_NAME));

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
	_tcscat_s(ImagePath, sizeof(ImagePath), _T("\\system32\\drivers\\"));
	_tcscat_s(ImagePath, sizeof(ImagePath), _T(SHARE_NAME));

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDlls"), 0, KEY_ALL_ACCESS | KEY_WOW64_32KEY, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hKey, ImagePath, NULL, NULL, (LPBYTE)&value, &cnt) == ERROR_SUCCESS)
		{
			TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FINTERNAL, _T("SharedDLL-counter"), _T("%d"), value));
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
			TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FINTERNAL, _T("No SharedDLL-counter exists --> counter"), _T("%d"), value));
		}
		RegCloseKey(hKey);
	}

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T(""), _T("%d"), value));
	return value;
}


// Open service and if start type AUTO_START then it is a service installed with CreateService: Remove it.
// Return true when something went wrong.
static bool Migrate(MSIHANDLE hMsi, bool bUpdate)
{
	bool rv = false;

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FENTER, _T(""), _T("hMsi=0x%08x, bUpdate: %d"), hMsi, bUpdate));

	SC_HANDLE hSCm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hSCm)
	{
		SC_HANDLE hSCs = OpenService(hSCm, _T(OLD_SERVICE_NAME), SERVICE_ALL_ACCESS);
		if (hSCs) 
		{
			QUERY_SERVICE_CONFIG ServiceConfig;
			memset(&ServiceConfig, 0, sizeof(ServiceConfig));
			DWORD dBytesNeeded;
			if (QueryServiceConfig(hSCs, &ServiceConfig, sizeof(ServiceConfig), &dBytesNeeded ) == ERROR_SUCCESS)
			{
				if (ServiceConfig.dwStartType == SERVICE_AUTO_START)
				{
					LogString(hMsi, DLL_NAME ": migrate() is going to DeleteService(" OLD_SERVICE_NAME ")");
					SERVICE_STATUS_PROCESS status;
					ChangeServiceStatus(hSCs, &status, SERVICE_STOPPED);
					if (status.dwCurrentState == SERVICE_STOPPED)
					{
						if (!DeleteService(hSCs))
						{
							LogString(hMsi, DLL_NAME ": migrate() could not delete service " OLD_SERVICE_NAME);
							ChangeServiceStatus(hSCs, &status, SERVICE_RUNNING);
							rv = true;
						}
					}
					else
					{
						LogString(hMsi, DLL_NAME ": migrate() could not stop service " OLD_SERVICE_NAME);
						rv = true;
					}
					if (rv)
					{
						// special case, use old driver version but increment share count
						if (!bUpdate) IncrementShareCount();
					}
				}
			}
		}
		if (hSCs) CloseServiceHandle(hSCs);
		CloseServiceHandle(hSCm);
	}

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T(""), _T("rv=0x%08x"), rv));
	return rv;
}

static UINT UnconditionalUnInstall(MSIHANDLE hMsi)
{
	UINT rv;

	// When the property exists then it holds an OS version number so it is not empty.
	BOOL Is64bit = VersionNT64 ? true : false;

	TCHAR exe[MAX_PATH+100];

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FENTER, _T(""), _T("")));

	_tcscpy_s(exe, sizeof(exe), DVDRIVERPATH);
	_tcscat_s(exe, sizeof(exe), (Is64bit) ? _T("\\amd64") : _T("\\i386"));
	_tcscat_s(exe, sizeof(exe), _T("\\RmPnpDr.exe"));

	LogString(hMsi, "GenericInstallPnP.h: " DLL_NAME " calling RunMyUninstallProcess()");
	rv = RunMyUninstallProcess(hMsi, exe);

#if 1
	if (rv == 0)
	{
		TCHAR ImagePath[MAX_PATH + 100];
		(void)GetWindowsDirectory(ImagePath, MAX_PATH);
		_tcscat_s(ImagePath, sizeof(ImagePath), _T("\\system32\\drivers\\"));
		_tcscat_s(ImagePath, sizeof(ImagePath), _T(SHARE_NAME));
		DeleteFile(ImagePath);
	}
#endif

	char s[120];
	sprintf_s(s, sizeof(s), DLL_NAME": RmPnpDr.exe returned %08x", rv);
	LogString(hMsi, s);

	TRC_OUTTV2((TRC_LEVEL_INFO, TRC_TYPE_FLEAVE, _T(""), _T("rv=0x%08x"), rv));
	return rv;
}
#endif

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

	if (!Migrate(hMsi, false))
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
	if (!Migrate(hMsi, true))
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

#ifdef REBOOT_UNINSTALL
		rv = ERROR_SUCCESS_REBOOT_REQUIRED;
#endif
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