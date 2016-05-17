// Test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <winioctl.h>
//#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "fsccpuid.h"
#include "FSCIoctl.h"

int main(int argc, char* argv[])
{
	DWORD dwBytesCount;

	HANDLE hCpuid = CreateFile(CpuidDriverName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hCpuid == INVALID_HANDLE_VALUE) {
		printf("No driver ?\n");
		exit(1);
	}

	// IOCTL_BAPI_PRESENT
	ULONG ulPresent = 0;
	dwBytesCount = 0;
	BOOLEAN bRetVal = DeviceIoControl(hCpuid, IOCTL_FSC_HARDWARE_PRESENT,
									NULL, 0,
									&ulPresent, sizeof(ulPresent),
									&dwBytesCount,
									NULL);

	printf("CPUID driver present: %d(%d)\n", ulPresent, dwBytesCount);

	// IOCTL_BAPI_GET_DRIVER_VERSION
	ULONG ulVersion = 0;
	dwBytesCount = 0;
	bRetVal = DeviceIoControl(hCpuid, IOCTL_FSC_GET_DRIVER_VERSION,
									NULL, 0,
									&ulVersion, sizeof(ulVersion),
									&dwBytesCount,
									NULL);

	printf("CPUID driver version: %x(%d)\n", ulVersion, dwBytesCount);
	

	//
	// CPUID 0
	//
	CPUID_T id;
	ULONG fct = 0;
	dwBytesCount = 0;
	bRetVal = DeviceIoControl(hCpuid, IOCTL_CPUID_CPUID,
									&fct, sizeof(fct), 
									&id, sizeof(id),
									&dwBytesCount,
									NULL);

	printf("\nIOCTL_CPUID_CPUID 0\n");
	printf("bRetVal = %d, dwBytesCount = %d\n", bRetVal, dwBytesCount);
	printf("eax = %x\n", id.eax);
	CHAR s[13]; memset(s, 0, sizeof s);
	memcpy(s, id.id, sizeof(id.id));
	printf("id = '%s'\n", s);

	//
	// CPUID 1
	//
	fct = 1;
	dwBytesCount = 0;
	bRetVal = DeviceIoControl(hCpuid, IOCTL_CPUID_CPUID,
									&fct, sizeof(fct), 
									&id, sizeof(id),
									&dwBytesCount,
									NULL);

	printf("\nIOCTL_CPUID_CPUID 1\n");
	printf("bRetVal = %d, dwBytesCount = %d\n", bRetVal, dwBytesCount);
	printf("dwBytesCount = %d\n", dwBytesCount);
	printf("eax = %x\n", id.eax);

	fct = 1;
	dwBytesCount = 0;
	bRetVal = DeviceIoControl(hCpuid, IOCTL_CPUID_CPUID,
									&fct, sizeof(fct), 
									&id, sizeof(id),
									&dwBytesCount,
									NULL);

	//
	// RDMSR 0x1b
	//
	fct = 0x1b;
	MSR_T msr;
	dwBytesCount = 0;
	bRetVal = DeviceIoControl(hCpuid, IOCTL_CPUID_RDMSR,
									&fct, sizeof(fct), 
									&msr, sizeof(msr),
									&dwBytesCount,
									NULL);

	printf("\nIOCTL_CPUID_RDMSR 0x1b\n");
	printf("bRetVal = %d, dwBytesCount = %d\n", bRetVal, dwBytesCount);
	printf("eax = %x\n", msr.eax);
	printf("edx = %x\n", msr.edx);

#if 1
	printf("And now for CPU0..3\n");
	for (int i=0; i<4; i++)
	{
		struct {
			ULONG fct;
			ULONG nr;
		} x;
		x.fct = 0;
		x.nr = i;
		dwBytesCount = 0;
		bRetVal = DeviceIoControl(hCpuid, IOCTL_CPUID_CPUID,
										&x, sizeof(x), 
										&id, sizeof(id),
										&dwBytesCount,
										NULL);

		if (!bRetVal) break;
		printf("\nIOCTL_CPUID_CPUID[%d] 0\n", i);
		printf("bRetVal = %d, dwBytesCount = %d\n", bRetVal, dwBytesCount);
		printf("eax = %x\n", id.eax);
		CHAR s[13]; memset(s, 0, sizeof s);
		memcpy(s, id.id, sizeof(id.id));
		printf("id = '%s'\n", s);

	}
#endif
	return 0;
}

