#include "stddcls.h"
#include "driver.h"
#include "DevQueue.h"


#include "D3003OEM.h"
#include "FSCIoctl.h"

#define BUSY_WAIT_RETRY		100

#define RelativeInterval_1ms	((LONGLONG)(-10*1000))


// This is nearly all locked code
#pragma LOCKEDCODE

static UCHAR SMBUS_READ_UCHAR(IN PDEVICE_EXTENSION pdx, unsigned port)
{
	if (pdx->bIoMapped)
	{
		return READ_PORT_UCHAR(pdx->AcpiMMioAddr);
	}
	else
	{
		return pdx->AcpiMMioAddr[port];
	}
}
static void SMBUS_WRITE_UCHAR(IN PDEVICE_EXTENSION pdx, unsigned port, UCHAR v)
{
	if (pdx->bIoMapped)
	{
		WRITE_PORT_UCHAR(pdx->AcpiMMioAddr, v);
	}
	else
	{
		pdx->AcpiMMioAddr[port] = v;
	}
}


///////////////////////////////////////////////////////////////////////////////
//_______SMBus_ClearStatus_____________________________________________________________________
//_______
//_______	Functionality:	Wait for Busy to become clear, then clear all interrupt status bits.
//_______					Incase Busy does not clear, the kill bit will be set to abort the
//							current operation. Prepare for next command.
//_______________________________________________________________________________________

static BOOLEAN SMBus_ClearStatus( IN PDEVICE_EXTENSION pdx )
{
	unsigned StatusPort = SMBUS_HOST_STATUS_REGISTER;
	unsigned CtrlPort   = SMBUS_HOST_CONTROL_REGISTER;
	
	// wait for busy inactive
	// Only in exceptional cases where someone else is using the SMBUS controller
	// we will have to wait.
	for (int retry = BUSY_WAIT_RETRY; retry--; )
	{
		if ((SMBUS_READ_UCHAR( pdx, StatusPort ) & SMBUS_HST_STA_HOST_BUSY) == 0)
			break;

		// Time to kill :-) ?
		if (retry == 0)
		{
			DbgPrint(SMBUS_DRIVER_NAME" SMBus_ClearStatus : Kill Function, HostCtrl = %02X, HostStatus = %02X",
					SMBUS_READ_UCHAR( pdx, CtrlPort ), SMBUS_READ_UCHAR( pdx, StatusPort ));

			SMBUS_WRITE_UCHAR( pdx, CtrlPort, SMBUS_HST_CNT_KILL );
			KeStallExecutionProcessor(50);
			SMBUS_WRITE_UCHAR( pdx, CtrlPort, 0 );
		}
		KeStallExecutionProcessor(50);
	}

	// Clear all interrupt status bits
	SMBUS_WRITE_UCHAR( pdx, StatusPort, SMBUS_HST_STA_INT_MASK);

	// Controller should be idle now
	return (SMBUS_READ_UCHAR( pdx, StatusPort ) & (SMBUS_HST_STA_INT_MASK | SMBUS_HST_STA_HOST_BUSY)) ? FALSE : TRUE;
}


//
// Timeout timer.
//
static VOID  IoTimer(PDEVICE_OBJECT fdo, VOID *)
{
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	if (pdx->TimeOutCounter) {
		pdx->TimeOutCounter--;
	} else {
		//KIRQL oldirql;
		//
		// TODO: there is a race here.
		//
		KeCancelTimer(&pdx->Timer);

//		KeAcquireSpinLock( &pdx->TimeoutLock, &oldirql);
		KeRemoveQueueDpc( &fdo->Dpc );
//		KeReleaseSpinLock( &pdx->TimeoutLock, oldirql );

		PIRP Irp = GetCurrentIrp(&pdx->dqReadWrite);
		if (Irp) {
			Irp->IoStatus.Status = STATUS_TRANSACTION_TIMED_OUT;
			Irp->IoStatus.Information = 0;
			IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
			StartNextPacket(&pdx->dqReadWrite, fdo);
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
		}
	}
}

static NTSTATUS HostStatus2NtStatus( IN UCHAR ucHostStatus )
{
	if (ucHostStatus & SMBUS_HST_STA_DEV_ERR)
		return STATUS_NO_SUCH_DEVICE;
	if (ucHostStatus & (SMBUS_HST_STA_BUS_ERR | SMBUS_HST_STA_FAILED))
		return STATUS_IO_DEVICE_ERROR;
	if (ucHostStatus & SMBUS_HST_STA_HOST_BUSY)
		return STATUS_TRANSACTION_TIMED_OUT;
	if (ucHostStatus & SMBUS_HST_STA_INTR)
		return STATUS_SUCCESS;

	return STATUS_ADAPTER_HARDWARE_ERROR;
}

//
// The interrupt stuff.
//
VOID DpcForPoll(PKDPC /*Dpc*/, PDEVICE_OBJECT fdo, PVOID, PVOID)
{
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;

	// Call the interrupt handler without interrupt object -> polling mode.
	if (!OnInterrupt(NULL, pdx )) {

		// Polling not yet done: re-schedule this DPC again in 1 msec.
		LARGE_INTEGER	liInterval;	liInterval.QuadPart = RelativeInterval_1ms;
		KeSetTimer( &pdx->Timer, liInterval, &pdx->PollDpc );
	} else {
		// Polling done: run DpcForIsr
		KeInsertQueueDpc( &fdo->Dpc, NULL, pdx );
	}
}

static void e2t(UCHAR ucHostStatus)
{
	if (ucHostStatus & SMBUS_HST_STA_INTR)
		DbgPrint(SMBUS_DRIVER_NAME" ERROR: no error ??\n");
	if (ucHostStatus & SMBUS_HST_STA_DEV_ERR)
		DbgPrint(SMBUS_DRIVER_NAME" ERROR: device error\n");
	if (ucHostStatus & SMBUS_HST_STA_BUS_ERR)
		DbgPrint(SMBUS_DRIVER_NAME" ERROR: bus error\n");
	if (ucHostStatus & SMBUS_HST_STA_FAILED)
		DbgPrint(SMBUS_DRIVER_NAME" ERROR: failed\n");
	if (ucHostStatus & SMBUS_HST_STA_HOST_BUSY)
		DbgPrint(SMBUS_DRIVER_NAME" ERROR: busy\n");
}

VOID DpcForIsr(
    _In_ PKDPC /*Dpc*/,
    _In_ struct _DEVICE_OBJECT *fdo,
    _Inout_ struct _IRP * /*junk*/,
    _In_opt_ PVOID pContext
    )
{		
	ULONG info;
	NTSTATUS status;
    PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pContext;
	PIRP Irp = GetCurrentIrp(&pdx->dqReadWrite);
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	PVOID SystemBuffer = Irp->AssociatedIrp.SystemBuffer;

	UCHAR HostStatus = SMBUS_READ_UCHAR( pdx, SMBUS_HOST_STATUS_REGISTER);
	status = HostStatus2NtStatus(HostStatus);

	info = 0;

	switch (stack->Parameters.DeviceIoControl.IoControlCode)
	{						// process request

		case IOCTL_SMBus_ByteDataRead:
			if (NT_SUCCESS(status)) {
				pdx->SMBusInfo.Status = status;
				pdx->SMBusInfo.DataByteLow = SMBUS_READ_UCHAR( pdx, SMBUS_HOST_DATA0_REGISTER );
				RtlCopyMemory( SystemBuffer, &pdx->SMBusInfo, sizeof(SMB_INFO) );
				info = sizeof(SMB_INFO);
			}
			else
				e2t(HostStatus);
			break;

		case IOCTL_SMBus_ByteDataWrite:
			if (NT_SUCCESS(status)) {
				pdx->SMBusInfo.Status = status;
				RtlCopyMemory( SystemBuffer, &pdx->SMBusInfo, sizeof(SMB_INFO) );
				info = sizeof(SMB_INFO);
			}
			break;

		default:
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;

	}
	// TODO: cannot be PENDING, move StopTimer to start of routine.
	if (status != STATUS_PENDING)
	{
		IoStopTimer( fdo );
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = info;
		IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
		StartNextPacket(&pdx->dqReadWrite, fdo);
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
}							// DpcForIsr

VOID DisableInterrupt( IN PDEVICE_EXTENSION pdx )
{
	UCHAR HostControl = SMBUS_READ_UCHAR( pdx, SMBUS_HOST_CONTROL_REGISTER );
	SMBUS_WRITE_UCHAR( pdx, SMBUS_HOST_CONTROL_REGISTER, HostControl & ~SMBUS_HST_CNT_INTREN );
}

// 
// Return value:
//		InterruptObject != NULL: TRUE if this is our interrupt,
//								 FALSE when it is not.
//		InterruptObject == NULL: TRUE when the DPC should be called.
//								 FALSE when further polling is necessary.
//
BOOLEAN OnInterrupt( IN PKINTERRUPT InterruptObject, IN PDEVICE_EXTENSION pdx )
{
	UCHAR HostStatus;

//KdPrint(("OnInterrupt, AcpiMMioAddr = %x\n", AcpiMMioAddr));

	if (pdx->StalledForPower)
		// Then it cannot be my interrupt.
		return FALSE;

	if (InterruptObject) {
		// Interrupts disabled ?
		if (!(SMBUS_READ_UCHAR( pdx, SMBUS_HOST_CONTROL_REGISTER ) & SMBUS_HST_CNT_INTREN))
			return FALSE;
	}

//KdPrint(("OnInterrupt, interrupts are enabled.\n"));
	// Interrupt active ?
	HostStatus = SMBUS_READ_UCHAR( pdx, SMBUS_HOST_STATUS_REGISTER );
	if ((HostStatus & SMBUS_HST_STA_INT_MASK) == 0)
		return FALSE;

//KdPrint(("OnInterrupt, it is our interrupt; HOST_STATUS = %x, CONTROL = %x\n", HostStatus, READ_PORT_UCHAR( AcpiMMioAddr + SMBUS_HOST_CONTROL_REGISTER)));
	// Yes, it is our interrupt.
	// Command should be finished, real work is done in Dpc
	DisableInterrupt( pdx );
	if (InterruptObject)
		IoRequestDpc(pdx->DeviceObject, NULL, pdx);
	return TRUE;
}							// OnInterrupt

static BOOLEAN CheckAndCopyIn(ULONG cbin, ULONG cbout, PVOID dest, PVOID src, unsigned size)
{
	if (cbin < size || cbout < size) return FALSE;
	RtlCopyMemory( dest, src, size);
	return TRUE;
}

VOID StartIo(PDEVICE_OBJECT fdo, PIRP Irp)
{
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
	ULONG	info = 0;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG *pulSystemBuffer = (ULONG*)Irp->AssociatedIrp.SystemBuffer;
	UCHAR *pucSystemBuffer = (PUCHAR)pulSystemBuffer;
	PVOID SystemBuffer = Irp->AssociatedIrp.SystemBuffer;

	if (stack->MajorFunction != IRP_MJ_DEVICE_CONTROL) {
		// Uh ??
		status = STATUS_UNSUCCESSFUL;
	}
	else 
	{
		status = STATUS_INVALID_DEVICE_REQUEST;

		switch (stack->Parameters.DeviceIoControl.IoControlCode)
		{						// process request
		case IOCTL_FSC_HARDWARE_PRESENT:
			if (cbin == 0 && cbout == sizeof(*pulSystemBuffer))
			{
				*pulSystemBuffer = (pdx->AcpiMMioAddr) ? 1 : 0;
				info = sizeof(*pulSystemBuffer);
				status = STATUS_SUCCESS;
			}
			break;

		case IOCTL_FSC_GET_DRIVER_VERSION:
			if (pdx->AcpiMMioAddr && cbin == 0 && cbout == sizeof(*pulSystemBuffer))
			{
				*pulSystemBuffer = DRIVER_VERSION;
				info = sizeof(*pulSystemBuffer);
				status = STATUS_SUCCESS;
			}
			break;

		case IOCTL_SMBus_ByteDataRead:
			if (pdx->AcpiMMioAddr && CheckAndCopyIn(cbin ,cbout, &pdx->SMBusInfo, SystemBuffer, sizeof(SMB_INFO)))
			{
				SMBus_ClearStatus( pdx );

				// Write Slave Address
				SMBUS_WRITE_UCHAR( pdx, SMBUS_HOST_ADDRESS_REGISTER, (UCHAR) ((pdx->SMBusInfo.SlaveAddress<<1) | 1) );
				// Write Command
				SMBUS_WRITE_UCHAR( pdx, SMBUS_HOST_COMMAND_REGISTER, (UCHAR) pdx->SMBusInfo.CommandCode );
				// Write Control (Command Protocol)
				SMBUS_WRITE_UCHAR( pdx, SMBUS_HOST_CONTROL_REGISTER, SMBUS_HST_CNT_CMD_BYTE_DATA | pdx->StartCommand );

				status = STATUS_PENDING;
			}
			break;

		case IOCTL_SMBus_ByteDataWrite:
			if (pdx->AcpiMMioAddr && CheckAndCopyIn(cbin ,cbout, &pdx->SMBusInfo, SystemBuffer, sizeof(SMB_INFO)))
			{
				SMBus_ClearStatus( pdx );

				// Write Slave Address
				SMBUS_WRITE_UCHAR( pdx, SMBUS_HOST_ADDRESS_REGISTER, (UCHAR) ((pdx->SMBusInfo.SlaveAddress<<1) & ~1) );
				// Write Command
				SMBUS_WRITE_UCHAR( pdx, SMBUS_HOST_COMMAND_REGISTER, (UCHAR) pdx->SMBusInfo.CommandCode );
				// Write Data Byte
				SMBUS_WRITE_UCHAR( pdx, SMBUS_HOST_DATA0_REGISTER, (UCHAR) pdx->SMBusInfo.DataByteLow );
				// Write Control (Command Protocol)
				SMBUS_WRITE_UCHAR( pdx, SMBUS_HOST_CONTROL_REGISTER, SMBUS_HST_CNT_CMD_BYTE_DATA | pdx->StartCommand );

				status = STATUS_PENDING;
			}
			break;

		case IOCTL_WRITE_WD_CONTROL:
			if (pdx->watchdogcontrol && cbin == 1 && cbout == 0)
			{
				*(pdx->watchdogcontrol) = *pucSystemBuffer;
				info = 0;
				status = STATUS_SUCCESS;
			}
			break;

		case IOCTL_READ_WD_CONTROL:
			if (pdx->watchdogcontrol && cbin == 0 && cbout == 1)
			{
				*pucSystemBuffer = *(pdx->watchdogcontrol);
				info = 1;
				status = STATUS_SUCCESS;
			}
			break;

		case IOCTL_WRITE_WD_COUNT:
			if (pdx->watchdogcontrol && cbin == 4 && cbout == 0)
			{
				*(pdx->watchdogcount) = *pulSystemBuffer;
				pdx->LastCountWritten = *pulSystemBuffer;
				info = 0;
				status = STATUS_SUCCESS;
			}
			break;

		case IOCTL_READ_WD_COUNT:
			if (pdx->watchdogcontrol && cbin == 0 && cbout == 4)
			{
				*pulSystemBuffer = *(pdx->watchdogcount);
				info = 4;
				status = STATUS_SUCCESS;
			}
			break;

		case IOCTL_TRIGGER_WD:
			if (pdx->watchdogcontrol && cbin == 0 && cbout == 0)
			{
				*(pdx->watchdogcount) = pdx->LastCountWritten;
				*(pdx->watchdogcontrol) |= 0x80;
				info = 0;
				status = STATUS_SUCCESS;
			}
			break;

		default:
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;

		}						// process request
	}
	if (status != STATUS_PENDING)
	{
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = info;
		IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
		StartNextPacket(&pdx->dqReadWrite, fdo);
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	} else {
		if (!pdx->InterruptObject)
		{
			// Run Isr in polling mode every 1 msec;
			LARGE_INTEGER	liInterval;	liInterval.QuadPart = RelativeInterval_1ms;
			KeSetTimer( &pdx->Timer, liInterval, &pdx->PollDpc );
	    }
		pdx->TimeOutCounter = 4;
		IoStartTimer( fdo );
	}
}
#pragma INITCODE

static BOOLEAN Hudson_Initialize( IN PDEVICE_EXTENSION pdx )
{
	union {
		UCHAR b[4];
		ULONG base;
	} u;

	//
	// SMBUS base address
	//
	WRITE_PORT_UCHAR((PUCHAR)0xCD6, (UCHAR)0x24);
	u.b[0] = READ_PORT_UCHAR((PUCHAR)0xCD7);
	WRITE_PORT_UCHAR((PUCHAR)0xCD6, (UCHAR)0x25);
	u.b[1] = READ_PORT_UCHAR((PUCHAR)0xCD7);
	WRITE_PORT_UCHAR((PUCHAR)0xCD6, (UCHAR)0x26);
	u.b[2] = READ_PORT_UCHAR((PUCHAR)0xCD7);
	WRITE_PORT_UCHAR((PUCHAR)0xCD6, (UCHAR)0x27);
	u.b[3] = READ_PORT_UCHAR((PUCHAR)0xCD7);

	PHYSICAL_ADDRESS base = {0,0};
	base.LowPart = u.base & ~0xfff;

	if (!(u.base & 1)) return FALSE;	// decode not enabled
	if (u.base & 2)						// io or memory mapped
	{
		pdx->bIoMapped = TRUE;
		pdx->AcpiMMioAddr = (PUCHAR)NULL + base.LowPart;
	}
	else
	{
		pdx->AcpiMMioAddr = (PUCHAR) MmMapIoSpace(base, 8, MmNonCached);
	}
	// chaotic stuff, select SMBus0
	//WRITE_PORT_UCHAR((PUCHAR)0xCD6, (UCHAR)0x2f);
	WRITE_PORT_UCHAR((PUCHAR)0xCD6, (UCHAR)0x2e);
	WRITE_PORT_UCHAR((PUCHAR)0xCD7, (READ_PORT_UCHAR((PUCHAR)0xCD7) & ~6));

	WRITE_PORT_UCHAR((PUCHAR)0xCD6, (UCHAR)0x2c);
	WRITE_PORT_UCHAR((PUCHAR)0xCD7, (READ_PORT_UCHAR((PUCHAR)0xCD7) & ~6));

	//
	// Watchdog base address
	//
	WRITE_PORT_UCHAR((PUCHAR)0xCD6, (UCHAR)0x48);
	u.b[0] = READ_PORT_UCHAR((PUCHAR)0xCD7);
	WRITE_PORT_UCHAR((PUCHAR)0xCD6, (UCHAR)0x49);
	u.b[1] = READ_PORT_UCHAR((PUCHAR)0xCD7);
	WRITE_PORT_UCHAR((PUCHAR)0xCD6, (UCHAR)0x4a);
	u.b[2] = READ_PORT_UCHAR((PUCHAR)0xCD7);
	WRITE_PORT_UCHAR((PUCHAR)0xCD6, (UCHAR)0x4b);
	u.b[3] = READ_PORT_UCHAR((PUCHAR)0xCD7);

	base.LowPart = u.base & ~0x7;
	if (u.base & 1)	// decode enabled ?
	{
		pdx->watchdogcontrol = (PUCHAR) MmMapIoSpace(base, 8, MmNonCached);

		if (pdx->watchdogcontrol)
		{
			pdx->watchdogcount = (PULONG)(pdx->watchdogcontrol) + 1;

			pdx->LastCountWritten = 0xffff;
			*pdx->watchdogcontrol = *(pdx->watchdogcontrol) & ~1;	// stop watchdog

			// select 1Hz clock
			WRITE_PORT_UCHAR((PUCHAR)0xCD6, (UCHAR)0x4c);
			WRITE_PORT_UCHAR((PUCHAR)0xCD7, 3);

			// enable wd
			WRITE_PORT_UCHAR((PUCHAR)0xCD6, (UCHAR)0x48);
			WRITE_PORT_UCHAR((PUCHAR)0xCD7, u.b[0] & ~2);	// enable watchdog function

		}
	}
	IoInitializeTimer(pdx->DeviceObject, IoTimer, NULL);

	return TRUE;
}
#pragma PAGEDCODE

NTSTATUS StartDevice(PDEVICE_OBJECT fdo, PCM_PARTIAL_RESOURCE_LIST /*raw*/, PCM_PARTIAL_RESOURCE_LIST translated)
	{							// StartDevice
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	//NTSTATUS status;

	// Identify the I/O resources we're supposed to use.
	
	ULONG vector;
	KIRQL irql;
	KINTERRUPT_MODE mode;
	KAFFINITY affinity;
	BOOLEAN irqshare;
	//BOOLEAN gotport = FALSE;
	//PHYSICAL_ADDRESS portbase;

	if (translated)
	{
		PCM_PARTIAL_RESOURCE_DESCRIPTOR resource = translated->PartialDescriptors;
		ULONG nres = translated->Count;
		for (ULONG i = 0; i < nres; ++i, ++resource)
		{						// for each resource
			switch (resource->Type)
			{					// switch on resource type

			case CmResourceTypeInterrupt:
				irql = (KIRQL) resource->u.Interrupt.Level;
				vector = resource->u.Interrupt.Vector;
				affinity = resource->u.Interrupt.Affinity;
				mode = (resource->Flags == CM_RESOURCE_INTERRUPT_LATCHED) ? Latched : LevelSensitive;
				irqshare = resource->ShareDisposition == CmResourceShareShared;
#if !NO_INTERRUPT
				pdx->UseInterrupt = TRUE;
#endif
				break;

			default:
				KdPrint((SMBUS_DRIVER_NAME " - Unexpected I/O resource type %d\n", resource->Type));
			}					// switch on resource type
		}						// for each resource
	}
	if (!Hudson_Initialize(pdx)) return STATUS_UNSUCCESSFUL;

#if 0
	This stuff does not work, HalGetInterruptVector does not exist anymore on 64 bit windows.
	IoConnectInterrupt ist difficult without BIOS support.
	And it seems the interrupt is not routed correctly anyway, so forget about it untill someone needs it.

	WRITE_PORT_UCHAR((PUCHAR)0xC00, (UCHAR)(0x11 | 0x80));	// smbus0 interrupt routing ioapic
	ULONG raw_vector = (ULONG)READ_PORT_UCHAR((PUCHAR)0xC01);
	if (raw_vector == 0x1f)
	{
		KIRQL irql;
		KAFFINITY affinity;
		ULONG translated_vector = HalGetInterruptVector(InterfaceTypeUndefined , 0, 1, raw_vector, &irql, &affinity);
		status = IoConnectInterrupt(&pdx->InterruptObject, (PKSERVICE_ROUTINE) OnInterrupt,
				(PVOID) pdx, NULL, translated_vector, irql, irql, LevelSensitive , 1, affinity, FALSE);
		if (!pdx->InterruptObject)
			DbgPrint(SMBUS_DRIVER_NAME " - IoConnectInterrupt failed - %X\n", status);
		else
			DbgPrint(SMBUS_DRIVER_NAME " - IoConnectInterrupt success\n");
	}
#endif
	pdx->StartCommand = (pdx->InterruptObject) ? (SMBUS_HST_CNT_START | SMBUS_HST_CNT_INTREN) : SMBUS_HST_CNT_START;
	
	return STATUS_SUCCESS;
	}	

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID StopDevice(IN PDEVICE_OBJECT fdo, BOOLEAN /*oktouch = FALSE */)
	{							// StopDevice
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	if (pdx->InterruptObject)
		IoDisconnectInterrupt(pdx->InterruptObject);
	pdx->InterruptObject = NULL;


	if (pdx->AcpiMMioAddr && pdx->bIoMapped)
		MmUnmapIoSpace(pdx->AcpiMMioAddr, 8);
	pdx->AcpiMMioAddr = NULL;

	if (pdx->watchdogcontrol)
	{
		*pdx->watchdogcontrol = *(pdx->watchdogcontrol) & ~1;	// stop watchdog
		// disable wd
		WRITE_PORT_UCHAR((PUCHAR)0xCD6, (UCHAR)0x48);
		UCHAR tmp = READ_PORT_UCHAR((PUCHAR)0xCD7);
		WRITE_PORT_UCHAR((PUCHAR)0xCD7, tmp | 2);	// watchdog function disable

		MmUnmapIoSpace(pdx->watchdogcontrol, 8);
	}
	pdx->watchdogcontrol = NULL;
}							// StopDevice
