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
		WdfIoQueueDispatchSequential
		);

	queueConfig.EvtIoDeviceControl = GabiAcpiEvtIoDeviceControl;
	queueConfig.EvtIoInternalDeviceControl = GabiAcpiEvtIoInternalDeviceControl;
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
	PDEVICE_CONTEXT devExt = DeviceGetContext(parent);

	TraceEvents(TRACE_LEVEL_INFORMATION,
		TRACE_QUEUE,
		"!FUNC! Internal Queue 0x%p, Request 0x%p OutputBufferLength %d InputBufferLength %d IoControlCode %d",
		Queue, Request, (int)OutputBufferLength, (int)InputBufferLength, IoControlCode);

	if (devExt->byUseFirmwareMem == FIRMWARE_MEM_CHECK)
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "check acpi node caps\n");
		status = CheckACPI_NodeCaps(parent, NULL, NULL);
	}

	if (NT_SUCCESS(status))
	{
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

					if (devExt->byUseFirmwareMem == FIRMWARE_MEM_KERNEL)
					{
						status = GabiAcpiCallAcpi(Request, parent, 0);
					}
					else if (devExt->byUseFirmwareMem == FIRNWARE_MEM_FIRMWARE)
					{
						status = GabiAcpiCallAcpiFirmwareMem(Request, parent, 0);
					}
				}
				break;

			default:
				status = STATUS_NOT_SUPPORTED;
				break;
		}
	}
	else
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "status != STATUS_SUCCESS\n");
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
	WDFDEVICE parent = WdfIoQueueGetDevice(Queue);
	PDEVICE_CONTEXT devExt = DeviceGetContext(parent);

	TraceEvents(TRACE_LEVEL_INFORMATION,
		TRACE_QUEUE,
		"!FUNC! Queue 0x%p, Request 0x%p OutputBufferLength %d InputBufferLength %d IoControlCode %d",
		Queue, Request, (int)OutputBufferLength, (int)InputBufferLength, IoControlCode);

	if (devExt->byUseFirmwareMem == FIRMWARE_MEM_CHECK)
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "check acpi node caps\n");
		status = CheckACPI_NodeCaps(parent, NULL, NULL);
	}

	if (NT_SUCCESS(status))
	{
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

					if (devExt->byUseFirmwareMem == FIRMWARE_MEM_KERNEL)
					{
						status = GabiAcpiCallAcpi(Request, parent, 1);
					}
					else  if (devExt->byUseFirmwareMem == FIRNWARE_MEM_FIRMWARE)
					{
						status = GabiAcpiCallAcpiFirmwareMem(Request, parent, 1);
					}
				}
				break;

			default:
				status = STATUS_NOT_SUPPORTED;
				break;
		}
	}
	else
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "status != STATUS_SUCCESS\n");
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

PVOID AllocateContiguousMemory(SIZE_T NumberOfBytes, PHYSICAL_ADDRESS HighestAcceptableAddress)
{
	typedef PVOID(*PFN_MmAllocateContiguousNodeMemory)(_In_ SIZE_T NumberOfBytes, _In_ PHYSICAL_ADDRESS LowestAcceptableAddress, _In_ PHYSICAL_ADDRESS HighestAcceptableAddress, _In_opt_ PHYSICAL_ADDRESS BoundaryAddressMultiple, _In_ ULONG Protect, _In_ NODE_REQUIREMENT PreferredNode);

	static PFN_MmAllocateContiguousNodeMemory pfnMmAllocateContiguousNodeMemory = NULL;
	static BOOLEAN bAlreadyGot = FALSE;

	if (!bAlreadyGot)
	{
		UNICODE_STRING uniFuncName;
		RtlInitUnicodeString(&uniFuncName, L"MmAllocateContiguousNodeMemory");

#pragma warning(push)
#pragma warning(disable : 4055)
		pfnMmAllocateContiguousNodeMemory = (PFN_MmAllocateContiguousNodeMemory)MmGetSystemRoutineAddress(&uniFuncName);
#pragma warning(pop)
		bAlreadyGot = TRUE;
	}

	if (pfnMmAllocateContiguousNodeMemory != NULL)
	{
		return pfnMmAllocateContiguousNodeMemory(NumberOfBytes, ZeroAddr, HighestAcceptableAddress, ZeroAddr, PAGE_READWRITE, MM_ANY_NODE_OK);
	}
	else
	{
		return MmAllocateContiguousMemory(NumberOfBytes, HighestAcceptableAddress);
	}
}

NTSTATUS PrepareBuffers(PGabiAcpiCmd pCmd, PHYSICAL_ADDRESS firmwareMemoryBaseAddress, PHYSICAL_ADDRESS* pControlBuffer, PHYSICAL_ADDRESS* pRequestBuffer, PHYSICAL_ADDRESS* pResponseBuffer, UCHAR ucExternal)
{
	NTSTATUS status = STATUS_SUCCESS;
	ULONGLONG ullTotalLength = 0;

	if (pCmd->ControlBufferLen == 0 && ucExternal == 0)
	{
		//ucExternal == 0 buffer provided by other kernel driver
		//check structur to get size 
		PUCHAR pSource;
		UINT16 len;

		pSource = (PUCHAR)MmMapIoSpace(pCmd->ControlBuffer, sizeof(len), MmNonCached);

		memcpy(&len, pSource, sizeof(len));

		MmUnmapIoSpace(pSource, sizeof(len));

		pCmd->ControlBufferLen = len;
	}

	if (pCmd->RequestBufferLen == 0 && ucExternal == 0)
	{
		//ucExternal == 0 buffer provided by other kernel driver
		//check structur to get size
		PUCHAR pSource;
		UINT16 len;

		pSource = (PUCHAR)MmMapIoSpace(pCmd->RequestBuffer, sizeof(len), MmNonCached);

		memcpy(&len, pSource, sizeof(len));

		MmUnmapIoSpace(pSource, sizeof(len));

		pCmd->RequestBufferLen = len;
	}

	if (pCmd->ResponseBufferLen == 0 && ucExternal == 0)
	{
		//ucExternal == 0 buffer provided by other kernel driver
		//check structur to get size
		PUCHAR pSource;
		UINT16 len;

		pSource = (PUCHAR)MmMapIoSpace(pCmd->ResponseBuffer, sizeof(len), MmNonCached);

		memcpy(&len, pSource, sizeof(len));

		MmUnmapIoSpace(pSource, sizeof(len));

		pCmd->ResponseBufferLen = len;
	}

	if (ucExternal == 0)
	{
		//ucExternal == 0 buffer provided by other kernel driver
		//check for sub pointer and get pointer size
		UINT16 uiServiceCategory;
		UINT16 uiServiceCode;
		PUCHAR pSource;

		pSource = (PUCHAR)MmMapIoSpace(pCmd->ControlBuffer, 4 + sizeof(uiServiceCode), MmNonCached);

		memcpy(&uiServiceCategory, pSource + 2, sizeof(uiServiceCategory));
		memcpy(&uiServiceCode, pSource + 4, sizeof(uiServiceCode));

		MmUnmapIoSpace(pSource, 4 + sizeof(uiServiceCode));

		pCmd->AddressLength = 0;

		switch (uiServiceCategory)
		{
			case 3: // Japan FLASH
				if (uiServiceCode == 3 || uiServiceCode == 4)
				{
					pCmd->AddressLength = 8;
				}
				break;

			case 0x8002: // Japan FLASH
				if (uiServiceCode == 3)
				{
					pCmd->AddressLength = 8;
				}
				break;

			case 0x8000:
			case 0x5:	// Japan system data
				if (uiServiceCode == 4 || uiServiceCode == 5)
				{
					pCmd->AddressLength = 4;
				}
				break;

			case 0x7: // UEFI NVRAM data
				pCmd->AddressLength = 8;
				break;

			case 0x8:
				if (uiServiceCode != 1 && uiServiceCode != 2) // UEFI NVRAM data except Enter and Exit will be OTHE
				{
					pCmd->AddressLength = 8;
				}
				break;

			default:
				if ((uiServiceCategory & 0xfe00) == 0x8200)
				{
					pCmd->AddressLength = 8;
				}
				break;
		}

		TraceEvents(TRACE_LEVEL_INFORMATION,
			TRACE_QUEUE,
			"!FUNC! ACPI Service category: 0x%X, code: 0x%X, len = 0x%X",
			uiServiceCategory, uiServiceCode, pCmd->AddressLength);
	}

	pControlBuffer->QuadPart = firmwareMemoryBaseAddress.QuadPart;
	ullTotalLength += pCmd->ControlBufferLen;
	pResponseBuffer->QuadPart = firmwareMemoryBaseAddress.QuadPart + ullTotalLength;
	ullTotalLength += pCmd->ResponseBufferLen;
	pRequestBuffer->QuadPart = firmwareMemoryBaseAddress.QuadPart + ullTotalLength;
	ullTotalLength += pCmd->RequestBufferLen;

	return status;
}

NTSTATUS ReplaceMemoryBlocks(PHYSICAL_ADDRESS firmwareMemoryBaseAddress, UINT32 firmwareMemorySize, UINT32* puiOffset, PUCHAR pBufferSrc, ULONG ulBufferLen, ULONG ulPointerSize, ULONG ulOffsetSrc, UCHAR ucExternal)
{
	NTSTATUS status = STATUS_SUCCESS;

	if (pBufferSrc != NULL)
	{
		try
		{
			for (ULONG i = ulOffsetSrc; i < (ulBufferLen - ulPointerSize); i += sizeof(PHYSICAL_ADDRESS))
			{
				PULONGLONG pAddr = (PULONGLONG)(pBufferSrc + i);

				if (*pAddr != 0)
				{
					PUCHAR pDest, pSrc = NULL;
					PHYSICAL_ADDRESS nextFreeMem, sourceMem;
					SIZE_T len = (SIZE_T)LENGHT_BUFFER(pBufferSrc + i + sizeof(PHYSICAL_ADDRESS), ulPointerSize);
					sourceMem.QuadPart = 0;

					i += ulPointerSize; 

					if (ucExternal != 0)
					{
#if defined(_AMD64_) || defined(_IA64_)
						ProbeForRead((void*)*pAddr,
#else
						ProbeForRead((void*)(*((UINT32*)pAddr)),
#endif
							len,
							sizeof(UCHAR));
					}

					if (*puiOffset + len > firmwareMemorySize)
					{
						status = STATUS_DATA_OVERRUN;
						TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "buffer to small\n");
						break;
					}					

					nextFreeMem.QuadPart = firmwareMemoryBaseAddress.QuadPart + *puiOffset;
					*puiOffset = *puiOffset + (UINT32)len;

#if defined(_AMD64_) || defined(_IA64_)
					pDest = (PUCHAR)nextFreeMem.QuadPart;
#else
					pDest = (PUCHAR)nextFreeMem.LowPart;
#endif					
					TraceEvents(TRACE_LEVEL_INFORMATION,
						TRACE_QUEUE,
						"!FUNC! ACPI Next copy buffer: 0x%llX, offset: %d, 0x%llX",
						nextFreeMem.QuadPart, i, *pAddr);					

					if (ucExternal == 0)
					{
#if defined(_AMD64_) || defined(_IA64_)
						sourceMem.QuadPart = *pAddr;
#else
						sourceMem.LowPart = *(UINT32*)pAddr;
#endif
					}
					else
					{
						RtlCopyMemory(&pSrc, pAddr, sizeof(PHYSICAL_ADDRESS));
					}

					RtlCopyMemory(pAddr, &pDest, sizeof(PHYSICAL_ADDRESS));

					if (len > 0)
					{
						pDest = (UCHAR*)MmMapIoSpace(nextFreeMem, len, MmNonCached);

						if (ucExternal == 0)
						{
							pSrc = (UCHAR*)MmMapIoSpace(sourceMem, len, MmNonCached);
						}

						RtlCopyMemory(pDest, pSrc, len);

						if (ucExternal == 0)
						{
							MmUnmapIoSpace(pSrc, len);
						}
						MmUnmapIoSpace(pDest, len);
					}
				}
				else
				{
					break;
				}
			}
		}
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			status = STATUS_IN_PAGE_ERROR;
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "exception read user mode buffer1\n");
		}
	}

	return status;
}

NTSTATUS GabiAcpiCallAcpiFirmwareMem(WDFREQUEST Request, WDFDEVICE parent, UCHAR ucExternal)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDFMEMORY inputMemory;
	GabiAcpiCmd cmd;
	PGabiAcpiCmd pCmd = &cmd;
	PHYSICAL_ADDRESS controlBuffer;
	PHYSICAL_ADDRESS requestBuffer;
	PHYSICAL_ADDRESS responseBuffer;
	PHYSICAL_ADDRESS firmwareMemoryBaseAddress;
	UINT32 firmwareMemorySize;
	PUCHAR pData;
	PUCHAR pSource;
	ULONG ulOffset = 0x10;

	status = CheckACPI_NodeCaps(parent, &firmwareMemoryBaseAddress, &firmwareMemorySize);

	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "EvaluateAcpiMethode FunctionIndex=2 failed: 0x%x\n", status);
		return status;
	}

	status = WdfRequestRetrieveInputMemory(Request, &inputMemory);

	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfRequestRetrieveOutputMemory failed: 0x%x\n", status);
		return status;
	}

	memcpy(pCmd, (PGabiAcpiCmd)WdfMemoryGetBuffer(inputMemory, NULL), sizeof(GabiAcpiCmd));

	status = PrepareBuffers(pCmd, firmwareMemoryBaseAddress, &controlBuffer, &requestBuffer, &responseBuffer, ucExternal);

	try
	{
		if (ucExternal != 0)
		{
#if defined(_AMD64_) || defined(_IA64_)
			pSource = (PUCHAR)pCmd->ControlBuffer.QuadPart;
#else
			pSource = (PUCHAR)pCmd->ControlBuffer.LowPart;
#endif

			//Usermode buffers
			ProbeForRead(pSource,
				pCmd->ControlBufferLen,
				sizeof(UCHAR));

			pData = (UCHAR*)MmMapIoSpace(controlBuffer, pCmd->ControlBufferLen, MmNonCached);
		}
		else
		{
			pSource = (UCHAR*)MmMapIoSpace(pCmd->ControlBuffer, pCmd->ControlBufferLen, MmNonCached);
			pData = (UCHAR*)MmMapIoSpace(controlBuffer, pCmd->ControlBufferLen, MmNonCached);
		}

		RtlCopyMemory(pData, pSource, pCmd->ControlBufferLen);

		if (ucExternal != 0)
		{
			MmUnmapIoSpace(pData, pCmd->ControlBufferLen);

#if defined(_AMD64_) || defined(_IA64_)
			pSource = (PUCHAR)pCmd->RequestBuffer.QuadPart;
#else
			pSource = (PUCHAR)pCmd->RequestBuffer.LowPart;
#endif

			//Usermode buffers
			ProbeForRead(pSource,
				pCmd->RequestBufferLen,
				sizeof(UCHAR));

			pData = (UCHAR*)MmMapIoSpace(requestBuffer, pCmd->RequestBufferLen, MmNonCached);
		}
		else
		{
			MmUnmapIoSpace(pSource, pCmd->ControlBufferLen);
			MmUnmapIoSpace(pData, pCmd->ControlBufferLen);

			pSource = (UCHAR*)MmMapIoSpace(pCmd->RequestBuffer, pCmd->RequestBufferLen, MmNonCached);
			pData = (UCHAR*)MmMapIoSpace(requestBuffer, pCmd->RequestBufferLen, MmNonCached);
		}

		RtlCopyMemory(pData, pSource, pCmd->RequestBufferLen);

		if (ucExternal != 0)
		{
			MmUnmapIoSpace(pData, pCmd->ControlBufferLen);

#if defined(_AMD64_) || defined(_IA64_)
			pSource = (PUCHAR)pCmd->ResponseBuffer.QuadPart;
#else
			pSource = (PUCHAR)pCmd->ResponseBuffer.LowPart;
#endif

			//Usermode buffers
			ProbeForRead(pSource,
				pCmd->ResponseBufferLen,
				sizeof(UCHAR));

			pData = (UCHAR*)MmMapIoSpace(responseBuffer, pCmd->ResponseBufferLen, MmNonCached);
		}
		else
		{
			MmUnmapIoSpace(pSource, pCmd->RequestBufferLen);
			MmUnmapIoSpace(pData, pCmd->RequestBufferLen);

			pSource = (UCHAR*)MmMapIoSpace(pCmd->ResponseBuffer, pCmd->ResponseBufferLen, MmNonCached);
			pData = (UCHAR*)MmMapIoSpace(responseBuffer, pCmd->ResponseBufferLen, MmNonCached);
		}

		RtlCopyMemory(pData, pSource, pCmd->ResponseBufferLen);

		if (ucExternal == 0)
		{
			MmUnmapIoSpace(pSource, pCmd->ResponseBufferLen);
			MmUnmapIoSpace(pData, pCmd->ResponseBufferLen);
		}
		else
		{
			MmUnmapIoSpace(pData, pCmd->ResponseBufferLen);
		}

		if (pCmd->AddressLength > 0)
		{
			UINT32 uiTotalLength = pCmd->ControlBufferLen;
			uiTotalLength += pCmd->ResponseBufferLen;
			uiTotalLength += pCmd->RequestBufferLen;

			if (ucExternal != 0)
			{
#if defined(_AMD64_) || defined(_IA64_)
				pData = (PUCHAR)requestBuffer.QuadPart;
#else
				pData = (PUCHAR)requestBuffer.LowPart;
#endif
			}
			else
			{
				pData = (UCHAR*)MmMapIoSpace(requestBuffer, pCmd->RequestBufferLen, MmNonCached);
			}

			if (NT_SUCCESS(status))
			{
				if (pCmd->AddressLength == 4)
				{
					ulOffset = 0x14;  //offset of write structure
				}

				status = ReplaceMemoryBlocks(firmwareMemoryBaseAddress, firmwareMemorySize, &uiTotalLength, pData, pCmd->RequestBufferLen, pCmd->AddressLength, ulOffset, ucExternal);
			}
			else
			{
				TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "ReplaceMemoryBlocks1 failed: 0x%x\n", status);
			}

			if (ucExternal != 0)
			{
#if defined(_AMD64_) || defined(_IA64_)
				pData = (PUCHAR)responseBuffer.QuadPart;
#else
				pData = (PUCHAR)responseBuffer.LowPart;
#endif
			}
			else
			{
				MmUnmapIoSpace(pData, pCmd->RequestBufferLen);

				pData = (UCHAR*)MmMapIoSpace(responseBuffer, pCmd->ResponseBufferLen, MmNonCached);
			}

			if (NT_SUCCESS(status))
			{
				if (pCmd->AddressLength == 4)
				{
					ulOffset = 0xC;  //offset of read structure
				}

				status = ReplaceMemoryBlocks(firmwareMemoryBaseAddress, firmwareMemorySize, &uiTotalLength, pData, pCmd->ResponseBufferLen, pCmd->AddressLength, ulOffset, ucExternal);
			}
			else
			{
				TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "ReplaceMemoryBlocks2 failed: 0x%x\n", status);
			}

			if (ucExternal == 0)
			{
				MmUnmapIoSpace(pData, pCmd->ResponseBufferLen);
			}
		}
	}
	except(EXCEPTION_EXECUTE_HANDLER)
	{
		status = STATUS_IN_PAGE_ERROR;
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "exception read user mode buffers\n");
	}

	if (NT_SUCCESS(status))
	{		
		status = EvaluateAcpiMethode(WdfDeviceGetIoTarget(parent), pCmd->Revision, pCmd->FunctionIndex, controlBuffer, requestBuffer, responseBuffer, NULL, 0);
	}

	if (pCmd->AddressLength > 0)
	{
		if (pCmd->AddressLength == 4)
		{
			ulOffset = 0xC;  //offset of read structure
		}

		if (ucExternal != 0)
		{
#if defined(_AMD64_) || defined(_IA64_)
			pData = (PUCHAR)pCmd->ResponseBuffer.QuadPart;
#else
			pData = (PUCHAR)pCmd->ResponseBuffer.LowPart;
#endif
			pSource = (UCHAR*)MmMapIoSpace(responseBuffer, pCmd->ResponseBufferLen, MmNonCached);
		}
		else
		{
			pData = (UCHAR*)MmMapIoSpace(pCmd->ResponseBuffer, pCmd->ResponseBufferLen, MmNonCached);
			pSource = (UCHAR*)MmMapIoSpace(responseBuffer, pCmd->ResponseBufferLen, MmNonCached);
		}

		if (NT_SUCCESS(status))
		{
			try
			{
				if (ucExternal != 0)
				{
					//Usermode buffers
					ProbeForWrite(pData,
						ulOffset,
						sizeof(UCHAR));
				}

				RtlCopyMemory(pData, pSource, ulOffset);

				status = CopyMemoryBlocks(pSource, pData, pCmd->ResponseBufferLen, ucExternal, ulOffset, pCmd->AddressLength, 1);

				if (!NT_SUCCESS(status))
				{
					TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "CopyMemoryBlocks(Res) failed: 0x%x\n", status);
				}
			}
			except(EXCEPTION_EXECUTE_HANDLER)
			{
				status = STATUS_IN_PAGE_ERROR;
				TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "exception write user mode buffers\n");
			}
		}

		if (ucExternal == 0)
		{
			MmUnmapIoSpace(pData, pCmd->ResponseBufferLen);
			MmUnmapIoSpace(pSource, pCmd->ResponseBufferLen);
		}
		else
		{
			MmUnmapIoSpace(pSource, pCmd->ResponseBufferLen);
		}
	}
	else
	{
		try
		{
			if (ucExternal != 0)
			{
#if defined(_AMD64_) || defined(_IA64_)
				pData = (PUCHAR)pCmd->ResponseBuffer.QuadPart;
#else
				pData = (PUCHAR)pCmd->ResponseBuffer.LowPart;
#endif

				//Usermode buffers
				ProbeForWrite(pData,
					pCmd->ResponseBufferLen,
					sizeof(UCHAR));

				pSource = (UCHAR*)MmMapIoSpace(responseBuffer, pCmd->ResponseBufferLen, MmNonCached);
			}
			else
			{
				pData = (UCHAR*)MmMapIoSpace(pCmd->ResponseBuffer, pCmd->ResponseBufferLen, MmNonCached);
				pSource = (UCHAR*)MmMapIoSpace(responseBuffer, pCmd->ResponseBufferLen, MmNonCached);
			}

			RtlCopyMemory(pData, pSource, pCmd->ResponseBufferLen);

			if (ucExternal == 0)
			{
				MmUnmapIoSpace(pData, pCmd->ResponseBufferLen);
				MmUnmapIoSpace(pSource, pCmd->ResponseBufferLen);
			}
			else
			{
				MmUnmapIoSpace(pSource, pCmd->ResponseBufferLen);
			}
		}
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			status = STATUS_IN_PAGE_ERROR;
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "exception write user mode buffers\n");
		}
	}

	try
	{
		if (ucExternal != 0)
		{
#if defined(_AMD64_) || defined(_IA64_)
			pData = (PUCHAR)pCmd->ControlBuffer.QuadPart;
#else
			pData = (PUCHAR)pCmd->ControlBuffer.LowPart;
#endif

			//Usermode buffers
			ProbeForWrite(pData,
				pCmd->ControlBufferLen,
				sizeof(UCHAR));

			pSource = (UCHAR*)MmMapIoSpace(controlBuffer, pCmd->ControlBufferLen, MmNonCached);
		}
		else
		{
			pData = (UCHAR*)MmMapIoSpace(pCmd->ControlBuffer, pCmd->ControlBufferLen, MmNonCached);
			pSource = (UCHAR*)MmMapIoSpace(controlBuffer, pCmd->ControlBufferLen, MmNonCached);
		}

		RtlCopyMemory(pData, pSource, pCmd->ControlBufferLen);

		if (ucExternal == 0)
		{
			MmUnmapIoSpace(pData, pCmd->ControlBufferLen);
			MmUnmapIoSpace(pSource, pCmd->ControlBufferLen);
		}
		else
		{
			MmUnmapIoSpace(pSource, pCmd->ControlBufferLen);
		}
	}
	except(EXCEPTION_EXECUTE_HANDLER)
	{
		status = STATUS_IN_PAGE_ERROR;
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "exception write user mode buffers\n");
	}

	return status;
}

NTSTATUS GabiAcpiCallAcpi(WDFREQUEST Request, WDFDEVICE parent, UCHAR ucExternal)
{
	const ULONG ulOffset = 0x10;
	NTSTATUS status = STATUS_SUCCESS;
	PVOID controlBufferVirtual = NULL;
	PVOID requestBufferVirtual = NULL;
	PVOID responseBufferVirtual = NULL;
	PHYSICAL_ADDRESS controlBuffer;
	PHYSICAL_ADDRESS requestBuffer;
	PHYSICAL_ADDRESS responseBuffer;
	WDFMEMORY inputMemory;
	PGabiAcpiCmd pCmd = NULL;
	PDriverBufferDescriptor pBuffers = NULL;

	status = WdfRequestRetrieveInputMemory(Request, &inputMemory);

	if (!NT_SUCCESS(status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfRequestRetrieveOutputMemory failed: 0x%x\n", status);
		return status;
	}

	pCmd = (PGabiAcpiCmd)WdfMemoryGetBuffer(inputMemory, NULL);

	if (ucExternal != 0)
	{
		//Usermode buffers
		PUCHAR pData;

		controlBufferVirtual = AllocateContiguousMemory(pCmd->ControlBufferLen, Phys4GB);
		requestBufferVirtual = AllocateContiguousMemory(pCmd->RequestBufferLen, Phys4GB);
		responseBufferVirtual = AllocateContiguousMemory(pCmd->ResponseBufferLen, Phys4GB);

		if (controlBufferVirtual == NULL || requestBufferVirtual == NULL || responseBufferVirtual == NULL)
		{
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "AllocateContiguousMemory failed\n");

			if (controlBufferVirtual != NULL)
			{
				MmFreeContiguousMemory(controlBufferVirtual);
			}

			if (requestBufferVirtual != NULL)
			{
				MmFreeContiguousMemory(requestBufferVirtual);
			}

			if (responseBufferVirtual != NULL)
			{
				MmFreeContiguousMemory(responseBufferVirtual);
			}

			return STATUS_NO_MEMORY;
		}

		try
		{
#if defined(_AMD64_) || defined(_IA64_)
			pData = (PUCHAR)pCmd->ControlBuffer.QuadPart;
#else
			pData = (PUCHAR)pCmd->ControlBuffer.LowPart;
#endif

			ProbeForRead(pData,
				pCmd->ControlBufferLen,
				sizeof(UCHAR));

			RtlCopyMemory(controlBufferVirtual, pData, pCmd->ControlBufferLen);

#if defined(_AMD64_) || defined(_IA64_)
			pData = (PUCHAR)pCmd->RequestBuffer.QuadPart;
#else
			pData = (PUCHAR)pCmd->RequestBuffer.LowPart;
#endif

			ProbeForRead(pData,
				pCmd->RequestBufferLen,
				sizeof(UCHAR));

			RtlCopyMemory(requestBufferVirtual, pData, pCmd->RequestBufferLen);

#if defined(_AMD64_) || defined(_IA64_)
			pData = (PUCHAR)pCmd->ResponseBuffer.QuadPart;
#else
			pData = (PUCHAR)pCmd->ResponseBuffer.LowPart;
#endif

			ProbeForRead(pData,
				pCmd->ResponseBufferLen,
				sizeof(UCHAR));

			RtlCopyMemory(responseBufferVirtual, pData, pCmd->ResponseBufferLen);
		}
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			status = STATUS_IN_PAGE_ERROR;
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "exception read user mode buffers\n");
		}

		controlBuffer = MmGetPhysicalAddress(controlBufferVirtual);
		requestBuffer = MmGetPhysicalAddress(requestBufferVirtual);
		responseBuffer = MmGetPhysicalAddress(responseBufferVirtual);

		if (pCmd->AddressLength > 0)
		{
			status = AllocateDriverBufferDescriptor(requestBufferVirtual, pCmd->RequestBufferLen, responseBufferVirtual, pCmd->ResponseBufferLen, &pBuffers, ulOffset, pCmd->AddressLength);

			if (!NT_SUCCESS(status))
			{
				TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "AllocateDriverBufferDescriptor failed: 0x%x\n", status);
			}
			else
			{
				status = ReplaceAndAllocateMemoryBlocks(requestBufferVirtual, pCmd->RequestBufferLen, pBuffers, ulOffset, pCmd->AddressLength);

				if (!NT_SUCCESS(status))
				{
					TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "CopyAndAllocateMemoryBlocks(Req) failed: 0x%x\n", status);
				}
				else
				{
					status = ReplaceAndAllocateMemoryBlocks(responseBufferVirtual, pCmd->ResponseBufferLen, pBuffers, ulOffset, pCmd->AddressLength);

					if (!NT_SUCCESS(status))
					{
						TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "CopyAndAllocateMemoryBlocks(Res) failed: 0x%x\n", status);
					}
				}
			}
		}
	}
	else
	{
		controlBuffer = pCmd->ControlBuffer;
		requestBuffer = pCmd->RequestBuffer;
		responseBuffer = pCmd->ResponseBuffer;
	}

	if (NT_SUCCESS(status))
	{
		status = EvaluateAcpiMethode(WdfDeviceGetIoTarget(parent), pCmd->Revision, pCmd->FunctionIndex, controlBuffer, requestBuffer, responseBuffer, NULL, 0);
	}

	if (ucExternal != 0)
	{
		//Usermode buffers
		PUCHAR pData;

		if (pCmd->AddressLength > 0)
		{
#if defined(_AMD64_) || defined(_IA64_)
			pData = (PUCHAR)pCmd->ResponseBuffer.QuadPart;
#else
			pData = (PUCHAR)pCmd->ResponseBuffer.LowPart;
#endif

			if (NT_SUCCESS(status))
			{
				status = CopyMemoryBlocks(responseBufferVirtual, pData, pCmd->ResponseBufferLen, ucExternal, ulOffset, pCmd->AddressLength, 0);

				if (!NT_SUCCESS(status))
				{
					TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "CopyMemoryBlocks(Res) failed: 0x%x\n", status);
				}
			}

			status = FreeMemoryBlocks(pBuffers);

			if (!NT_SUCCESS(status))
			{
				TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "FreeMemoryBlocks failed: 0x%x\n", status);
			}
		}
		else
		{
			try
			{
#if defined(_AMD64_) || defined(_IA64_)
				pData = (PUCHAR)pCmd->ResponseBuffer.QuadPart;
#else
				pData = (PUCHAR)pCmd->ResponseBuffer.LowPart;
#endif

				ProbeForWrite(pData,
					pCmd->ResponseBufferLen,
					sizeof(UCHAR));

				RtlCopyMemory(pData, responseBufferVirtual, pCmd->ResponseBufferLen);
			}
			except(EXCEPTION_EXECUTE_HANDLER)
			{
				status = STATUS_IN_PAGE_ERROR;
				TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "exception write user mode buffers\n");
			}
		}

		try
		{
#if defined(_AMD64_) || defined(_IA64_)
			pData = (PUCHAR)pCmd->ControlBuffer.QuadPart;
#else
			pData = (PUCHAR)pCmd->ControlBuffer.LowPart;
#endif

			ProbeForWrite(pData,
				pCmd->ControlBufferLen,
				sizeof(UCHAR));

			RtlCopyMemory(pData, controlBufferVirtual, pCmd->ControlBufferLen);
		}
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			status = STATUS_IN_PAGE_ERROR;
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "exception write user mode buffers\n");
		}

		if (controlBufferVirtual != NULL)
		{
			MmFreeContiguousMemory(controlBufferVirtual);
		}

		if (requestBufferVirtual != NULL)
		{
			MmFreeContiguousMemory(requestBufferVirtual);
		}

		if (responseBufferVirtual != NULL)
		{
			MmFreeContiguousMemory(responseBufferVirtual);
		}
	}

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
CheckACPI_NodeCaps(
	WDFDEVICE parent,
	PHYSICAL_ADDRESS* pFirmwareMemoryBaseAddress,
	PUINT32 pFirmwareMemorySize
)
{	
	PHYSICAL_ADDRESS zero;
	NTSTATUS status;		
	
	zero.QuadPart = 0;
	
	if (pFirmwareMemoryBaseAddress == NULL || pFirmwareMemorySize == NULL)
	{
		PDEVICE_CONTEXT devExt = DeviceGetContext(parent);
		ULONGLONG ullFlags = 0;

		status = EvaluateAcpiMethode(WdfDeviceGetIoTarget(parent), 0, 0, zero, zero, zero, (BYTE*)&ullFlags, sizeof(ullFlags));

		if (NT_SUCCESS(status))
		{			
			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "FunctionIndex=0 returns: 0x%I64X\n", ullFlags);

			if ((ullFlags & 7) == 7)  //Function 0-2 supported?
			{
				devExt->byUseFirmwareMem = FIRNWARE_MEM_FIRMWARE;
			}
			else
			{
				devExt->byUseFirmwareMem = FIRMWARE_MEM_KERNEL;
			}
		}
		else
		{
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "EvaluateAcpiMethode failed: 0x%x\n", status);
			devExt->byUseFirmwareMem = FIRMWARE_MEM_KERNEL;
		}
	}
	else
	{
		const UINT32 bufferSize = 12;
		BYTE bufferResponse[12];

		status = EvaluateAcpiMethode(WdfDeviceGetIoTarget(parent), 0, 2, zero, zero, zero, bufferResponse, bufferSize);

		if (NT_SUCCESS(status))
		{
			memcpy(pFirmwareMemoryBaseAddress, bufferResponse, 8);
			memcpy(pFirmwareMemorySize, &bufferResponse[8], 4);

			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "FunctionIndex=2 returns address: 0x%I64X  size: 0x%x\n", pFirmwareMemoryBaseAddress->QuadPart, *pFirmwareMemorySize);			
		}
		else
		{
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "EvaluateAcpiMethode failed: 0x%x\n", status);			
		}
	}

	return status;
}

NTSTATUS
EvaluateAcpiMethode(
	IN WDFIOTARGET		IoTarget,
	IN ULONG            Revision,
	IN ULONG            FunctionIndex,
	IN PHYSICAL_ADDRESS	ControlBuffer,
	IN PHYSICAL_ADDRESS	RequestBuffer,
	IN PHYSICAL_ADDRESS	ResponseBuffer,
	IN BYTE* pBuffer, 
	IN UINT32 dwBufferLength
	)
{
	const ULONG gabiQueryFunc = 0;
	const ULONG gabiBufferQuery = 2;
	const ULONG argumentCount = 4;
	const size_t acpiInBufferSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) + argumentCount * sizeof(ACPI_METHOD_ARGUMENT) + sizeof(GUID) + 3 * sizeof(PHYSICAL_ADDRESS);
	NTSTATUS status;
	PACPI_EVAL_INPUT_BUFFER_COMPLEX pInputBuffer = NULL;
	PACPI_METHOD_ARGUMENT pargument;
	// {6AB81A65-1149-4c2b-B0EF-A83E13F282F0}
	static const GUID acpiGuid = { 0x6AB81A65, 0x1149, 0x4c2b,{ 0xB0, 0xEF, 0xA8, 0x3E, 0x13, 0xF2, 0x82, 0xF0 } };

	TraceEvents(TRACE_LEVEL_INFORMATION,
		TRACE_QUEUE,
		"!FUNC! EvaluateAcpiMethode argcount: 0x%x function: 0x%x", argumentCount, FunctionIndex);

	if (FunctionIndex == gabiBufferQuery)
	{
		if (pBuffer == NULL || dwBufferLength < 12)
		{
			return STATUS_INVALID_PARAMETER;
		}
	}
	else if (FunctionIndex == gabiQueryFunc)
	{
		if (pBuffer == NULL || dwBufferLength < 8)
		{
			return STATUS_INVALID_PARAMETER;
		}
	}

	pInputBuffer = (PACPI_EVAL_INPUT_BUFFER_COMPLEX)ExAllocatePoolZero(PagedPool, acpiInBufferSize, MEM_TAG);

	if (pInputBuffer == NULL)
	{
		return STATUS_NO_MEMORY;
	}

	// Fill in the input data
	pInputBuffer->MethodNameAsUlong = (ULONG)('MSD_'); //little endian _DSM
	pInputBuffer->ArgumentCount = argumentCount;
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

	if (FunctionIndex != gabiBufferQuery && FunctionIndex != gabiQueryFunc)
	{
		// Send the request along
		status = SendDownStreamIrp(
			IoTarget,
			IOCTL_ACPI_EVAL_METHOD,
			pInputBuffer,
			(ULONG)acpiInBufferSize,
			NULL,
			0
		);
	}
	else
	{
		PACPI_EVAL_OUTPUT_BUFFER pOutputBuffer = NULL;
		size_t outputBufferSize = FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) + 24;

		pOutputBuffer = (PACPI_EVAL_OUTPUT_BUFFER)ExAllocatePoolZero(PagedPool, outputBufferSize, MEM_TAG);

		if (pOutputBuffer != NULL)
		{
			// Send the request along
			status = SendDownStreamIrp(
				IoTarget,
				IOCTL_ACPI_EVAL_METHOD,
				pInputBuffer,
				(ULONG)acpiInBufferSize,
				pOutputBuffer,
				(ULONG)outputBufferSize
			);
		}
		else
		{
			status = STATUS_NO_MEMORY;
		}

		if (NT_SUCCESS(status) && pOutputBuffer != NULL)
		{
			if (FunctionIndex == gabiBufferQuery)
			{
				if (pOutputBuffer->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE || pOutputBuffer->Count < 1 || pOutputBuffer->Argument[0].Type != ACPI_METHOD_ARGUMENT_INTEGER || pOutputBuffer->Argument[0].DataLength < 4)
				{
					if (pOutputBuffer->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE)
					{
						TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "Signature doesn't match Signature: 0x%x\n", pOutputBuffer->Signature);
					}
					else
					{
						if (pOutputBuffer->Count < 1)
						{
							TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "Count doesn't match Count: 0x%x\n", pOutputBuffer->Count);
						}
						else
						{
							TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Count: 0x%x\n", pOutputBuffer->Count);
							TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Length: 0x%x\n", pOutputBuffer->Argument[0].DataLength);

							if (pOutputBuffer->Argument[0].Type != ACPI_METHOD_ARGUMENT_INTEGER)
							{
								TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "Argument doesn't match Type: 0x%x\n", pOutputBuffer->Argument[0].Type);
							}
							else
							{
								if (pOutputBuffer->Argument[0].DataLength < 4)
								{
									for (int i = 0; i < pOutputBuffer->Argument[0].DataLength; i++)
									{
										TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Data[0x%x] = 0x%x\n", i, pOutputBuffer->Argument[0].Data[i]);
									}

									TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "DataLength doesn't match DataLength: 0x%x\n", pOutputBuffer->Argument[0].DataLength);
								}
							}
						}
					}

					status = STATUS_ACPI_INVALID_DATA;
				}
				else
				{
					UCHAR* pBufferSrc = NULL;
					UINT32 returnAddr;
					UINT64 returnAddr64;
					PHYSICAL_ADDRESS pyAddress;

					if (pOutputBuffer->Argument[0].DataLength == 4)
					{
						memcpy(&returnAddr, pOutputBuffer->Argument[0].Data, min(pOutputBuffer->Argument[0].DataLength, sizeof(returnAddr)));

						TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "returnAddr: 0x%x\n", returnAddr);

						pyAddress.QuadPart = returnAddr;
					}
					else
					{
						memcpy(&returnAddr64, pOutputBuffer->Argument[0].Data, min(pOutputBuffer->Argument[0].DataLength, sizeof(returnAddr64)));

						TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "returnAddr64: 0x%llX\n", returnAddr64);

						pyAddress.QuadPart = returnAddr64;
					}

					pBufferSrc = (UCHAR*)MmMapIoSpace(pyAddress, dwBufferLength, MmNonCached);

					memcpy(pBuffer, pBufferSrc, dwBufferLength);

					MmUnmapIoSpace(pBufferSrc, dwBufferLength);
				}
			}
			else if (FunctionIndex == gabiQueryFunc)
			{
				if (pOutputBuffer->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE || pOutputBuffer->Count < 1 || pOutputBuffer->Argument[0].Type != ACPI_METHOD_ARGUMENT_BUFFER || pOutputBuffer->Argument[0].DataLength < 1)
				{
					if (pOutputBuffer->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE)
					{
						TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "Signature doesn't match Signature: 0x%x\n", pOutputBuffer->Signature);
					}
					else
					{
						if (pOutputBuffer->Count < 1)
						{
							TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "Count doesn't match Count: 0x%x\n", pOutputBuffer->Count);
						}
						else
						{
							TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Count: 0x%x\n", pOutputBuffer->Count);
							TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Length: 0x%x\n", pOutputBuffer->Argument[0].DataLength);

							if (pOutputBuffer->Argument[0].Type != ACPI_METHOD_ARGUMENT_BUFFER)
							{
								TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "Argument doesn't match Type: 0x%x\n", pOutputBuffer->Argument[0].Type);
							}
							else
							{
								if (pOutputBuffer->Argument[0].DataLength < 1)
								{
									for (int i = 0; i < pOutputBuffer->Argument[0].DataLength; i++)
									{
										TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Data[0x%x] = 0x%x\n", i, pOutputBuffer->Argument[0].Data[i]);
									}

									TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "DataLength doesn't match DataLength: 0x%x\n", pOutputBuffer->Argument[0].DataLength);
								}
							}
						}
					}

					status = STATUS_ACPI_INVALID_DATA;
				}
				else
				{
					BYTE returnV = 0;
					UINT64 returnV64 = 0;

					if (pOutputBuffer->Argument[0].DataLength == 1)
					{
						memcpy(&returnV, pOutputBuffer->Argument[0].Data, min(pOutputBuffer->Argument[0].DataLength, sizeof(returnV)));

						TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "returnV: 0x%x\n", returnV);

						returnV64 = returnV;
					}

					memcpy(pBuffer, &returnV64, sizeof(returnV64));
				}
			}
		}

		if (pOutputBuffer != NULL)
		{
			ExFreePoolWithTag(pOutputBuffer, MEM_TAG);
		}
	}

	if (pInputBuffer != NULL)
	{
		ExFreePoolWithTag(pInputBuffer, MEM_TAG);
	}

	return status;
}

NTSTATUS AllocateDriverBufferDescriptor(PVOID* pBuffer1, ULONG ulLenBuffer1, PVOID* pBuffer2, ULONG ulLenBuffer2, PDriverBufferDescriptor* ppBufferDesc, ULONG ulOffset, ULONG ulPointerSize)
{
	NTSTATUS status = STATUS_SUCCESS;
	ULONG count = 0;

	if (ppBufferDesc == NULL)
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "ppBufferDesc is null\n");
		return STATUS_INVALID_PARAMETER_1;
	}

	if (pBuffer1 != NULL)
	{
		try
		{
			for (ULONG i = ulOffset; i < (ulLenBuffer1 - ulPointerSize); i += sizeof(PHYSICAL_ADDRESS))
			{
				PULONGLONG pAddr = (PULONGLONG)((char*)pBuffer1 +i);
				

				ProbeForRead(pAddr,
					ulPointerSize,
					sizeof(UCHAR));

				if (*pAddr != 0)
				{
					count++;
					i += ulPointerSize; //skip length
				}
				else
				{
					break;
				}
			}
		}
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			status = STATUS_IN_PAGE_ERROR;
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "exception read user mode buffer1\n");
		}
	}

	if (pBuffer2 != NULL)
	{
		try
		{
			for (ULONG i = ulOffset; i < (ulLenBuffer2 - ulPointerSize); i += sizeof(PHYSICAL_ADDRESS))
			{
				PULONGLONG pAddr = (PULONGLONG)((char*)pBuffer2 +i);
			

				// ProbeForRead(pBuffer2 + i,
				ProbeForRead(  (volatile void*) (pAddr),
					ulPointerSize,
					sizeof(UCHAR));

				if (*pAddr != 0)
				{
					count++;
					i += ulPointerSize; //skip length
				}
				else
				{
					break;
				}
			}
		}
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			status = STATUS_IN_PAGE_ERROR;
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "exception read user mode buffer2\n");
		}
	}

	if (count > 0)
	{
		ULONG ulLen = (count + 1) * sizeof(DriverBufferDescriptor); //one free entry to mark end of list
		*ppBufferDesc = ExAllocatePoolZero(PagedPool, ulLen, MEM_TAG);

		if (*ppBufferDesc != NULL)
		{
			RtlZeroMemory(*ppBufferDesc, ulLen);
		}
		else
		{
			return STATUS_NO_MEMORY;
		}
	}

	return status;
}

NTSTATUS FreeMemoryBlocks(PDriverBufferDescriptor pBufferDesc)
{
	NTSTATUS status = STATUS_SUCCESS;

	if (pBufferDesc != NULL)
	{
		PDriverBufferDescriptor pBuffer = pBufferDesc;

		while (1)
		{
			if (pBuffer == NULL)
			{
				break;
			}

			if (pBuffer->pVirtual == NULL)
			{
				break; //end of list
			}

			MmFreeContiguousMemory(pBuffer->pVirtual);

			pBuffer++;
		}

		ExFreePoolWithTag(pBufferDesc, MEM_TAG);
	}

	return status;
}

NTSTATUS CopyMemoryBlocks(PUCHAR pBufferSrc, PUCHAR pBufferDest, ULONG ulBufferLen, UCHAR ucExternal, ULONG ulOffset, ULONG ulPointerSize, BYTE phyMem)
{
	NTSTATUS status = STATUS_SUCCESS;

	if (pBufferSrc != NULL && pBufferDest != NULL && ulBufferLen > ulPointerSize)
	{
		try
		{
			for (ULONG i = ulOffset; i < (ulBufferLen - ulPointerSize); i += (sizeof(PHYSICAL_ADDRESS) + ulPointerSize))
			{
#if defined(_AMD64_) || defined(_IA64_)
				PULONGLONG pAddrDest = (PULONGLONG)(pBufferDest + i);
				PULONGLONG pAddrSrc = (PULONGLONG)(pBufferSrc + i);
#else
				PULONG pAddrDest = (PULONG)(pBufferDest + i);
				PULONG pAddrSrc = (PULONG)(pBufferSrc + i);
#endif
				if (ucExternal != 0)
				{
					//Usermode buffers
					ProbeForRead(pAddrDest,
						sizeof(PHYSICAL_ADDRESS) + ulPointerSize,
						sizeof(UCHAR));
				}

				TraceEvents(TRACE_LEVEL_INFORMATION,
					TRACE_QUEUE,
					"!FUNC! ACPI copyback buffer: 0x%llX, offset: %d",
					*pAddrDest, i);

				if (*pAddrDest != 0) 
				{
					SIZE_T lenDest = (SIZE_T)LENGHT_BUFFER(pBufferDest + i + sizeof(PHYSICAL_ADDRESS), ulPointerSize);
					SIZE_T lenSrc = (SIZE_T)LENGHT_BUFFER(pBufferSrc + i + sizeof(PHYSICAL_ADDRESS), ulPointerSize);

					if (lenSrc == 0 || lenDest == 0)
					{
						continue;
					}

					if (ucExternal != 0)
					{
						//Usermode buffers
						ProbeForWrite((void*)*pAddrDest,
							lenDest,
							sizeof(UCHAR));
					}

					if (phyMem != 0)
					{
						PHYSICAL_ADDRESS phDest;
						PHYSICAL_ADDRESS phSrc;
						PUCHAR pAddrDestM;
						PUCHAR pAddrSrcM;

						phSrc.QuadPart = *pAddrSrc;
						pAddrSrcM = (PUCHAR)MmMapIoSpace(phSrc, lenSrc, MmNonCached);

						if (ucExternal == 0)
						{
							phDest.QuadPart = *pAddrDest;
							pAddrDestM = (PUCHAR)MmMapIoSpace(phDest, lenDest, MmNonCached);
						}
						else
						{
							pAddrDestM = (PUCHAR)*pAddrDest;
						}
				
						RtlCopyMemory(pAddrDestM, pAddrSrcM, min(lenDest, lenSrc));

						MmUnmapIoSpace(pAddrSrcM, lenSrc);

						if (ucExternal == 0)
						{
							MmUnmapIoSpace(pAddrDestM, lenDest);
						}
					}
					else
					{
						RtlCopyMemory((void*)*pAddrDest, (void*)*pAddrSrc, min(lenDest, lenSrc));
					}
				}
				else
				{
					break;
				}
			}
		}
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			status = STATUS_IN_PAGE_ERROR;
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "exception read user mode buffer2\n");
		}
	}

	return status;
}

NTSTATUS ReplaceAndAllocateMemoryBlocks(PVOID* pBuffer, ULONG ulBufferLen, PDriverBufferDescriptor pBufferDesc, ULONG ulOffset, ULONG ulPointerSize)
{
	NTSTATUS status = STATUS_SUCCESS;

	//search for free entry
	while (1)
	{
		if (pBufferDesc == NULL)
		{
			break;
		}

		if (pBufferDesc->pVirtual == NULL)
		{
			break; //end of list
		}

		pBufferDesc++;
	}

	try
	{
		for (ULONG i = ulOffset; i < (ulBufferLen - ulPointerSize); i += sizeof(PHYSICAL_ADDRESS))
		{
#if defined(_AMD64_) || defined(_IA64_)
			PULONGLONG pAddr = (PULONGLONG) ( (char*) pBuffer + i);
			
#else
			PULONG pAddr = (PULONG)((char*) pBuffer + i);
#endif

			ProbeForRead(pAddr,
				ulPointerSize + sizeof(PHYSICAL_ADDRESS),
				sizeof(UCHAR));

			if (*pAddr != 0)
			{
				SIZE_T len = (SIZE_T)LENGHT_BUFFER((char*)pBuffer + i + sizeof(PHYSICAL_ADDRESS), ulPointerSize);

				ProbeForRead((void*)*pAddr,
					len,
					sizeof(UCHAR));

				if (pBufferDesc !=NULL)
					pBufferDesc->pVirtual = AllocateContiguousMemory(len, Phys4GB);

				if (pBufferDesc==NULL||pBufferDesc->pVirtual == NULL)
				{
					TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "AllocateContiguousMemory failed\n");

					return STATUS_NO_MEMORY;
				}

				pBufferDesc->Physical = MmGetPhysicalAddress(pBufferDesc->pVirtual);
				pBufferDesc->ullSize = len;

				RtlCopyMemory(pAddr, &(pBufferDesc->Physical), sizeof(PHYSICAL_ADDRESS));
				pBufferDesc++;
			}
			else
			{
				break;
			}
		}
	}
	except(EXCEPTION_EXECUTE_HANDLER)
	{
		status = STATUS_IN_PAGE_ERROR;
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "exception read user mode buffer\n");
	}

	return status;
}
