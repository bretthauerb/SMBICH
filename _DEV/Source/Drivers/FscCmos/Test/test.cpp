// Test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <winioctl.h>
#include <time.h>
#include <stdlib.h>
#include "FSCIoctl.h"
#include "../Driver/Sources/fsccmos.h"

void x(UCHAR *p, int c)
{
	while (c > 0)
	{
		for (int i=0; i<16; i++)
		{
			printf("%02x ", *p++);
			c--;
		}
		printf("\n");
	}
}
int main(int argc, char* argv[])
{
	unsigned long Offset; char Byte2Write;
	if (argc==3)
	{
		Offset = strtol(argv[1], NULL, 16);
		Byte2Write = (char)strtol(argv[2], NULL, 16);
	}
	else
		printf("Usage: test register data\n");

	DWORD dwBytesCount;

	HANDLE hCmos = CreateFile(CmosDriverName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hCmos == INVALID_HANDLE_VALUE) {
		printf("No driver ?\n");
		exit(1);
	}

	// IOCTL_CMOS_PRESENT
	ULONG ulPresent = 0;
	dwBytesCount = 0;
	BOOLEAN bRetVal = DeviceIoControl(hCmos, IOCTL_FSC_HARDWARE_PRESENT,
									NULL, 0,
									&ulPresent, sizeof(ulPresent),
									&dwBytesCount,
									NULL);

	printf("CMOS driver present: %d(%d)\n", ulPresent, dwBytesCount);

	// IOCTL_CMOSI_GET_DRIVER_VERSION
	ULONG ulVersion = 0;
	dwBytesCount = 0;
	bRetVal = DeviceIoControl(hCmos, IOCTL_FSC_GET_DRIVER_VERSION,
									NULL, 0,
									&ulVersion, sizeof(ulVersion),
									&dwBytesCount,
									NULL);

	printf("CMOS driver version: %x(%d)\n", ulVersion, dwBytesCount);
	

	//
	// IOCTL_CMOS_READ
	//
	CMOS_READ_T rd;
	rd.ulOffset = 0;
	UCHAR data[128];
	bRetVal = DeviceIoControl(hCmos, IOCTL_CMOS_READ,
									&rd, sizeof(rd), 
									data, sizeof(data),
									&dwBytesCount,
									NULL);

	printf("IOCTL_CMOS_READ\n");
	printf("bRetVal = %x; dwBytesCount = %d\n", bRetVal, dwBytesCount);
	x(data, 128);

	if (argc==3)
	{
		//
		// IOCTL_CMOS_WRITE
		//
		CMOS_WRITE_T* wr = (CMOS_WRITE_T*)malloc(sizeof CMOS_WRITE_T + 1);
		wr->ulOffset = Offset;
		wr->Data[0] = Byte2Write;
		bRetVal = DeviceIoControl(hCmos, IOCTL_CMOS_WRITE,
										wr, sizeof(wr) + 1, 
										NULL, 0,
										&dwBytesCount,
										NULL);

		printf("IOCTL_CMOS_WRITE: Offset=%d, Data=%d, bRetVal = %x\n", wr->ulOffset, wr->Data[0], bRetVal);

		bRetVal = DeviceIoControl(hCmos, IOCTL_CMOS_READ,
										&rd, sizeof(rd), 
										data, sizeof(data),
										&dwBytesCount,
										NULL);

		printf("IOCTL_CMOS_READ\n");
		printf("bRetVal = %x; dwBytesCount = %d\n", bRetVal, dwBytesCount);
		x(data, 128);
	}
	return 0;
}

