/*++

Module Name:

	device.h

Abstract:

	This file contains the device definitions.

Environment:

	Kernel-mode Driver Framework

--*/

#include "AcpiGabi.h"

EXTERN_C_START

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT
{	
	BYTE byUseFirmwareMem;  // values:
							//  0: need to initalize flag
							//  1: alloc kernel mem
							//  2: use firnware mem
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

#define FIRMWARE_MEM_CHECK		0  //  need to initalize flag
#define FIRMWARE_MEM_KERNEL		1  //  alloc kernel mem
#define FIRNWARE_MEM_FIRMWARE	2  //  use firnware mem

//
// This macro will generate an inline function called DeviceGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

//
// Function to initialize the device and its callbacks
//
NTSTATUS
GabiAcpiCreateDevice(
	_Inout_ PWDFDEVICE_INIT DeviceInit
	);

EXTERN_C_END
