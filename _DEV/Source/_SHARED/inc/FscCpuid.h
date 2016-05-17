#ifndef _CPUID_H_
#define _CPUID_H_


#define CpuidServiceName "FscCpuid"
#define CpuidDriverImage "FscCpuid.sys"
#define CpuidDriverName	"\\\\.\\FSC_CPUID"

#ifndef _NTDDK_
#include <winioctl.h>
#endif

// ============
// IOCTL:
// ============

#define FILE_DEVICE_CPUID	0xAA34	// custom number in the range 0x8000 to 0xFFFF


// IOCTL_EXECUTE_REQUEST -----------------------------------------------


// input parameters: 1 or 2 ULONG (DWORD)
//					first parameter is contents of EAX register for CPUID instruction.
//					optional 2nd parameter is the processor number where the instruction should be executed.
// output paramters:  CPUID_T defined below
// input parameter with CPU number
#pragma pack(push,1)
typedef struct {
	ULONG eax;
	ULONG cpu;
} FscCpuidEx_T;
#define IOCTL_CPUID_CPUID	CTL_CODE(FILE_DEVICE_CPUID, 0,	METHOD_BUFFERED,FILE_ANY_ACCESS)

// input parameters: 1 or 2 ULONG (DWORD)
//					first parameter is contents of ECX register for RDMSR instruction (MSR index).
//					optional 2nd parameter is the processor number where the instruction should be executed.
// output paramters:  MSR_T defined below
typedef struct {
	ULONG ecx;
	ULONG cpu;
} FscRDMSREx_T;
#define IOCTL_CPUID_RDMSR	CTL_CODE(FILE_DEVICE_CPUID, 1,	METHOD_BUFFERED,FILE_ANY_ACCESS)

#pragma warning ( disable : 4200 )	// nonstandard extension used

typedef struct {
	ULONG eax;
	char id[12]; // EBX, ECX, EDX
} CPUID_T;

typedef struct {
	ULONG eax;
	ULONG edx;
} MSR_T;
#pragma pack(pop)

#endif