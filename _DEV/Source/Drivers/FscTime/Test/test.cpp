// Test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <winioctl.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>

#include "FSCIoctl.h"
#include "../Driver/Sources/FscTime.h"

int main(int argc, char* argv[])
{
	DWORD dwBytesCount;

	HANDLE hF = CreateFile("\\\\.\\FSC_TIME",
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hF == INVALID_HANDLE_VALUE) {
		printf("No driver ?\n");
		exit(1);
	}

	// IOCTL_FSC_HARDWARE_PRESENT
	ULONG ulPresent = 0;
	dwBytesCount = 0;
	BOOLEAN bRetVal = DeviceIoControl(hF, IOCTL_FSC_HARDWARE_PRESENT,
									NULL, 0,
									&ulPresent, sizeof(ulPresent),
									&dwBytesCount,
									NULL);

	printf("FscTime driver present: %d(%d)\n", ulPresent, dwBytesCount);

	// IOCTL_CMOSI_GET_DRIVER_VERSION
	ULONG ulVersion = 0;
	dwBytesCount = 0;
	bRetVal = DeviceIoControl(hF, IOCTL_FSC_GET_DRIVER_VERSION,
									NULL, 0,
									&ulVersion, sizeof(ulVersion),
									&dwBytesCount,
									NULL);

	printf("FscTime driver version: %x(%d)\n", ulVersion, dwBytesCount);

	// Test: exit without closing handle.
	// handle will be closed by OS: is a different thread.
	//	CloseHandle(hF);

	return 0;
}

