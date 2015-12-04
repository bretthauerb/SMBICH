#ifndef _FSC_GABI_UEFI_H_
#define _FSC_GABI_UEFI_H_

#include "FscGabi.h"

#ifndef ULLONG
#define ULLONG unsigned long long
#endif

// UEFI Gabi ServiceCategory
#define UEFIGABISYSMON 0x8208


#ifndef _NTDDK_
#include <winioctl.h>

#pragma pack(push,1)

typedef struct
{
	USHORT	ServiceCategory;
	USHORT	Service;
	UCHAR	reserved[26];			// must be 0
	USHORT	AllocatedBufferSize;	// reserved for driver
	USHORT	StructureVersion;
	ULONG	TransactionHandle;
	ULLONG  DataCount;
} UEFI_GabiEnterRequest_T;

typedef struct
{
	USHORT	ServiceCategory;
	USHORT	Service;
	USHORT	Status;
	USHORT	ErrorCode;
	UCHAR	reserved[22];			// must be 0
	USHORT	AllocatedBufferSize;	// reserved for driver
	USHORT	StructureVersion;
	ULONG	TransactionHandle;
	ULLONG  DataCount;
} UEFI_GabiEnterResponse_T;

typedef struct
{
	USHORT	ServiceCategory;
	USHORT	Service;
	UCHAR	reserved[26];			// must be 0
	USHORT	AllocatedBufferSize;	// reserved for driver
	USHORT	StructureVersion;
	ULONG	TransactionHandle;
	ULLONG  DataCount;
} UEFI_GabiExitRequest_T;

typedef struct
{
	USHORT	ServiceCategory;
	USHORT	Service;
	USHORT	Status;
	USHORT	ErrorCode;
	UCHAR	reserved[22];			// must be 0
	USHORT	AllocatedBufferSize;	// reserved for driver
	USHORT	StructureVersion;
	ULONG	TransactionHandle;
	ULLONG  DataCount;				// = 0
} UEFI_GabiExitResponse_T;

typedef struct
{
	USHORT	ServiceCategory;
	USHORT	Service;
	UCHAR	reserved[26];			// must be 0
	USHORT	AllocatedBufferSize;	// reserved for driver
	USHORT	StructureVersion;
	ULONG	TransactionHandle;
	ULLONG  DataCount;				// = 0
} UEFI_GabiSysmonRequest_T;

typedef struct
{
	USHORT	ServiceCategory;
	USHORT	Service;
	USHORT	Status;
	USHORT	ErrorCode;
	USHORT	ActionCode;
	UCHAR	reserved[20];
	USHORT	AllocatedBufferSize;	// reserved for driver
	USHORT	StructureVersion;
	ULONG	TransactionHandle;
	ULLONG  DataCount;				// 4 = size of following SysmonStatus field
	ULONG	SysmonStatus;
} UEFI_GabiSysmonResponse_T;

typedef struct
{
	USHORT	ServiceCategory;
	USHORT	Service;
	UCHAR	reserved[6];			// must be 0
	ULLONG	SpecialCommand;
	UCHAR	reserved1[4];			// must be 0
	ULLONG	DescriptorTableOffset;
	USHORT	AllocatedBufferSize;	// reserved for driver
	USHORT	StructureVersion;
	ULONG	TransactionHandle;
	ULLONG	DataCount;				// Size of following Data array
	UCHAR	Data[0];
} UEFI_GabiFlashRequest_T;

typedef struct
{
	USHORT	ServiceCategory;
	USHORT	Service;
	USHORT	Status;
	USHORT	ErrorCode;
	USHORT	ActionCode;
	ULLONG	SpecialCommand;
	USHORT	ProgressState;
	USHORT	ProgressCode;
	ULLONG	DescriptorTableOffset;
	USHORT	AllocatedBufferSize;	// reserved for driver
	USHORT	StructureVersion;
	ULONG	TransactionHandle;
	ULLONG	DataCount;				// Size of following Data array
	UCHAR	Data[0];
} UEFI_GabiFlashResponse_T;

#pragma pack(pop)
#endif

#ifdef _NTDDK_

#pragma pack(push,1)
typedef struct
{
	USHORT	ServiceCategory;
	USHORT	Service;
	USHORT	Status;
	USHORT	ErrorCode;
	USHORT	ActionCode;
	ULLONG	SpecialCommand;
	USHORT	ProgressState;
	USHORT	ProgressCode;
	ULLONG	DescriptorTableOffset;
} UEFI_GabiKernelControl_T;

typedef struct
{
	USHORT	AllocatedBufferSize;	// reserved for driver
	USHORT	StructureVersion;
	ULONG	TransactionHandle;
	ULLONG	DataCount;				// Size of following Data array
	UCHAR	Data[0];
} UEFI_GabiIO_T;

typedef struct
{
	UEFI_GabiKernelControl_T Header;
	UEFI_GabiIO_T Request;
} UEFI_GabiKernelRequest_T;

typedef struct
{
	UEFI_GabiKernelControl_T Header;
	UEFI_GabiIO_T Response;
} UEFI_GabiKernelResponse_T;

#pragma pack(pop)

#endif
#endif
