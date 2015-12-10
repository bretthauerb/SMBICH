/*++

Module Name:

	queue.c

Abstract:

	This file contains the queue entry points and callbacks.

Environment:

	Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "AcpiGabi.h"
#include "queue.tmh"
#include <Acpiioct.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, GabiAcpiQueueInitialize)
#endif

#define MEM_TAG 'AcMe'

NTSTATUS
GabiAcpiQueueInitialize(
	_In_ WDFDEVICE Device
	)
/*++

Routine Description:


	 The I/O dispatch callbacks for the frameworks device object
	 are configured in this function.

	 A single default I/O Queue is configured for parallel request
	 processing, and a driver context memory allocation is created
	 to hold our structure QUEUE_CONTEXT.

Arguments:

	Device - Handle to a framework device object.

Return Value:

	VOID

--*/
{
	WDFQUEUE queue;
	NTSTATUS status;
	WDF_IO_QUEUE_CONFIG    queueConfig;

	PAGED_CODE();
	
	//
	// Configure a default queue so that requests that are not
	// configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
	// other queues get dispatched here.
	//
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
		 &queueConfig,
		WdfIoQueueDispatchParallel
		);

	queueConfig.EvtIoDeviceControl = GabiAcpiEvtIoDeviceControl;
	queueConfig.EvtIoStop = GabiAcpiEvtIoStop;

	status = WdfIoQueueCreate(
				 Device,
				 &queueConfig,
				 WDF_NO_OBJECT_ATTRIBUTES,
				 &queue
				 );

	if( !NT_SUCCESS(status) ) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfIoQueueCreate failed %!STATUS!", status);
		return status;
	}

	return status;
}

VOID
GabiAcpiEvtIoInternalDeviceControl(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ size_t OutputBufferLength,
	_In_ size_t InputBufferLength,
	_In_ ULONG IoControlCode
	)
/*++

Routine Description:

	This event is invoked when the framework receives IRP_MJ_INTERNAL_DEVICE_CONTROL request.

Arguments:

	Queue -  Handle to the framework queue object that is associated with the
			 I/O request.

	Request - Handle to a framework request object.

	OutputBufferLength - Size of the output buffer in bytes

	InputBufferLength - Size of the input buffer in bytes

	IoControlCode - I/O control code.

Return Value:

	VOID

--*/
{
	NTSTATUS status = STATUS_SUCCESS;
	WDFDEVICE parent = WdfIoQueueGetDevice(Queue);

	TraceEvents(TRACE_LEVEL_INFORMATION,
		TRACE_QUEUE,
		"!FUNC! Internal Queue 0x%p, Request 0x%p OutputBufferLength %d InputBufferLength %d IoControlCode %d",
		Queue, Request, (int)OutputBufferLength, (int)InputBufferLength, IoControlCode);

	switch (IoControlCode)
	{
		case IOCTL_GABI_ACPI_CMD:
			{
				if (InputBufferLength != sizeof(GabiAcpiCmd))
				{
					TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "wrong buffer length\n");
					status = STATUS_INVALID_PARAMETER_1;
					break;
				}

				status = GabiAcpiCallAcpi(Request, parent, 0);
			}
			break;

		default:
			status = STATUS_NOT_SUPPORTED;
			break;
	}

	WdfRequestComplete(Request, status);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "exit: 0x%x\n", status);
	return;
}

VOID
GabiAcpiEvtIoDeviceControl(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ size_t OutputBufferLength,
	_In_ size_t InputBufferLength,
	_In_ ULONG IoControlCode
	)
/*++

Routine Description:

	This event is invoked when the framework receives IRP_MJ_DEVICE_CONTROL request.

Arguments:

	Queue -  Handle to the framework queue object that is associated with the
			 I/O request.

	Request - Handle to a framework request object.

	OutputBufferLength - Size of the output buffer in bytes

	InputBufferLength - Size of the input buffer in bytes

	IoControlCode - I/O control code.

Return Value:

	VOID

--*/
{
	NTSTATUS status = STATUS_SUCCESS;
	//WDFDEVICE parent = WdfIoQueueGetDevice(Queue);

	TraceEvents(TRACE_LEVEL_INFORMATION,
		TRACE_QUEUE,
		"!FUNC! Queue 0x%p, Request 0x%p OutputBufferLength %d InputBufferLength %d IoControlCode %d",
		Queue, Request, (int)OutputBufferLength, (int)InputBufferLength, IoControlCode);

	switch (IoControlCode)
	{
		//case IOCTL_GABI_ACPI_CMD:
		//	{
		//		if (InputBufferLength != sizeof(GabiAcpiCmd))
		//		{
		//			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "wrong buffer length\n");
		//			status = STATUS_INVALID_PARAMETER_1;
		//			break;
		//		}

		//		status = GabiAcpiCallAcpi(Request, parent, 1);
		//	}
		//	break;

		default:
			status = STATUS_NOT_SUPPORTED;
			break;
	}

	WdfRequestComplete(Request, status);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "exit: 0x%x\n", status);
	return;
}

VOID
GabiAcpiEvtIoStop(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ ULONG ActionFlags
)
/*++

Routine Description:

	This event is invoked for a power-managed queue before the device leaves the working state (D0).

Arguments:

	Queue -  Handle to the framework queue object that is associated with the
			 I/O request.

	Request - Handle to a framework request object.

	ActionFlags - A bitwise OR of one or more WDF_REQUEST_STOP_ACTION_FLAGS-typed flags
				  that identify the reason that the callback function is being called
				  and whether the request is cancelable.

Return Value:

	VOID

--*/
{
	TraceEvents(TRACE_LEVEL_INFORMATION, 
				TRACE_QUEUE, 
				"!FUNC! Queue 0x%p, Request 0x%p ActionFlags %d", 
				Queue, Request, ActionFlags);

	//
	// In most cases, the EvtIoStop callback function completes, cancels, or postpones
	// further processing of the I/O request.
	//
	// Typically, the driver uses the following rules:
	//
	// - If the driver owns the I/O request, it calls WdfRequestUnmarkCancelable
	//   (if the request is cancelable) and either calls WdfRequestStopAcknowledge
	//   with a Requeue value of TRUE, or it calls WdfRequestComplete with a
	//   completion status value of STATUS_SUCCESS or STATUS_CANCELLED.
	//
	//   Before it can call these methods safely, the driver must make sure that
	//   its implementation of EvtIoStop has exclusive access to the request.
	//
	//   In order to do that, the driver must synchronize access to the request
	//   to prevent other threads from manipulating the request concurrently.
	//   The synchronization method you choose will depend on your driver's design.
	//
	//   For example, if the request is held in a shared context, the EvtIoStop callback
	//   might acquire an internal driver lock, take the request from the shared context,
	//   and then release the lock. At this point, the EvtIoStop callback owns the request
	//   and can safely complete or requeue the request.
	//
	// - If the driver has forwarded the I/O request to an I/O target, it either calls
	//   WdfRequestCancelSentRequest to attempt to cancel the request, or it postpones
	//   further processing of the request and calls WdfRequestStopAcknowledge with
	//   a Requeue value of FALSE.
	//
	// A driver might choose to take no action in EvtIoStop for requests that are
	// guaranteed to complete in a small amount of time.
	//
	// In this case, the framework waits until the specified request is complete
	// before moving the device (or system) to a lower power state or removing the device.
	// Potentially, this inaction can prevent a system from entering its hibernation state
	// or another low system power state. In extreme cases, it can cause the system
	// to crash with bugcheck code 9F.
	//

	return;
}

//NTSTATUS AllocateDriverBufferDescriptor(PVOID* pBuffer1, ULONG ulLenBuffer1, PVOID* pBuffer2, ULONG ulLenBuffer2, PDriverBufferDescriptor* ppBufferDesc)
//{
//	NTSTATUS status = STATUS_SUCCESS;
//	ULONG count = 0;
//
//	if (ppBufferDesc == NULL)
//	{
//		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "ppBufferDesc is null\n");
//		return STATUS_INVALID_PARAMETER_1;
//	}
//
//	if (pBuffer1 != NULL)
//	{
//		try
//		{
//			for (ULONG i = 0x10; i < (ulLenBuffer1 - 8); i += 8)
//			{
//				if ((ULONGLONG)(pBuffer1 + i) != NULL)
//				{
//					count++;
//					i += 8; //skip length
//				}
//			}
//		}
//		except(EXCEPTION_EXECUTE_HANDLER)
//		{
//			status = STATUS_IN_PAGE_ERROR;
//			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "exception read user mode buffer1\n");
//		}
//	}
//
//	if (pBuffer2 != NULL)
//	{
//		try
//		{
//			for (ULONG i = 0x10; i < (ulLenBuffer2 - 8); i += 8)
//			{
//				if ((ULONGLONG)(pBuffer2 + i) != NULL)
//				{
//					count++;
//					i += 8; //skip length
//				}
//			}
//		}
//		except(EXCEPTION_EXECUTE_HANDLER)
//		{
//			status = STATUS_IN_PAGE_ERROR;
//			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "exception read user mode buffer2\n");
//		}
//	}
//
//	if (count > 0)
//	{
//		ULONG ulLen = (count + 1) * sizeof(DriverBufferDescriptor); //one free entry to mark end of list
//		*ppBufferDesc = ExAllocatePoolWithTag(PagedPool, ulLen, MEM_TAG);
//
//		if (*ppBufferDesc != NULL)
//		{
//			RtlZeroMemory(*ppBufferDesc, ulLen);
//		}
//		else
//		{
//			return STATUS_NO_MEMORY;
//		}
//	}
//
//	return status;
//}
//
//NTSTATUS FreeMemoryBlocks(PDriverBufferDescriptor pBufferDesc)
//{
//	NTSTATUS status = STATUS_SUCCESS;
//
//	if (pBufferDesc == NULL)
//	{
//		return;
//	}
//
//
//
//	return status;
//}
//
//NTSTATUS CopyMemoryBlocks(PVOID* pBufferSrc, PUCHAR pBufferDest, ULONG ulBufferLen)
//{
//	NTSTATUS status = STATUS_SUCCESS;
//
//
//
//	return status;
//}
//
//NTSTATUS ReplaceAndAllocateMemoryBlocks(PVOID* pBuffer, ULONG ulBufferLen, PDriverBufferDescriptor pBufferDesc)
//{
//	NTSTATUS status = STATUS_SUCCESS;
//
//
//
//	return status;
//}

NTSTATUS GabiAcpiCallAcpi(WDFREQUEST Request, WDFDEVICE parent, UCHAR ucExternal)
{
	NTSTATUS status = STATUS_SUCCESS;
	//PVOID controlBufferVirtual = NULL;
	//PVOID requestBufferVirtual = NULL;
	//PVOID responseBufferVirtual = NULL;
	PHYSICAL_ADDRESS controlBuffer;
	PHYSICAL_ADDRESS requestBuffer;
	PHYSICAL_ADDRESS responseBuffer;
	WDFMEMORY inputMemory;
	PGabiAcpiCmd pCmd = NULL;
	//PDriverBufferDescriptor pBuffers = NULL;

	UNREFERENCED_PARAMETER(ucExternal);

	status = WdfRequestRetrieveInputMemory(Request, &inputMemory);

	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfRequestRetrieveOutputMemory failed: 0x%x\n", status);
		return status;
	}

	pCmd = (PGabiAcpiCmd)WdfMemoryGetBuffer(inputMemory, NULL);

	//if (ucExternal != 0)
	//{
	//	//Usermode buffers
	//	PUCHAR pData;

	//	controlBufferVirtual = MmAllocateContiguousMemory(pCmd->ControlBufferLen, Phys4GB);
	//	requestBufferVirtual = MmAllocateContiguousMemory(pCmd->RequestBufferLen, Phys4GB);
	//	responseBufferVirtual = MmAllocateContiguousMemory(pCmd->ResponseBufferLen, Phys4GB);

	//	if (controlBufferVirtual == NULL || requestBufferVirtual == NULL || responseBufferVirtual == NULL)
	//	{
	//		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "MmAllocateContiguousMemory failed\n");

	//		if (controlBufferVirtual != NULL)
	//		{
	//			MmFreeContiguousMemory(controlBufferVirtual);
	//		}

	//		if (requestBufferVirtual != NULL)
	//		{
	//			MmFreeContiguousMemory(requestBufferVirtual);
	//		}

	//		if (responseBufferVirtual != NULL)
	//		{
	//			MmFreeContiguousMemory(responseBufferVirtual);
	//		}

	//		return STATUS_NO_MEMORY;
	//	}

	//	try
	//	{
	//		RtlCopyMemory(&pData, &pCmd->ControlBuffer, sizeof(PUCHAR));

	//		ProbeForRead(controlBufferVirtual,
	//			pCmd->ControlBufferLen,
	//			sizeof(UCHAR));

	//		RtlCopyMemory(controlBufferVirtual, pData, pCmd->ControlBufferLen);

	//		RtlCopyMemory(&pData, &pCmd->RequestBuffer, sizeof(PUCHAR));

	//		ProbeForRead(requestBufferVirtual,
	//			pCmd->RequestBufferLen,
	//			sizeof(UCHAR));

	//		RtlCopyMemory(requestBufferVirtual, pData, pCmd->RequestBufferLen);

	//		RtlCopyMemory(&pData, &pCmd->ResponseBuffer, sizeof(PUCHAR));

	//		ProbeForRead(responseBufferVirtual,
	//			pCmd->ResponseBufferLen,
	//			sizeof(UCHAR));

	//		RtlCopyMemory(responseBufferVirtual, pData, pCmd->ResponseBufferLen);
	//	}
	//	except(EXCEPTION_EXECUTE_HANDLER)
	//	{
	//		status = STATUS_IN_PAGE_ERROR;
	//		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "exception read user mode buffers\n");
	//	}

	//	controlBuffer = MmGetPhysicalAddress(controlBufferVirtual);
	//	requestBuffer = MmGetPhysicalAddress(requestBufferVirtual);
	//	responseBuffer = MmGetPhysicalAddress(responseBufferVirtual);

	//	if (pCmd->AddressLength > 0)
	//	{
	//		status = AllocateDriverBufferDescriptor(requestBufferVirtual, pCmd->RequestBufferLen, responseBufferVirtual, pCmd->ResponseBufferLen, &pBuffers);

	//		if (!NT_SUCCESS(status))
	//		{
	//			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "CopyAndAllocateMemoryBlocks(Req) failed: 0x%x\n", status);
	//		}
	//		else
	//		{
	//			status = ReplaceAndAllocateMemoryBlocks(requestBufferVirtual, pCmd->RequestBufferLen, pBuffers);

	//			if (!NT_SUCCESS(status))
	//			{
	//				TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "CopyAndAllocateMemoryBlocks(Req) failed: 0x%x\n", status);
	//			}
	//			else
	//			{
	//				status = ReplaceAndAllocateMemoryBlocks(responseBufferVirtual, pCmd->ResponseBufferLen, pBuffers);

	//				if (!NT_SUCCESS(status))
	//				{
	//					TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "CopyAndAllocateMemoryBlocks(Res) failed: 0x%x\n", status);
	//				}
	//			}
	//		}
	//	}
	//}
	//else
	{
		controlBuffer = pCmd->ControlBuffer;
		requestBuffer = pCmd->RequestBuffer;
		responseBuffer = pCmd->ResponseBuffer;
	}

	if (NT_SUCCESS(status))
	{
		status = EvaluateAcpiMethode(WdfDeviceGetIoTarget(parent), pCmd->Revision, pCmd->FunctionIndex, controlBuffer, requestBuffer, responseBuffer);
	}

	//if (ucExternal != 0)
	//{
	//	//Usermode buffers
	//	PUCHAR pData;

	//	if (pCmd->AddressLength > 0)
	//	{
	//		RtlCopyMemory(&pData, &pCmd->ResponseBuffer, sizeof(PUCHAR));

	//		if (NT_SUCCESS(status))
	//		{
	//			status = CopyMemoryBlocks(responseBufferVirtual, pData, pCmd->ResponseBufferLen);

	//			if (!NT_SUCCESS(status))
	//			{
	//				TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "CopyMemoryBlocks(Res) failed: 0x%x\n", status);
	//			}
	//		}

	//		status = FreeMemoryBlocks(pBuffers);

	//		if (!NT_SUCCESS(status))
	//		{
	//			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "FreeMemoryBlocks failed: 0x%x\n", status);
	//		}
	//	}
	//	else
	//	{
	//		try
	//		{
	//			RtlCopyMemory(&pData, &pCmd->ResponseBuffer, sizeof(PUCHAR));

	//			ProbeForWrite(responseBufferVirtual,
	//				pCmd->ResponseBufferLen,
	//				sizeof(UCHAR));

	//			RtlCopyMemory(pData, responseBufferVirtual, pCmd->ResponseBufferLen);
	//		}
	//		except(EXCEPTION_EXECUTE_HANDLER)
	//		{
	//			status = STATUS_IN_PAGE_ERROR;
	//			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "exception write user mode buffers\n");
	//		}
	//	}

	//	if (controlBufferVirtual != NULL)
	//	{
	//		MmFreeContiguousMemory(controlBufferVirtual);
	//	}

	//	if (requestBufferVirtual != NULL)
	//	{
	//		MmFreeContiguousMemory(requestBufferVirtual);
	//	}

	//	if (responseBufferVirtual != NULL)
	//	{
	//		MmFreeContiguousMemory(responseBufferVirtual);
	//	}
	//}

	return status;
}

NTSTATUS
SendDownStreamIrp(
	IN WDFIOTARGET		IoTarget,
	IN ULONG            IoctlControlCode,
	IN PVOID            InputBuffer,
	IN ULONG            InputBufferLength,
	IN PVOID            OutputBuffer,
	IN ULONG            OutputBufferLength
	)
	/*
	Routine Description:
	General-purpose function called to send a request to the PDO.
	The IOCTL argument accepts the control method being passed down
	by the calling function

	This subroutine is only valid for the IOCTLS other than ASYNC EVAL.

	Parameters:
	Pdo             - the request is sent to this device object
	Ioctl           - the request - specified by the calling function
	InputBuffer     - incoming request
	InputSize       - size of the incoming request
	OutputBuffer    - the answer
	OutputSize      - size of the answer buffer

	Return Value:
	NT Status of the operation
	*/
{
	NTSTATUS                status;
	WDF_MEMORY_DESCRIPTOR   inputDesc, outputDesc;
	PWDF_MEMORY_DESCRIPTOR  pInputDesc = NULL, pOutputDesc = NULL;
	ULONG_PTR               bytesReturned;

	TraceEvents(TRACE_LEVEL_INFORMATION,
		TRACE_QUEUE,
		"!FUNC! SendDownStreamIrp");

	if (InputBuffer)
	{
		WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDesc,
			InputBuffer,
			InputBufferLength);
		pInputDesc = &inputDesc;
	}

	if (OutputBuffer)
	{
		WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDesc,
			OutputBuffer,
			OutputBufferLength);
		pOutputDesc = &outputDesc;
	}

	status = WdfIoTargetSendIoctlSynchronously(
		IoTarget,
		WDF_NO_HANDLE, // Request
		IoctlControlCode,
		pInputDesc,
		pOutputDesc,
		NULL, // PWDF_REQUEST_SEND_OPTIONS
		&bytesReturned);

	return status;
}

NTSTATUS
EvaluateAcpiMethode(
	IN WDFIOTARGET		IoTarget,
	IN ULONG            Revision,
	IN ULONG            FunctionIndex,
	IN PHYSICAL_ADDRESS	ControlBuffer,
	IN PHYSICAL_ADDRESS	RequestBuffer,
	IN PHYSICAL_ADDRESS	ResponseBuffer
	)
{
	const size_t acpiInBufferSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) + 4 * sizeof(ACPI_METHOD_ARGUMENT) + sizeof(GUID) + 3 * sizeof(PHYSICAL_ADDRESS);
	NTSTATUS status;
	PACPI_EVAL_INPUT_BUFFER_COMPLEX pInputBuffer = NULL;
	PACPI_METHOD_ARGUMENT pargument;
	// {6AB81A65-1149-4c2b-B0EF-A83E13F282F0}
	static const GUID acpiGuid = { 0x6AB81A65, 0x1149, 0x4c2b,{ 0xB0, 0xEF, 0xA8, 0x3E, 0x13, 0xF2, 0x82, 0xF0 } };

	TraceEvents(TRACE_LEVEL_INFORMATION,
		TRACE_QUEUE,
		"!FUNC! EvaluateAcpiMethode");

	pInputBuffer = (PACPI_EVAL_INPUT_BUFFER_COMPLEX)ExAllocatePoolWithTag(PagedPool, acpiInBufferSize, MEM_TAG);

	if (pInputBuffer == NULL)
	{
		return STATUS_NO_MEMORY;
	}

	// Fill in the input data
	pInputBuffer->MethodNameAsUlong = (ULONG)('MSD_'); //little endian _DSM
	pInputBuffer->ArgumentCount = 4;
	pInputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;

	pargument = pInputBuffer->Argument;
	ACPI_METHOD_SET_ARGUMENT_BUFFER(pargument, (PUCHAR)&acpiGuid, sizeof(acpiGuid));

	pargument = ACPI_METHOD_NEXT_ARGUMENT(pargument);
	ACPI_METHOD_SET_ARGUMENT_INTEGER(pargument, Revision);

	pargument = ACPI_METHOD_NEXT_ARGUMENT(pargument);
	ACPI_METHOD_SET_ARGUMENT_INTEGER(pargument, FunctionIndex);

	pargument = ACPI_METHOD_NEXT_ARGUMENT(pargument);
	pargument->Type = ACPI_METHOD_ARGUMENT_PACKAGE;
	pargument->DataLength = 3 * sizeof(PHYSICAL_ADDRESS);
	RtlCopyMemory(pargument->Data, (PUCHAR)&ControlBuffer, sizeof(PHYSICAL_ADDRESS));
	RtlCopyMemory(pargument->Data + sizeof(PHYSICAL_ADDRESS), (PUCHAR)&RequestBuffer, sizeof(PHYSICAL_ADDRESS));
	RtlCopyMemory(pargument->Data + 2 * sizeof(PHYSICAL_ADDRESS), (PUCHAR)&ResponseBuffer, sizeof(PHYSICAL_ADDRESS));

	TraceEvents(TRACE_LEVEL_INFORMATION,
		TRACE_QUEUE,
		"!FUNC! ACPI Buffer: Control 0x%llX, Physical 0x%llX Response 0x%llX",
		ControlBuffer.QuadPart, RequestBuffer.QuadPart, ResponseBuffer.QuadPart);

	// Send the request along
	status = SendDownStreamIrp(
		IoTarget,
		IOCTL_ACPI_EVAL_METHOD,
		pInputBuffer,
		(ULONG)acpiInBufferSize,
		NULL,
		0
		);

	if (pInputBuffer != NULL)
	{
		ExFreePoolWithTag(pInputBuffer, MEM_TAG);
	}

	return status;
}
