// Test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <winioctl.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include "DMI.h"

#include "FSCIoctl.h"
#include "../Driver/Sources/FscEfdmi.h"

// 
// Return pointer to DMI string # n.
// The pointer points to a string in BIOS space 
//
static const char * dmi_string (LPDMI_20_HEADER DmiHeader, UCHAR n )
{
	const char *bp = (const char *)DmiHeader + DmiHeader->Length;

	if (n == 0) {
//		LogString(hMsi, "InDriver.cpp: Error in BIOS DMI data; String index is 0.");
		return NULL;
	}
	else
	{
		while (--n) {
			// advance bp over string
			bp += strlen(bp) + 1;
			// Reached end of data ?
			if (*bp == 0) {
//				LogString(hMsi, "InDriver.cpp: Error in BIOS DMI data; String index too high.");
				return NULL;
			}
		}
	}

	return bp;
}

struct DMIid_T {
	char *BoardId;
	char *BoardRev;
};

static BOOL GetBoardId(struct DMIid_T *pId, char *pDmi, unsigned uSize, unsigned NumberOfStructures)
{
	LPDMI_20_HEADER DmiHeader = (LPDMI_20_HEADER) pDmi;
	unsigned nStructCount = 0;
	char *EndAddress_DMIStructures = pDmi + uSize;

	while ( nStructCount <= NumberOfStructures)
	{
		// Check strings can be accessed and compute address of next structure
		char *pcNext = (char*) DmiHeader + DmiHeader->Length;
		if (pcNext > EndAddress_DMIStructures - 2)
		{
//			LogString(hMsi, "InDriver.cpp: Error1 in DMI data");
			return FALSE;
		}
		while (*pcNext || pcNext[1]) {
			if (pcNext < EndAddress_DMIStructures - 2)
				pcNext++;
			else {
//				LogString(hMsi, "InDriver.cpp: Error2 in DMI data");
				return FALSE;
			}
		}
		pcNext += 2;

		if (DmiHeader->Type == DMI_TYPE_BASEBOARD_INFO)
		{
			DMI_TYPE2_HEADER *h = (DMI_TYPE2_HEADER*)DmiHeader;
			const char *s = dmi_string( DmiHeader, h->Product);
			if (s)
			{
				size_t l = strlen(s)+1;
				pId->BoardId = new char[l];
				memcpy(pId->BoardId, s, l);
			}
			s = dmi_string( DmiHeader, h->Version);
			if (s)
			{
				size_t l = strlen(s)+1;
				pId->BoardRev = new char[l];
				memcpy(pId->BoardRev, s, l);
			}
			return TRUE;
		}
		DmiHeader = (LPDMI_20_HEADER) pcNext;
	
		nStructCount++;

	}

	return FALSE;
}

void x(UCHAR *p, int c)
{
	while (c > 0)
	{
		for (int i=0; i<64; i++)
		{
//			printf("%02x(%c) ", *p, *p);
			printf("%c", isalpha(*p) ? (*p) : ' ');
			p++;
			c--;
		}
		printf("\n");
	}
}
int main(int argc, char* argv[])
{
	DWORD dwBytesCount;

	HANDLE hF = CreateFile(EfdmiDriverName,
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

	// IOCTL_CMOS_PRESENT
	ULONG ulPresent = 0;
	dwBytesCount = 0;
	BOOLEAN bRetVal = DeviceIoControl(hF, IOCTL_FSC_HARDWARE_PRESENT,
									NULL, 0,
									&ulPresent, sizeof(ulPresent),
									&dwBytesCount,
									NULL);

	printf("EFDMI driver present: %d(%d)\n", ulPresent, dwBytesCount);

	// IOCTL_CMOSI_GET_DRIVER_VERSION
	ULONG ulVersion = 0;
	dwBytesCount = 0;
	bRetVal = DeviceIoControl(hF, IOCTL_FSC_GET_DRIVER_VERSION,
									NULL, 0,
									&ulVersion, sizeof(ulVersion),
									&dwBytesCount,
									NULL);

	printf("EFDMI driver version: %x(%d)\n", ulVersion, dwBytesCount);
	

	//
	// IOCTL_EFDMI_GET_E_AND_F_SEGMENT
	//
	UCHAR data[0x20000];
	bRetVal = DeviceIoControl(hF, IOCTL_EFDMI_GET_E_AND_F_SEGMENT,
									NULL, 0,
									data, sizeof(data),
									&dwBytesCount,
									NULL);

	printf("IOCTL_EFDMI_GET_E_AND_F_SEGMENT\n");
	printf("bRetVal = %x; dwBytesCount = %d\n", bRetVal, dwBytesCount);


	//
	// IOCTL_EFDMI_GET_E_AND_F_SEGMENT
	//
	UCHAR data1[0x20000];
	LARGE_INTEGER pa;
	pa.LowPart = 0xe0000;
	pa.HighPart = 0;
	bRetVal = DeviceIoControl(hF, IOCTL_EFDMI_GET_PHYSICAL_MEMORY,
									&pa, sizeof(pa),
									data1, sizeof(data1),
									&dwBytesCount,
									NULL);

	printf("IOCTL_EFDMI_GET_PHYSICAL_MEMORY\n");
	printf("bRetVal = %x; dwBytesCount = %d\n", bRetVal, dwBytesCount);
	if (memcmp(data, data1, sizeof(data)))
	{
		printf("Uh, data != data\n");
	}
	//
	// IOCTL_EFDMI_GET_DMI_SIZE
	//
	ULONG DmiSize;
	bRetVal = DeviceIoControl(hF, IOCTL_EFDMI_GET_DMI_SIZE,
									NULL, 0,
									&DmiSize, sizeof(DmiSize),
									&dwBytesCount,
									NULL);

	printf("IOCTL_EFDMI_GET_DMI_SIZE = %d\n", DmiSize);
	printf("bRetVal = %x; dwBytesCount = %d\n", bRetVal, dwBytesCount);


	//
	// IOCTL_EFDMI_GET_DMI_DATA
	//
	bRetVal = DeviceIoControl(hF, IOCTL_EFDMI_GET_DMI_DATA,
									NULL, 0,
									&data, DmiSize,
									&dwBytesCount,
									NULL);

	printf("IOCTL_EFDMI_GET_DMI_DATA\n");
	printf("bRetVal = %x; dwBytesCount = %d\n", bRetVal, dwBytesCount);

	DMI_20_HEADER *p =  (DMI_20_HEADER*)data;

	printf("Type = %d\n", p->Type);
	printf("Length = %d\n", p->Length);
	printf("Handle = %d\n", p->Handle);
	// looks ok .....
//	x(data, dwBytesCount);
	printf("\n");

		// Get system board from DMI
	struct DMIid_T DMIid = { NULL, NULL};

	struct {
		unsigned Size;
		unsigned NumberOfStructures;
	} DmiSizeAndCount = {0,0};
	
	//
	// IOCTL_EFDMI_GET_DMI_SIZE
	//
	DeviceIoControl(hF, IOCTL_EFDMI_GET_DMI_SIZE,
								NULL, 0,
								&DmiSizeAndCount, sizeof(DmiSizeAndCount),
								&dwBytesCount,
								NULL);
	if (DmiSizeAndCount.Size)
	{
		char *b = new char[DmiSizeAndCount.Size];
		DeviceIoControl(hF, IOCTL_EFDMI_GET_DMI_DATA,
								NULL, 0,
								b, DmiSizeAndCount.Size,
								&dwBytesCount,
								NULL);
		if (dwBytesCount == DmiSizeAndCount.Size)
		{
			GetBoardId(&DMIid, b, DmiSizeAndCount.Size, DmiSizeAndCount.NumberOfStructures);
		}
		delete(b);
	}

	printf("BoardId: %s\nBoardRev: %s\n", DMIid.BoardId, DMIid.BoardRev);
	CloseHandle(hF);

	return 0;
}

