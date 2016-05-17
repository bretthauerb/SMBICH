#ifndef _FSCTIME_H_
#define _FSCTIME_H_

#define FscTimeServiceName "FscTime"
#define FscTimeDriverImage "FscTime.sys"
#define FscTimeDriverName	"\\\\.\\FSC_TIME"

#ifndef _NTDDK_
#include <winioctl.h>
#endif

// ============
// IOCTL:
// ============
#define FILE_DEVICE_FSCTIME	0xA912	// custom number in the range 0x8000 to 0xFFFF

#define NUMBER_OF_COUNTERS	16
#endif
