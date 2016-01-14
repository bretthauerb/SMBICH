/*++

Module Name:

	queue.h

Abstract:

	This file contains the queue definitions.

Environment:

	Kernel-mode Driver Framework

--*/

EXTERN_C_START

//
// This is the context that can be placed per queue
// and would contain per queue information.
//
typedef struct _QUEUE_CONTEXT {

	ULONG PrivateDeviceData;  // just a placeholder

} QUEUE_CONTEXT, *PQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, QueueGetContext)

NTSTATUS
GabiAcpiQueueInitialize(
	_In_ WDFDEVICE hDevice
	);

//
// Events from the IoQueue object
//
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL GabiAcpiEvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL GabiAcpiEvtIoInternalDeviceControl;
EVT_WDF_IO_QUEUE_IO_STOP GabiAcpiEvtIoStop;

NTSTATUS GabiAcpiCallAcpi(WDFREQUEST Request, WDFDEVICE parent, UCHAR ucExternal);

NTSTATUS SendDownStreamIrp(IN WDFIOTARGET IoTarget, IN ULONG Ioctl, IN PVOID InputBuffer, IN ULONG InputSize, IN PVOID OutputBuffer, IN ULONG OutputSize);
NTSTATUS EvaluateAcpiMethode(IN WDFIOTARGET IoTarget, IN ULONG Revision, IN ULONG FunctionIndex, IN PHYSICAL_ADDRESS ControlBuffer, IN PHYSICAL_ADDRESS RequestBuffer, IN PHYSICAL_ADDRESS ResponseBuffer);

static const PHYSICAL_ADDRESS ZeroAddr = {/*LowPart*/0UL, /*HighPart*/0 };
static const PHYSICAL_ADDRESS Phys4GB = {/*LowPart*/~0UL, /*HighPart*/0 };

typedef struct
{
	PVOID pVirtual;
	PHYSICAL_ADDRESS Physical;
	ULONGLONG ullSize;
} 
DriverBufferDescriptor, *PDriverBufferDescriptor;

NTSTATUS AllocateDriverBufferDescriptor(PVOID* pBuffer1, ULONG ulLenBuffer1, PVOID* pBuffer2, ULONG ulLenBuffer2, PDriverBufferDescriptor* ppBufferDesc);
NTSTATUS FreeMemoryBlocks(PDriverBufferDescriptor pBufferDesc);
NTSTATUS CopyMemoryBlocks(PVOID* pBufferSrc, PUCHAR pBufferDest, ULONG ulBufferLen);
NTSTATUS ReplaceAndAllocateMemoryBlocks(PVOID* pBuffer, ULONG ulBufferLen, PDriverBufferDescriptor pBufferDesc);


EXTERN_C_END
