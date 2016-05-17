//
// GenericInstall.h
//
// Generic code to install a driver from an MSI program.
//
//


#include <stdio.h>
#include <string.h>

#include <new>
//#include <new.h>
using namespace std;


#pragma warning ( disable : 4100)	// unreferenced formal parameter
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


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
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
    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif

static void DeleteShareCount(TCHAR *name)
{
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDlls"), 0, KEY_ALL_ACCESS | KEY_WOW64_32KEY, &hKey) == ERROR_SUCCESS)
	{
		RegDeleteValue(hKey, name);
		RegCloseKey(hKey);
	}
}
static unsigned DecrementShareCount(TCHAR *name)
{
	DWORD value = 1;
	DWORD cnt = sizeof(value);;
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDlls"), 0, KEY_ALL_ACCESS | KEY_WOW64_32KEY, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hKey, name, NULL, NULL, (LPBYTE)&value, &cnt) == ERROR_SUCCESS)
		{
			if (value) value--;
			if (value)
				RegSetValueEx(hKey, name, 0, REG_DWORD, (const BYTE *)&value, sizeof(value));
			else
				RegDeleteValue(hKey, name);
		}
		else
		{
			// No entry in registry, better not remove the driver
			value = 1;
		}
		RegCloseKey(hKey);
	}
	return value;
}
static void IncrementShareCount(TCHAR *name)
{
	DWORD value;
	DWORD cnt = sizeof(value);;
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDlls"), 0, KEY_ALL_ACCESS | KEY_WOW64_32KEY, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hKey, name, NULL, NULL, (LPBYTE)&value, &cnt) == ERROR_SUCCESS)
			value++;
		else
			value = 1;
		RegSetValueEx(hKey, name, 0, REG_DWORD, (const BYTE *)&value, sizeof(value));
		RegCloseKey(hKey);
	}
}

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
}

#ifndef NODRIVERVERSION
static DWORD GetDriverVersion(void)
{
	DWORD DriverVersion = 0;
	/*
	 *	Open device, test hardware available and get version
	 */
	HANDLE hF = CreateFile(_T(DRIVER_NAME),
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
	return DriverVersion;
}
#endif

//
// Control service to reach STATUS_STOPPED or STATUS_RUNNING
//
static DWORD ChangeServiceStatus(SC_HANDLE hS, SERVICE_STATUS_PROCESS *pStatus, DWORD want)
{
	DWORD cnt;
	int loop = 100;
	while (loop && QueryServiceStatusEx(hS, SC_STATUS_PROCESS_INFO, (LPBYTE)pStatus, sizeof(*pStatus), &cnt))
	{
		if (pStatus->dwCurrentState == want)
			return ERROR_SUCCESS;
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
	return ERROR_INTERNAL_ERROR;
}


// May call this with hMsi = NULL
static UINT MyInstall(MSIHANDLE hMsi, char *ImagePath, char *SourcePath, BOOL *started, BOOL bUpdate)
{
	UINT uError = ERROR_SUCCESS;
	BOOL Is64bit = FALSE;

	*started = FALSE;

#ifndef NODRIVERVERSION
	DWORD CurrentVersion = GetDriverVersion();
	if (CurrentVersion >= DRIVER_VERSION)
	{
		// Driver exists already in a newer version.
		LogString(hMsi, "GenericInstall.h: " SERVICE_NAME " is uptodate");
		*started = TRUE;
	}
	else
#endif
	{
		BOOL (__stdcall *wow64revert)(PVOID) = NULL;
		PVOID oldwow64 = NULL;
		HMODULE hL = LoadLibrary("Kernel32.dll");
		if (hL)
		{
			BOOL (__stdcall *wow64disable)(PVOID*) = (BOOL(__stdcall *)(PVOID*))GetProcAddress(hL, _T("Wow64DisableWow64FsRedirection"));
			wow64revert = (BOOL(__stdcall *)(PVOID))GetProcAddress(hL, _T("Wow64RevertWow64FsRedirection"));
			if (wow64disable && wow64revert)
			{
				if (wow64disable(&oldwow64))
					Is64bit = TRUE;
			}
		} 
		if (!Is64bit)
		{
			wow64revert = NULL;
		}

		// First try do delete the previous entry
		SC_HANDLE hSCm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (hSCm)
		{
			SC_HANDLE hSCs = OpenService(hSCm, _T(SERVICE_NAME), SERVICE_ALL_ACCESS);
			if (hSCs) 
			{
				SERVICE_STATUS_PROCESS status;
				ChangeServiceStatus(hSCs, &status, SERVICE_STOPPED);
				if (status.dwCurrentState == SERVICE_STOPPED)
				{
					if (DeleteService(hSCs))
					{
						DeleteFile(ImagePath);
					}
					else
					{
						LogString(hMsi, "GenericInstall.h: Could not delete service " SERVICE_NAME);
						// Don't know how to repair this
					}
				}
				else
				{
					LogString(hMsi, "GenericInstall.h: Could not stop service " SERVICE_NAME);
					// special case, use old driver version but increment share count
					if (!bUpdate) IncrementShareCount(ImagePath);
					CloseServiceHandle(hSCs);
					CloseServiceHandle(hSCm);
					return ERROR_SUCCESS;
				}
				CloseServiceHandle(hSCs);
				hSCs = NULL;
			}

			for (int x = 1000; !hSCs && x; x--)
			{
				hSCs = CreateService(hSCm, _T(SERVICE_NAME), _T(SERVICE_NAME),
					SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
// TODO: maybe SERVICE_SYSTEM_START ????
//					SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_SYSTEM_START, SERVICE_ERROR_NORMAL,
					ImagePath, _T("base"), NULL, NULL, NULL, NULL);
				// I don't know ....
				if (!hSCs && GetLastError() == 1072)	// Marked for deletion
					Sleep(10);
			}
			if (hSCs)
			{
				if (!CopyFile(SourcePath, ImagePath , FALSE))
				{
					char s[999];
					sprintf_s(s, sizeof(s), "GenericInstall.h: CopyFile(SourcePath = %s, ImagePath = %s) failed", SourcePath, ImagePath);
					LogString(hMsi, s);
				}
				SERVICE_STATUS_PROCESS status;
				ChangeServiceStatus(hSCs, &status, SERVICE_RUNNING);
				if (status.dwCurrentState == SERVICE_RUNNING)
				{
#ifndef NODRIVERVERSION
					DWORD NewDriverVersion = GetDriverVersion();
					// Test hardware available. when not available: stop and delete service
					if (NewDriverVersion == 0)
					{
						if (!ChangeServiceStatus(hSCs, &status, SERVICE_STOPPED))
						{
							// even if someone was still using it, sorry its gone .....
							DeleteService(hSCs);
							DeleteFile(ImagePath);
							DeleteShareCount(ImagePath);
						}
					}
					else
#endif
					{
						// Hey, it works
						*started = TRUE;
#ifndef NODRIVERVERSION
						// For convenence put current driver version in registry
						HKEY hKey;
						if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Services\\"SERVICE_NAME), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
						{
							RegSetValueEx(hKey, _T("DriverVersion"), 0, REG_DWORD, (const BYTE *)&NewDriverVersion, sizeof(DWORD));
							RegCloseKey(hKey);
						}
#endif
					}
				}
				else
				{
					char s[999];
					uError = GetLastError();
					sprintf_s(s, sizeof(s), "GenericInstall.h: Could not start service " SERVICE_NAME "; GetLastError() = %d", uError);
					LogString(hMsi, s);

					// even if someone was still using it, sorry its gone .....
					DeleteService(hSCs);
					DeleteFile(ImagePath);
					DeleteShareCount(ImagePath);
				}
				CloseServiceHandle(hSCs);
			}
			else
			{
				LogString(hMsi, "GenericInstall.h: CreateService() failed when installing service " SERVICE_NAME);
				uError = GetLastError();
			}
			CloseServiceHandle(hSCm);
		}
		else
		{
			LogString(hMsi, "GenericInstall.h: OpenSCManager() failed when installing " SERVICE_NAME);
			uError = GetLastError();
		}
		if (wow64revert) wow64revert(oldwow64);
		if (hL) FreeLibrary(hL);
		if (*started) LogString(hMsi, "GenericInstall.h: installed " SERVICE_NAME);
	}

	if (*started)
	{
		uError = ERROR_SUCCESS;
		if (!bUpdate) IncrementShareCount(ImagePath);
	}
	else if (!uError) uError = ERROR_SERVICE_NEVER_STARTED;
	
	return uError;
}

static UINT MyUnInstall(char *ImagePath)
{
	UINT uRetValue = ERROR_SUCCESS;
	if (!DecrementShareCount(ImagePath))
	{
		BOOL (__stdcall *wow64revert)(PVOID) = NULL;
		PVOID oldwow64 = NULL;
		HMODULE hL = LoadLibrary("Kernel32.dll");
		if (hL)
		{
			BOOL (__stdcall *wow64disable)(PVOID*) = (BOOL(__stdcall *)(PVOID*))GetProcAddress(hL, _T("Wow64DisableWow64FsRedirection"));
			wow64revert = (BOOL(__stdcall *)(PVOID))GetProcAddress(hL, _T("Wow64RevertWow64FsRedirection"));
			if (wow64disable && wow64revert)
			{
				wow64disable(&oldwow64);
			}
			else
			{
				wow64revert = NULL;
			}
		} 

		SC_HANDLE hSCm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (hSCm)
		{
			SC_HANDLE hSCs = OpenService(hSCm, _T(SERVICE_NAME), SERVICE_ALL_ACCESS);
			if (hSCs)
			{
				SERVICE_STATUS_PROCESS status;
				ChangeServiceStatus(hSCs, &status, SERVICE_STOPPED);
				if (!DeleteService(hSCs))
					uRetValue = GetLastError();
				else
				{
					DeleteFile(ImagePath);
				}
				CloseServiceHandle(hSCs);
			}
			else
				uRetValue = GetLastError();
			CloseServiceHandle(hSCm);
		}
		else
			uRetValue = GetLastError();
		if (wow64revert) wow64revert(oldwow64);
		if (hL) FreeLibrary(hL);
	}
	return uRetValue;
}

static UINT Install_or_Update(MSIHANDLE hMsi, BOOL bUpdate)
{
	UINT uRetValue = ERROR_SUCCESS;
	BOOL started = FALSE;

	// When the property exists then it holds an OS version number so it is not empty.
	BOOL Is64bit = VersionNT64 ? true : false;

	TCHAR SourcePath[MAX_PATH + 100];
	_tcscpy_s(SourcePath, sizeof(SourcePath), DVDRIVERPATH);
	_tcscat_s(SourcePath, sizeof(SourcePath), (Is64bit) ? _T("\\amd64\\") : _T("\\i386\\"));
	_tcscat_s(SourcePath, sizeof(SourcePath), _T(SERVICE_IMAGE));

	// C:\\WINDOWS\System32\drivers\FscEfDmi.sys
	TCHAR ImagePath[MAX_PATH + 100];
	(void)GetWindowsDirectory(ImagePath, MAX_PATH);
	_tcscat_s(ImagePath, sizeof(ImagePath), _T("\\system32\\drivers\\"));
	_tcscat_s(ImagePath, sizeof(ImagePath), _T(SERVICE_IMAGE));

	uRetValue = MyInstall(hMsi, ImagePath, SourcePath, &started, bUpdate);

	if (!started)
	{
		char s[120];
		sprintf_s(s, sizeof(s), SERVICE_NAME": MyInstall()  uRetValue = %x, started = %x", uRetValue, started);
		LogString(hMsi, s);
	}
	else
	{
		uRetValue = ERROR_SUCCESS;
	}

	return uRetValue;
}

#ifdef __cplusplus
extern "C" {
#endif
__declspec(dllexport) UINT GetDllParameterVersion()
{
	return 1;
}
__declspec(dllexport) UINT Install(MSIHANDLE hMsi, TCHAR *pVersionNT64, TCHAR *pDVDRIVERPATH, BOOL bSystemguard)
{
	SetGlobals(pVersionNT64, pDVDRIVERPATH);
	return Install_or_Update(hMsi, false);
}
__declspec(dllexport) UINT Update(MSIHANDLE hMsi, TCHAR *pVersionNT64, TCHAR *pDVDRIVERPATH, BOOL bSystemguard)
{
	SetGlobals(pVersionNT64, pDVDRIVERPATH);
	return Install_or_Update(hMsi, true);
}
__declspec(dllexport) UINT UnInstall(MSIHANDLE hMsi, TCHAR *pVersionNT64, TCHAR *pDVDRIVERPATH, BOOL bSystemguard)
{
	SetGlobals(pVersionNT64, pDVDRIVERPATH);

	TCHAR ImagePath[MAX_PATH + 100];

	// C:\\WINDOWS\System32\drivers\FscEfDmi.sys
	(void)GetWindowsDirectory(ImagePath, MAX_PATH);
	_tcscat_s(ImagePath, sizeof(ImagePath), _T("\\system32\\drivers\\"));
	_tcscat_s(ImagePath, sizeof(ImagePath), _T(SERVICE_IMAGE));

	UINT uRetValue = MyUnInstall(ImagePath);

	return uRetValue;
}
#ifdef __cplusplus
}
#endif