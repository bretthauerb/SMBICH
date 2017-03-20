/*++

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#define INITGUID

#include <ntddk.h>
#include <wdf.h>

#include "device.h"
#include "queue.h"
#include "trace.h"

#define MAJOR_VERSION	1
#define MINOR_VERSION	1
#define RELEASE			2
#define DRIVER_VERSION ((MAJOR_VERSION<<24) + (MINOR_VERSION<<16) + RELEASE)

EXTERN_C_START

//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD SysMonCharosEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP SysMonCharosEvtDriverContextCleanup;

EXTERN_C_END
