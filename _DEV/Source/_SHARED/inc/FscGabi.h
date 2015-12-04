#ifndef _FSC_GABI_H_
#define _FSC_GABI_H_

#define GabiServiceName "FscGabi"
#define GabiDriverImage "FscGabi.sys"
#define GabiDriverName	"\\\\.\\FSC_GABI"

#ifndef _NTDDK_
#include <winioctl.h>
#endif

// ============
// IOCTL:
// ============

#define FILE_DEVICE_GABI	0xA934	// custom number in the range 0x8000 to 0xFFFF


// IOCTL_GABI_EXECUTE_REQUEST -----------------------------------------------
#define IOCTL_GABI_EXECUTE_REQUEST	CTL_CODE(FILE_DEVICE_GABI, 0xc67, METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_GABI_GET_GABI_VERSION	CTL_CODE(FILE_DEVICE_GABI, 0xc68, METHOD_BUFFERED,FILE_ANY_ACCESS)

#pragma warning ( disable : 4200 )	// nonstandard extension used

#pragma pack(push,1)

typedef struct						// The header is the same size (30 bytes) for all GABI commands
{									// only the layout may be different.
	USHORT	ServiceCategory;
	USHORT	Service;
	USHORT	Status;
	USHORT	ErrorCode;
	USHORT	ActionCode;
	USHORT	SubFunction;			// Settings API
	UCHAR	reserved[18];
} GabiGenericAPIHeader_T;

typedef struct
{
	GabiGenericAPIHeader_T Header;
	USHORT	Length;
} GabiServiceAPIRequest_T;

typedef struct
{
	GabiGenericAPIHeader_T Header;
	USHORT	Length;
	ULONG	ResponseBufferSize;
	USHORT	Vendor;
	UCHAR	MajorVersion;
	UCHAR	MinorVersion;
	UCHAR	reserved1[22];
} GabiServiceAPIResponse_T;

#pragma pack(pop)


// The headers for other GABI services (BIOS Settings) are not defined here.

#endif