/*++

Module Name:

    queue.c

Abstract:

    This file contains the queue entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "queue.tmh"

#include "WatchdogIoctl.h"
#include "FSCIoctl.h"
#include "SmBusIoctl.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, SysMonCharosQueueInitialize)
#endif

NTSTATUS
SysMonCharosQueueInitialize(
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

    queueConfig.EvtIoDeviceControl = SysMonCharosEvtIoDeviceControl;
    queueConfig.EvtIoStop = SysMonCharosEvtIoStop;

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

static UCHAR WB_CR_READ(PUCHAR Pnp_Port, UCHAR index)
{
	UCHAR CRReg;

	// enter extended function mode 
	WRITE_PORT_UCHAR(Pnp_Port, 0x87); WRITE_PORT_UCHAR(Pnp_Port, 0x87);
	// select logical device B=HardwareMonitoring
	WRITE_PORT_UCHAR(Pnp_Port, 0x07); WRITE_PORT_UCHAR(Pnp_Port + 1, 0x0B);

	WRITE_PORT_UCHAR(Pnp_Port, index);
	CRReg = (UCHAR) READ_PORT_UCHAR(Pnp_Port + 1);

	WRITE_PORT_UCHAR(Pnp_Port, 0xAA);	// exit extended function mode

	return CRReg;
}

static UCHAR WB_READ(PDEVICE_CONTEXT pdx, UCHAR index)
{
	WRITE_PORT_UCHAR(pdx->indexport, index);
	return READ_PORT_UCHAR(pdx->dataport);
}

static VOID WB_WRITE(PDEVICE_CONTEXT pdx, UCHAR index, UCHAR data)
{
	WRITE_PORT_UCHAR(pdx->indexport, index);
	WRITE_PORT_UCHAR(pdx->dataport, data);
}

VOID
SysMonCharosEvtIoDeviceControl(
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
    TraceEvents(TRACE_LEVEL_INFORMATION, 
                TRACE_QUEUE, 
                "%!FUNC! Queue 0x%p, Request 0x%p OutputBufferLength %d InputBufferLength %d IoControlCode %d", 
                Queue, Request, (int) OutputBufferLength, (int) InputBufferLength, IoControlCode);

	//ULONG cbin = InputBufferLength;
	//ULONG cbout = OutputBufferLength;
	WDFDEVICE  device;
	PDEVICE_CONTEXT  pDevContext;
	USHORT requiredSize = 0;
	PVOID  pOutBuffer;
	PVOID  pInputBuffer;

	//WATCHDOG_INFO myWDInfo;
	PUCHAR PnP_Port;
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	device = WdfIoQueueGetDevice(Queue);
	//
	// GetDeviceContext is a driver-defined function 
	// to retrieve device object context space.
	//
	pDevContext = DeviceGetContext(device);
		
	PnP_Port = (PUCHAR)pDevContext->Pnp_Port;
	
	switch (IoControlCode)
	{					// process request
	  case IOCTL_FSC_HARDWARE_PRESENT:
	  {
		    ULONG ulPresent = 1;
			requiredSize = sizeof(ULONG);
			status = WdfRequestRetrieveOutputBuffer(Request, (size_t)requiredSize, &pOutBuffer, NULL);
			if (!NT_SUCCESS(status)) 
			{
				requiredSize = 0;
				break;
			}

			RtlCopyMemory(pOutBuffer, (void *) &ulPresent, sizeof(ULONG));
			status = STATUS_SUCCESS;
	  }
	  break;

	  case IOCTL_FSC_GET_DRIVER_VERSION:
	  {
		  requiredSize = sizeof(ULONG);
		  status = WdfRequestRetrieveOutputBuffer(Request, (size_t)requiredSize, &pOutBuffer, NULL);
		  if (!NT_SUCCESS(status))
		  {
			  requiredSize = 0;
			  break;
		  }

		  ULONG DriverVersion = DRIVER_VERSION;
		  RtlCopyMemory(pOutBuffer, (void *) &DriverVersion, sizeof(ULONG));
		  status = STATUS_SUCCESS;
	  }
	  break;

	  case IOCTL_GET_HARDWARE_ID:
	  {
		  requiredSize = sizeof(ULONG);
		  ULONG DevID = 0x00;

		  status = WdfRequestRetrieveOutputBuffer(Request, (size_t)requiredSize, &pOutBuffer, NULL);
		  if (!NT_SUCCESS(status))
		  {
			  requiredSize = 0;
			  break;
		  }

		  DevID = pDevContext->CRDeviceID;
		  RtlCopyMemory(pOutBuffer, (void *) &DevID, sizeof(ULONG));
		  status = STATUS_SUCCESS;
	  }
	  break;

	  case IOCTL_GET_CONFIG_REG:
	  {
		  requiredSize = sizeof(GET_CONFIG_REG_T);
		  status = WdfRequestRetrieveInputBuffer(Request, (size_t)requiredSize, &pInputBuffer, NULL);

		  if (NT_SUCCESS(status))
		  {
			  GET_CONFIG_REG_T *pCFGRegInfo = (GET_CONFIG_REG_T *)pInputBuffer;

			  pCFGRegInfo->ConfigRegData = WB_CR_READ(PnP_Port, (UCHAR)pCFGRegInfo->ConfigRegAdr);

				requiredSize = sizeof(GET_CONFIG_REG_T);
				status = WdfRequestRetrieveOutputBuffer(Request, (size_t)requiredSize, &pOutBuffer, NULL);
				if (!NT_SUCCESS(status))
				{
					requiredSize = 0;
					break;
				}

				RtlCopyMemory(pOutBuffer, (void *) pCFGRegInfo, sizeof(GET_CONFIG_REG_T));
				status = STATUS_SUCCESS;
		  }
		  else
			requiredSize = 0;
	  }
	  break;

	  case IOCTL_SMBus_ByteDataRead:
	  {
		  requiredSize = sizeof(SMB_INFO);
		  status = WdfRequestRetrieveInputBuffer(Request, (size_t)requiredSize, &pInputBuffer, NULL);
		  
		  if (NT_SUCCESS(status))
		  {
			  SMB_INFO *pSMBInfo = (SMB_INFO *)pInputBuffer;

			  if (pSMBInfo->SlaveAddress << 1 == 0x5c)
			  {
				  pSMBInfo->DataByteLow = WB_READ(pDevContext, (UCHAR) pSMBInfo->CommandCode);
				  
				  requiredSize = sizeof(SMB_INFO);
				  status = WdfRequestRetrieveOutputBuffer(Request, (size_t)requiredSize, &pOutBuffer, NULL);
				  if (!NT_SUCCESS(status))
				  {
					  requiredSize = 0;
					  break;
				  }

				  RtlCopyMemory(pOutBuffer, (void *) pSMBInfo, sizeof(SMB_INFO));
				  status = STATUS_SUCCESS;
			  }
		  }
		  else
			requiredSize = 0;
	  }
	  break;

	  case IOCTL_SMBus_ByteDataWrite:
	  {
		  requiredSize = sizeof(SMB_INFO);
		  status = WdfRequestRetrieveInputBuffer(Request, (size_t)requiredSize, &pInputBuffer, NULL);

		  if (NT_SUCCESS(status))
		  {
			  SMB_INFO *pSMBInfo = (SMB_INFO *)pInputBuffer;

			  if (pSMBInfo->SlaveAddress << 1 == 0x5c)
			  {
				  WB_WRITE(pDevContext, (UCHAR)pSMBInfo->CommandCode, (UCHAR)pSMBInfo->DataByteLow);

				  requiredSize = sizeof(SMB_INFO);
				  status = WdfRequestRetrieveOutputBuffer(Request, (size_t)requiredSize, &pOutBuffer, NULL);
				  if (!NT_SUCCESS(status))
				  {
					  requiredSize = 0;
					  break;
				  }

				  RtlCopyMemory(pOutBuffer, (void *) pSMBInfo, sizeof(SMB_INFO));
				  status = STATUS_SUCCESS;
			  }
		  }
		  else
			requiredSize = 0;
	  }
	  break;

	  case IOCTL_WATCHDOG_READCONFIG_REG:
	  {
		  requiredSize = sizeof(WATCHDOG_INFO);
		  status = WdfRequestRetrieveInputBuffer(Request, (size_t)requiredSize, &pInputBuffer, NULL);
		  WATCHDOG_INFO *myWDInfo = (WATCHDOG_INFO *) pInputBuffer;

		  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE,
			  "%!FUNC! IOCTL_WATCHDOG_READCONFIG_REG(WDINFO.ConfigRegisterAdr=0x%02X,WDINFO.DataByte=0x%02X), status=0x%08lX",
			   myWDInfo->ConfigRegisterAdr, myWDInfo->Data,status);

		  if (NT_SUCCESS(status))
		  {
			  // enter extended function mode 
			  WRITE_PORT_UCHAR(PnP_Port, 0x87);
			  WRITE_PORT_UCHAR(PnP_Port, 0x87);

			  // select logical device 8=WatchdogDevice
			  WRITE_PORT_UCHAR(PnP_Port, 0x07); WRITE_PORT_UCHAR(PnP_Port + 1, 0x08);

			  // select ConfigRegister
			  WRITE_PORT_UCHAR(PnP_Port, myWDInfo->ConfigRegisterAdr);

			  // read data from config register
			  myWDInfo->Data = READ_PORT_UCHAR(PnP_Port + 1);

			  // exit extended function mode
			  WRITE_PORT_UCHAR(PnP_Port, 0xAA);

			  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE,
				  "%!FUNC! IOCTL_WATCHDOG_READCONFIG_REG(WDINFO.ConfigRegisterAdr=0x%02X,WDINFO.DataByte=0x%02X",
				  myWDInfo->ConfigRegisterAdr, myWDInfo->Data);

			  // copy retrieved data to SystemBuffer
			  requiredSize = sizeof(WATCHDOG_INFO);
			  status = WdfRequestRetrieveOutputBuffer(Request, (size_t)requiredSize, &pOutBuffer, NULL);
			  if (!NT_SUCCESS(status))
			  {
				  requiredSize = 0;
				  break;
			  }

			  RtlCopyMemory(pOutBuffer, (void*) myWDInfo, sizeof(WATCHDOG_INFO));
			  status = STATUS_SUCCESS;
		  }
		  else
			requiredSize = 0;
	  }
	  break;

	  case IOCTL_WATCHDOG_WRITECONFIG_REG:
	  
	  
	  {
		  requiredSize = sizeof(WATCHDOG_INFO);
		  status = WdfRequestRetrieveInputBuffer(Request, (size_t)requiredSize, &pInputBuffer, NULL);
		  WATCHDOG_INFO *myWDInfo = (WATCHDOG_INFO *)pInputBuffer;

		  TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE,
			  "%!FUNC! IOCTL_WATCHDOG_WRITECONFIG_REG(WDINFO.ConfigRegisterAdr=0x%02X,WDINFO.DataByte=0x%02X),status=0x%08lX",
			  myWDInfo->ConfigRegisterAdr, myWDInfo->Data, status);

		  if (NT_SUCCESS(status))
		  {
			  // enter extended function mode 
			  WRITE_PORT_UCHAR(PnP_Port, 0x87);
			  WRITE_PORT_UCHAR(PnP_Port, 0x87);

			  // select logical device 8=WatchdogDevice
			  WRITE_PORT_UCHAR(PnP_Port, 0x07); WRITE_PORT_UCHAR(PnP_Port + 1, 0x08);

			  // select ConfigRegister
			  WRITE_PORT_UCHAR(PnP_Port, myWDInfo->ConfigRegisterAdr);

			  // write data to config register
			  WRITE_PORT_UCHAR(PnP_Port + 1, myWDInfo->Data);

			  // read data from config register
			  myWDInfo->Data = READ_PORT_UCHAR(PnP_Port + 1);

			  // exit extended function mode
			  WRITE_PORT_UCHAR(PnP_Port, 0xAA);

			  // copy retrieved data to SystemBuffer
			  requiredSize = sizeof(WATCHDOG_INFO);
			  status = WdfRequestRetrieveOutputBuffer(Request, (size_t)requiredSize, &pOutBuffer, NULL);
			  if (!NT_SUCCESS(status))
			  {
				  requiredSize = 0;
				  break;
			  }

			  RtlCopyMemory(pOutBuffer, (void *) myWDInfo, sizeof(WATCHDOG_INFO));
			  status = STATUS_SUCCESS;
		  }
		  else
			requiredSize = 0;
	  }
	  break;

	  default:
		  status = STATUS_INVALID_DEVICE_REQUEST;
		  break;
	}

	//
	// Complete the request.
	//
	WdfRequestCompleteWithInformation( Request,	status, requiredSize);

	return;

}

VOID
SysMonCharosEvtIoStop(
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
                "%!FUNC! Queue 0x%p, Request 0x%p ActionFlags %d", 
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

