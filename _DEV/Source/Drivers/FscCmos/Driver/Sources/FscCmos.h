#ifndef _FSCCMOS_H_
#define _FSCCMOS_H_


#define CmosServiceName "FscCmos"
#define CmosDriverImage "FscCmos.sys"
#define CmosDriverName	"\\\\.\\FSC_CMOS"

#ifndef _NTDDK_
#include <winioctl.h>
#endif

// ============
// IOCTL:
// ============

#define FILE_DEVICE_CPUID	0xAB34	// custom number in the range 0x8000 to 0xFFFF


// IOCTL_EXECUTE_REQUEST -----------------------------------------------
#define IOCTL_CMOS_READ		CTL_CODE(FILE_DEVICE_CPUID, 0, METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_CMOS_WRITE 	CTL_CODE(FILE_DEVICE_CPUID, 1,	METHOD_BUFFERED,FILE_ANY_ACCESS)

#pragma warning ( disable : 4200 )	// nonstandard extension used

#pragma pack(push,1)

typedef struct {
	ULONG ulOffset;
} CMOS_READ_T;
typedef struct {
	ULONG ulOffset;
	UCHAR Data[0];
} CMOS_WRITE_T;

#pragma pack(pop)

#endif