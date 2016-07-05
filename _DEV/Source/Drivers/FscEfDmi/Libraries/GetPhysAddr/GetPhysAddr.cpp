// GetPhysAddr.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "GetPhysAddr.h"
#include "FscEfdmi.h"


// This is an example of an exported function.
extern "C" GETPHYSADDR_API BOOL __stdcall fnGetPhysAddr(void*va, ULONG *low, ULONG *high)
{
	HANDLE hF = CreateFile(EfdmiDriverName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hF == INVALID_HANDLE_VALUE)
		return false;

	ULONGLONG pa = 0;
	ULONGLONG lva = (ULONGLONG)va;

	DWORD dwBytesCount = 0;
	BOOLEAN bRetVal = DeviceIoControl(hF, IOCTL_EFDMI_GET_PHYSICAL_ADDRESS,
									&lva, sizeof(lva),
									&pa, sizeof(pa),
									&dwBytesCount,
									NULL);
	CloseHandle(hF);

	if (low)  *low = (ULONG)pa;
	if (high) *high = (ULONG)(pa >> 32);

	return true;
}
#if 0
// This is the constructor of a class that has been exported.
// see GetPhysAddr.h for the class definition
CGetPhysAddr::CGetPhysAddr()
{
	return;
}
#endif