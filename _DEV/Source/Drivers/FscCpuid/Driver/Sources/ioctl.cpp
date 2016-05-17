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

#include "FscCpuid.h"
#include "FSCIoctl.h"
#include "stddcls.h"
#include "driver.h"
#include "asm_prototypes.h"

#ifndef min
#define min(x, y) (((x) > (y)) ? (y) : (x))
#endif
#ifndef max
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif

#pragma LOCKEDCODE

static BOOLEAN SetTargetProcessorDpc(ULONG Processor, PDEVICE_EXTENSION pdx)
{
	KAFFINITY mask = KeQueryActiveProcessors();
	unsigned NumberOfProcessors = 0;
	for (KAFFINITY i=1; i; i<<=1)
		if (mask & i) NumberOfProcessors++;
	if (!NumberOfProcessors) NumberOfProcessors=1;
//	DbgPrint("KeQueryActiveProcessors = %x, Processor = %d, NumberOfProcessors = %d\n", mask, Processor, NumberOfProcessors);
	if (Processor < NumberOfProcessors)
	{
		KeSetTargetProcessorDpc(&pdx->dpc, (CCHAR)Processor);
		return TRUE;
	}
	return FALSE;
}

VOID StartIo ( PDEVICE_OBJECT fdo, PIRP pIrp )
{
	PDEVICE_EXTENSION	pdx	  = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	PIO_STACK_LOCATION	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	NTSTATUS			ntStatus = STATUS_INVALID_DEVICE_REQUEST;
	ULONG				info = 0;
	ULONG				DpcProcessor = 0;

	ULONG ulIoctlInputLength  = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG ulIoctlOutputLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG *pulUlongSystemBuffer = (ULONG*)pIrp->AssociatedIrp.SystemBuffer;

	switch ( pIrpStack->Parameters.DeviceIoControl.IoControlCode )
	{
	case IOCTL_FSC_HARDWARE_PRESENT:
		if (!ulIoctlInputLength && ulIoctlOutputLength == sizeof(*pulUlongSystemBuffer))
		{
			*pulUlongSystemBuffer = 1;
			info = sizeof(*pulUlongSystemBuffer);
			ntStatus = STATUS_SUCCESS;
		}
		break;

	case IOCTL_FSC_GET_DRIVER_VERSION:
		if (!ulIoctlInputLength && ulIoctlOutputLength == sizeof(*pulUlongSystemBuffer))
		{
			*pulUlongSystemBuffer = DRIVER_VERSION;
			info = sizeof(*pulUlongSystemBuffer);
			ntStatus = STATUS_SUCCESS;
		}
		break;

	case IOCTL_CPUID_RDMSR:
		//MSR_T msr;
		if (ulIoctlOutputLength != sizeof MSR_T) break;
		switch (ulIoctlInputLength) {
			case 2*sizeof(*pulUlongSystemBuffer):
				DpcProcessor = pulUlongSystemBuffer[1];
				// FALL THRU
			case sizeof(*pulUlongSystemBuffer):
					if (SetTargetProcessorDpc(DpcProcessor, pdx))
					{
						ntStatus = STATUS_PENDING;
					}
				break;
		default:
			break;
		}
		break;

	case IOCTL_CPUID_CPUID:
		//CPUID_T id;
		if (ulIoctlOutputLength != sizeof CPUID_T) break;
		switch (ulIoctlInputLength) {
			case 2*sizeof(*pulUlongSystemBuffer):
				DpcProcessor = pulUlongSystemBuffer[1];
				// FALL THRU
			case sizeof(*pulUlongSystemBuffer):
					if (SetTargetProcessorDpc(DpcProcessor, pdx))
					{
						ntStatus = STATUS_PENDING;
					}
				break;
		default:
			break;
		}
		break;

	default:
		ntStatus = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	if (ntStatus != STATUS_PENDING)
	{
		pIrp->IoStatus.Status = ntStatus;
		pIrp->IoStatus.Information = info;
		IoReleaseRemoveLock(&pdx->RemoveLock, pIrp);
		StartNextPacket(&pdx->dqReadWrite, fdo);
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	}
	else
	{
		KeInsertQueueDpc( &pdx->dpc, pIrp, pdx );
	}
}

VOID DpcFor_CPUID_And_RDMSR(PKDPC /*Dpc*/, PDEVICE_OBJECT fdo, PIRP pIrp, PVOID pContext)
{
	ULONG info;
	NTSTATUS status;
    PDEVICE_EXTENSION pdx = static_cast<PDEVICE_EXTENSION>(pContext);
    PIRP Irp = GetCurrentIrp(&pdx->dqReadWrite);
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	PULONG pulUlongSystemBuffer = (PULONG)Irp->AssociatedIrp.SystemBuffer;

	info = 0;
	status = STATUS_INVALID_DEVICE_REQUEST;
	ULONG fct = *pulUlongSystemBuffer;

	// we can only come here with valid parameters, no need to check anything
	switch (stack->Parameters.DeviceIoControl.IoControlCode)
	{						// process request
	case IOCTL_CPUID_RDMSR:
		MSR_T msr;
		__try
		{
			GetMSR(fct, &msr.eax, &msr.edx);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			NTSTATUS tmp = GetExceptionCode();
			if (tmp != STATUS_SUCCESS)
				status = tmp;
			break;
		}
		RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, &msr, sizeof MSR_T);
		info = sizeof MSR_T;
		status = STATUS_SUCCESS;
		break;

	case IOCTL_CPUID_CPUID:
		CPUID_T id;
		__try
		{
			if (!GetCpuid(fct, &id.eax, id.id))
			{
				RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, &id, sizeof CPUID_T);
				info = sizeof CPUID_T;
				status = STATUS_SUCCESS;
			}
			else
				// CPU does not support CPUID instruction
				status = STATUS_INVALID_DEVICE_REQUEST;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			NTSTATUS tmp = GetExceptionCode();
			if (tmp != STATUS_SUCCESS)
				status = tmp;
			break;
		}

		break;
	}
	
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	StartNextPacket(&pdx->dqReadWrite, fdo);
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

