#ifdef __cplusplus
	extern "C" {
#endif

#pragma warning ( disable : 4201 )
#pragma warning ( disable : 4514 )

#include <stdio.h>
#include <stdarg.h>			// for variable argument list in DebugPrint function
#include <ntddk.h>

#pragma warning ( default : 4201 )

#ifdef __cplusplus
	}
#endif


#include "FscTime.h"
#include "FSCIoctl.h"
#include "stddcls.h"
#include "driver.h"

#ifndef min
#define min(x, y) (((x) > (y)) ? (y) : (x))
#endif
#ifndef max
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif

#pragma LOCKEDCODE

VOID StartIo ( PDEVICE_OBJECT fdo, PIRP pIrp )
{
	PDEVICE_EXTENSION	pdx	  = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	PIO_STACK_LOCATION	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	NTSTATUS			ntStatus = STATUS_INVALID_DEVICE_REQUEST;
	ULONG				info = 0;

	ULONG ulIoctlInputLength  = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG ulIoctlOutputLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG *pulUlongSystemBuffer = (ULONG*)pIrp->AssociatedIrp.SystemBuffer;

	switch ( pIrpStack->Parameters.DeviceIoControl.IoControlCode )
	{
	case IOCTL_FSC_HARDWARE_PRESENT:
		if (ulIoctlInputLength == 0 && ulIoctlOutputLength == sizeof(*pulUlongSystemBuffer))
		{
			*pulUlongSystemBuffer = 1;
			info = sizeof(*pulUlongSystemBuffer);
			ntStatus = STATUS_SUCCESS;
		}
		break;

	case IOCTL_FSC_GET_DRIVER_VERSION:
		if (ulIoctlInputLength == 0 && ulIoctlOutputLength == sizeof(*pulUlongSystemBuffer))
		{
			*pulUlongSystemBuffer = DRIVER_VERSION;
			info = sizeof(*pulUlongSystemBuffer);
			ntStatus = STATUS_SUCCESS;
		}
		break;
	default:
		break;
	}

	pIrp->IoStatus.Status = ntStatus;
	pIrp->IoStatus.Information = info;
	IoReleaseRemoveLock(&pdx->RemoveLock, pIrp);
	StartNextPacket(&pdx->dqReadWrite, fdo);
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return;
}

