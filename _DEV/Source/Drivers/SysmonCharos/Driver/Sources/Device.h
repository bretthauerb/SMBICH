/*++

Module Name:

    device.h

Abstract:

    This file contains the device definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include "public.h"

EXTERN_C_START

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT
{
	PUCHAR	indexport;
	PUCHAR	dataport;

	UCHAR	CRDeviceID;
	UCHAR	Pnp_Port;	// pnp adress 0x2E or 0x4E?



} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

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
SysMonCharosCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    );

EXTERN_C_END
