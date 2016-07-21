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


#include "FscEfDmi.h"
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

PVOID MapEntryPoint(_In_ PHYSICAL_ADDRESS PhysicalAddress, _In_ SIZE_T NumberOfBytes);

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

	case IOCTL_EFDMI_GET_PHYSICAL_MEMORY:
		if (ulIoctlInputLength == sizeof(PHYSICAL_ADDRESS))
		{
			PHYSICAL_ADDRESS *pa = (PHYSICAL_ADDRESS*)pIrp->AssociatedIrp.SystemBuffer;
			PUCHAR va = (PUCHAR)MapEntryPoint(*pa, ulIoctlOutputLength);
			if (va)
			{
				RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, va, ulIoctlOutputLength);
				MmUnmapIoSpace(va, ulIoctlOutputLength);
				info = ulIoctlOutputLength;
				ntStatus = STATUS_SUCCESS;
			}
		}
		break;

	case IOCTL_EFDMI_GET_PHYSICAL_ADDRESS:
		if ((ulIoctlInputLength == sizeof(PHYSICAL_ADDRESS)) &&
		    (ulIoctlOutputLength == sizeof(PHYSICAL_ADDRESS)))
		{
			void *va = *(void**)pIrp->AssociatedIrp.SystemBuffer;
			// return 0 when not possible
			PHYSICAL_ADDRESS pa = {0,0};
			if (MmIsAddressValid(va)) pa = MmGetPhysicalAddress(va);
			RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, &pa, ulIoctlOutputLength);
			info = ulIoctlOutputLength;
			ntStatus = STATUS_SUCCESS;
		}
		break;

	case IOCTL_EFDMI_GET_E_AND_F_SEGMENT:
		if (ulIoctlInputLength == 0 && ulIoctlOutputLength <= 0x20000)
		{
			RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, pdx->pMappedESegment, ulIoctlOutputLength);
			info = ulIoctlOutputLength;
			ntStatus = STATUS_SUCCESS;
		}
		break;

	case IOCTL_EFDMI_GET_DMI_SIZE:
		if (ulIoctlInputLength == 0)
		{
			PUCHAR pucSystemBuffer = (PUCHAR)pulUlongSystemBuffer;
			switch (ulIoctlOutputLength)
			{
			case 2*sizeof(ULONG)+2*sizeof(UCHAR):
				pucSystemBuffer[8] = pdx->ucDMIVersionMajor;
				pucSystemBuffer[9] = pdx->ucDMIVersionMinor;
				// fall thru */
			case 2*sizeof(ULONG):
				pulUlongSystemBuffer[1] = pdx->ulDMIStructCount;
				// fall thru */
			case sizeof(ULONG):
				pulUlongSystemBuffer[0] = pdx->ulDMISize;
				info = ulIoctlOutputLength;
				ntStatus = STATUS_SUCCESS;
				break;
			default:
				break;
			}
		}
		break;

	case IOCTL_EFDMI_GET_DMI_DATA:
		// no use to get partial dmi data, so we do not allow that
		if (ulIoctlInputLength == 0 && ulIoctlOutputLength == pdx->ulDMISize)
		{
			RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, pdx->pucDMI, ulIoctlOutputLength);
			info = ulIoctlOutputLength;
			ntStatus = STATUS_SUCCESS;
		}
		break;

	default:
		ntStatus = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	pIrp->IoStatus.Status = ntStatus;
	pIrp->IoStatus.Information = info;
	IoReleaseRemoveLock(&pdx->RemoveLock, pIrp);
	StartNextPacket(&pdx->dqReadWrite, fdo);
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return;
}

