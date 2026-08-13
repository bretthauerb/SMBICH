#include "stddcls.h"
#include "driver.h"
#include "DebugPrint.h"


#include "ICH_SMB.h"
#include "FSCIoctl.h"
#include "DMI.h"


// all in ms
#define SEMAPHORE_WAIT	500	// 500 ms
#define STATUS_WAIT		100
#define BUSY_WAIT		100

#define RelativeInterval_500ys	((LONGLONG)(-5*1000))
#define RelativeInterval_100ys  ((LONGLONG)(-1*1000))

static inline VOID	SMBus_ReleaseSemaphore( IN PDEVICE_EXTENSION pdx );
static BOOLEAN SMBus_ClearStatus( IN PDEVICE_EXTENSION pdx );

// This is nearly all locked code
#pragma LOCKEDCODE

PVOID MapEntryPoint(_In_ PHYSICAL_ADDRESS PhysicalAddress, _In_ SIZE_T NumberOfBytes)
{
	typedef PVOID(*PFN_MmMapIoSpaceEx)(_In_ PHYSICAL_ADDRESS PhysicalAddress, _In_ SIZE_T NumberOfBytes, _In_ ULONG Protect);

	static PFN_MmMapIoSpaceEx pfnMmMapIoSpaceEx = NULL;
	static BOOLEAN bAlreadyGot = FALSE;

	if (!bAlreadyGot)
	{
		UNICODE_STRING uniFuncName;
		RtlInitUnicodeString(&uniFuncName, L"MmMapIoSpaceEx");

#pragma warning(push)
#pragma warning(disable : 4055)
		pfnMmMapIoSpaceEx = (PFN_MmMapIoSpaceEx)MmGetSystemRoutineAddress(&uniFuncName);
#pragma warning(pop)
		bAlreadyGot = TRUE;
	}

	if (pfnMmMapIoSpaceEx != NULL)
	{
		return pfnMmMapIoSpaceEx(PhysicalAddress, NumberOfBytes, PAGE_READWRITE | PAGE_NOCACHE);
	}
	else
	{
		return MmMapIoSpace(PhysicalAddress, NumberOfBytes, MmNonCached);
	}
}

///////////////////////////////////////////////////////////////////////////////

//
// Timeout timer.
//
VOID IoTimer(PDEVICE_OBJECT fdo, PVOID)
{
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	if (pdx->TimeOutCounter) {
		pdx->TimeOutCounter--;
	} else {
		/*KIRQL oldirql;*/
		//
		// TODO: there is a race here.
		//
		DisableInterrupt( pdx );
		KeCancelTimer(&pdx->Timer);

		SMBus_ReleaseSemaphore( pdx );
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
    if (ucHostStatus & SMBUS_HST_STA_BYTE_DONE_STS )    // this happens for a Block Read
        return STATUS_SUCCESS;
	if (ucHostStatus & SMBUS_HST_STA_INTR)
		return STATUS_SUCCESS;
	if (ucHostStatus & SMBUS_HST_STA_DEV_ERR)
		return STATUS_NO_SUCH_DEVICE;
	if (ucHostStatus & (SMBUS_HST_STA_BUS_ERR | SMBUS_HST_STA_FAILED))
		return STATUS_IO_DEVICE_ERROR;
	if (ucHostStatus & SMBUS_HST_STA_HOST_BUSY)
		return STATUS_TRANSACTION_TIMED_OUT;

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

		// Polling not yet done: re-schedule this DPC again in 100 ysec.
		LARGE_INTEGER	liInterval;	liInterval.QuadPart = RelativeInterval_100ys;
		KeSetTimer( &pdx->Timer, liInterval, &pdx->PollDpc );
	} else {
		// Polling done: run DpcForIsr
		IoRequestDpc(fdo, NULL, pdx);
	}
}

#if DBG
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
#endif //#ifdef _USED_


// TRUE: next byte ready;  FALSE: all work done
BOOLEAN WaitForNextByte( PDEVICE_EXTENSION pdx )
{
    ULONG Retries = 100;
	UCHAR HostStatus = READ_PORT_UCHAR(pdx->portbase + SMBUS_HOST_STATUS_REGISTER);
    KdPrint(("WaitForNextByte: HostStatus = 0x%X\n",HostStatus));

    // Clear the BYTE DONE bit
    WRITE_PORT_UCHAR( pdx->portbase + SMBUS_HOST_STATUS_REGISTER, SMBUS_HST_STA_BYTE_DONE_STS );

    // Check whether the host is still busy or already done
    if ( 0 == (HostStatus & SMBUS_HST_STA_HOST_BUSY) &&  (HostStatus & SMBUS_HST_STA_INTR) )
    {
        KdPrint(("WaitForNextByte: returning FALSE (shortcut)\n"));
        return FALSE;
    }

    // Wait for the bit coming up again ...
    while ( Retries-- )
    {
        HostStatus = READ_PORT_UCHAR(pdx->portbase + SMBUS_HOST_STATUS_REGISTER);
        KdPrint(("WaitForNextByte: HostStatus = 0x%X\n",HostStatus));
        if ( HostStatus & SMBUS_HST_STA_BYTE_DONE_STS )
        {
            KdPrint(("WaitForNextByte: returning TRUE\n"));
            return TRUE;
        }
    }

    KdPrint(("WaitForNextByte: returning FALSE\n"));
    return FALSE;
}


VOID DpcForIsr(PKDPC /*Dpc*/, PDEVICE_OBJECT /*fdo*/, PIRP /*junk*/, PVOID pVoid)
{		
	ULONG info;
	NTSTATUS status;
	PDEVICE_EXTENSION pdx = static_cast<PDEVICE_EXTENSION>(pVoid);
	PIRP Irp = GetCurrentIrp(&pdx->dqReadWrite);
	if (Irp == NULL) {
		DbgPrint(SMBUS_DRIVER_NAME" ERROR: DpcForIsr called with no current IRP\n");
		return;
	}
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	PVOID SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
	PUCHAR portbase = pdx->portbase;

	UCHAR HostStatus = READ_PORT_UCHAR(portbase + SMBUS_HOST_STATUS_REGISTER);
    KdPrint(("DpcForIsr: HostStatus = 0x%X\n", HostStatus));
	status = HostStatus2NtStatus(HostStatus);
    KdPrint(("DpcForIsr:     Status = 0x%X\n", status));

	info = 0;

	switch (stack->Parameters.DeviceIoControl.IoControlCode)
	{						// process request
		case IOCTL_SMBus_ByteDataRead:
			if (NT_SUCCESS(status)) {
				pdx->SMBusInfo.Status = status;
				pdx->SMBusInfo.DataByteLow = READ_PORT_UCHAR( portbase + SMBUS_HOST_DATA0_REGISTER );
				RtlCopyMemory( SystemBuffer, &pdx->SMBusInfo, sizeof(SMB_INFO) );
				info = sizeof(SMB_INFO);
//				KdPrint(("IOCTL_SMBus_ByteDataRead: %x\n", pdx->SMBusInfo.DataByteLow));
			}
			else
			{
				// BUS error, retry access assuming arbitration was lost
				if ((HostStatus & SMBUS_HST_STA_BUS_ERR) && pdx->TimeOutCounter > 1)
				{
					SMBus_ClearStatus( pdx );
#if DBG
					pdx->RetryCount++;
#endif
					// Write Slave Address
					WRITE_PORT_UCHAR( portbase + SMBUS_HOST_ADDRESS_REGISTER, (UCHAR) ((pdx->SMBusInfo.SlaveAddress<<1) | 1) );
					// Write Command
					WRITE_PORT_UCHAR( portbase + SMBUS_HOST_COMMAND_REGISTER, (UCHAR) pdx->SMBusInfo.CommandCode );
					// Write Control (Command Protocol)
					WRITE_PORT_UCHAR( portbase + SMBUS_HOST_CONTROL_REGISTER, SMBUS_HST_CNT_CMD_BYTE_DATA | pdx->StartCommand );

					status = STATUS_PENDING;
				}
			}
			break;

		case IOCTL_SMBus_ByteDataWrite:
			if (NT_SUCCESS(status)) {
				pdx->SMBusInfo.Status = status;
				RtlCopyMemory( SystemBuffer, &pdx->SMBusInfo, sizeof(SMB_INFO) );
				info = sizeof(SMB_INFO);
			}
			else
			{
				// BUS error, retry access assuming arbitration was lost
				if ((HostStatus & SMBUS_HST_STA_BUS_ERR) && pdx->TimeOutCounter > 1)
				{
					SMBus_ClearStatus( pdx );
#if DBG
					pdx->RetryCount++;
#endif
					// Write Slave Address
					WRITE_PORT_UCHAR( portbase + SMBUS_HOST_ADDRESS_REGISTER, (UCHAR) ((pdx->SMBusInfo.SlaveAddress<<1) & ~1) );
					// Write Command
					WRITE_PORT_UCHAR( portbase + SMBUS_HOST_COMMAND_REGISTER, (UCHAR) pdx->SMBusInfo.CommandCode );
					// Write Data Byte
					WRITE_PORT_UCHAR( portbase + SMBUS_HOST_DATA0_REGISTER, (UCHAR) pdx->SMBusInfo.DataByteLow );
					// Write Control (Command Protocol)
					WRITE_PORT_UCHAR( portbase + SMBUS_HOST_CONTROL_REGISTER, SMBUS_HST_CNT_CMD_BYTE_DATA | pdx->StartCommand );
					status = STATUS_PENDING;
				}
			}
			break;

		case IOCTL_SMBus_WordDataRead:
			if (NT_SUCCESS(status)) {
				pdx->SMBusInfo.Status       = status;
				pdx->SMBusInfo.DataByteLow  = READ_PORT_UCHAR( portbase + SMBUS_HOST_DATA0_REGISTER );
				pdx->SMBusInfo.DataByteHigh = READ_PORT_UCHAR( portbase + SMBUS_HOST_DATA1_REGISTER );
				RtlCopyMemory( SystemBuffer, &pdx->SMBusInfo, sizeof(SMB_INFO) );
				info = sizeof(SMB_INFO);
				KdPrint(("IOCTL_SMBus_WordDataRead: High = 0x%X, Low = 0x%X\n", pdx->SMBusInfo.DataByteHigh, pdx->SMBusInfo.DataByteLow));
			}
			else
			{
				// BUS error, retry access assuming arbitration was lost
				if ((HostStatus & SMBUS_HST_STA_BUS_ERR) && pdx->TimeOutCounter > 1)
				{
					SMBus_ClearStatus( pdx );
#if DBG
					pdx->RetryCount++;
#endif
					// Write Slave Address
					WRITE_PORT_UCHAR( portbase + SMBUS_HOST_ADDRESS_REGISTER, (UCHAR) ((pdx->SMBusInfo.SlaveAddress<<1) | 1) );
					// Write Command
					WRITE_PORT_UCHAR( portbase + SMBUS_HOST_COMMAND_REGISTER, (UCHAR) pdx->SMBusInfo.CommandCode );
					// Write Control (Command Protocol)
					WRITE_PORT_UCHAR( portbase + SMBUS_HOST_CONTROL_REGISTER, SMBUS_HST_CNT_CMD_BYTE_DATA | pdx->StartCommand );

					status = STATUS_PENDING;
				}
			}
			break;

        //
        // SMBus Block Read is byte-driven.
        // BYTE_DONE must be acknowledged after each byte read,
        // otherwise the host controller keeps presenting the same
        // BLOCK_DATA value repeatedly.
        //
		case IOCTL_SMBus_BlockRead:
			if (NT_SUCCESS(status)) {
                constexpr const size_t BlockBufSize = sizeof(SMB_INFO::BlockBuf)/sizeof(SMB_INFO::BlockBuf[0]);

				pdx->SMBusInfo.Status = status;
				KdPrint(("IOCTL_SMBus_BlockRead: Status = 0x%X\n", pdx->SMBusInfo.Status));
				pdx->SMBusInfo.Count  = READ_PORT_UCHAR( portbase + SMBUS_HOST_DATA0_REGISTER );
				KdPrint(("IOCTL_SMBus_BlockRead: Count = 0x%X\n", pdx->SMBusInfo.Count));
                for ( size_t i = 0 ; i < pdx->SMBusInfo.Count && i < BlockBufSize ; ++i )
                {
                    pdx->SMBusInfo.BlockBuf[i] = READ_PORT_UCHAR( portbase + SMBUS_HOST_BLOCKDATA_REGISTER );
                    KdPrint(("IOCTL_SMBus_BlockRead: BlockBuf[%Iu] = 0x%X\n", i, pdx->SMBusInfo.BlockBuf[i]));
                    WaitForNextByte( pdx );
                }
				RtlCopyMemory( SystemBuffer, &pdx->SMBusInfo, sizeof(SMB_INFO) );
				info = sizeof(SMB_INFO);
				KdPrint(("IOCTL_SMBus_BlockRead: at eof\n"));
			}
			else
			{
				KdPrint(("IOCTL_SMBus_BlockRead: after BUS error\n"));
				// BUS error, retry access assuming arbitration was lost
				if ((HostStatus & SMBUS_HST_STA_BUS_ERR) && pdx->TimeOutCounter > 1)
				{
					SMBus_ClearStatus( pdx );
#if DBG
					pdx->RetryCount++;
#endif
					// Write Slave Address
					WRITE_PORT_UCHAR( portbase + SMBUS_HOST_ADDRESS_REGISTER, (UCHAR) ((pdx->SMBusInfo.SlaveAddress<<1) | 1) );
					// Write Command
					WRITE_PORT_UCHAR( portbase + SMBUS_HOST_COMMAND_REGISTER, (UCHAR) pdx->SMBusInfo.CommandCode );
					// Write Control (Command Protocol)
					WRITE_PORT_UCHAR( portbase + SMBUS_HOST_CONTROL_REGISTER, SMBUS_HST_CNT_CMD_BLOCK | pdx->StartCommand );

					status = STATUS_PENDING;
				}
			}
			break;

		default:
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;

	}
	if (status != STATUS_PENDING)
	{
#if DBG
		if (!NT_SUCCESS(status))
			e2t(HostStatus);
		if (pdx->RetryCount)
			DbgPrint(SMBUS_DRIVER_NAME" bus collision occured, command was retried %d times\n", pdx->RetryCount);
#endif
		IoStopTimer( fdo );
		SMBus_ReleaseSemaphore( pdx );
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = info;
		IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
		StartNextPacket(&pdx->dqReadWrite, fdo);
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
	else
	{
		// start polling again
		LARGE_INTEGER	liInterval;	liInterval.QuadPart = RelativeInterval_100ys;
		KeSetTimer( &pdx->Timer, liInterval, &pdx->PollDpc );
	}

}							// DpcForIsr

VOID DisableInterrupt( IN PDEVICE_EXTENSION pdx )
{
	PUCHAR p = pdx->portbase + SMBUS_HOST_CONTROL_REGISTER;
	UCHAR	HostControl = READ_PORT_UCHAR( p );
	WRITE_PORT_UCHAR( p, HostControl & ~SMBUS_HST_CNT_INTREN );
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
	PUCHAR portbase = pdx->portbase;

//KdPrint(("OnInterrupt, portbase = %x\n", portbase));

	if (pdx->StalledForPower)
		// Then it cannot be my interrupt.
		return FALSE;

	if (InterruptObject) {
		// Interrupts disabled ?
		if (!(READ_PORT_UCHAR( portbase + SMBUS_HOST_CONTROL_REGISTER) & SMBUS_HST_CNT_INTREN))
			return FALSE;
	}

//KdPrint(("OnInterrupt, interrupts are enabled.\n"));
	// Interrupt active ?
	HostStatus = READ_PORT_UCHAR( portbase + SMBUS_HOST_STATUS_REGISTER);
	if ((HostStatus & SMBUS_HST_STA_INT_MASK) == 0)
		return FALSE;

//KdPrint(("OnInterrupt, it is our interrupt; HOST_STATUS = %x, CONTROL = %x\n", HostStatus, READ_PORT_UCHAR( portbase + SMBUS_HOST_CONTROL_REGISTER)));
	// Yes, it is our interrupt.
	// Is a block transfer interrupt ?
	if ((!pdx->bPIIX4) && (HostStatus & SMBUS_HST_STA_BYTE_DONE_STS))
	{
		// Do we expect more data ?
		if (pdx->block_tranfer.count)
		{
			// Handle block interrupt only, other status will be handled on next interrupt.
			if (pdx->block_tranfer.mode == pdx->block_tranfer.WRITE)
			{
				WRITE_PORT_UCHAR( portbase + SMBUS_HOST_BLOCKDATA_REGISTER, *pdx->block_tranfer.address++);
				pdx->block_tranfer.count--;
				// clear ByteDone status
				WRITE_PORT_UCHAR( portbase + SMBUS_HOST_STATUS_REGISTER, SMBUS_HST_STA_BYTE_DONE_STS );
				return (InterruptObject) ? TRUE : FALSE;
			}
			if (pdx->block_tranfer.mode == pdx->block_tranfer.READ)
			{
				*pdx->block_tranfer.address++ = READ_PORT_UCHAR( portbase + SMBUS_HOST_BLOCKDATA_REGISTER );
				pdx->block_tranfer.count--;
				// clear ByteDone status
				WRITE_PORT_UCHAR( portbase + SMBUS_HOST_STATUS_REGISTER, SMBUS_HST_STA_BYTE_DONE_STS );
				return (InterruptObject) ? TRUE : FALSE;
			}
		}			
	}
	// Command should be finished, real work is done in Dpc
	DisableInterrupt( pdx );
	if (InterruptObject)
		IoRequestDpc(pdx->DeviceObject, NULL, pdx);
	return TRUE;
}							// OnInterrupt

//_______SMBus_ClearStatus_____________________________________________________________________
//_______
//_______	Functionality:	Wait for Busy to become clear, then clear all interrupt status bits.
//_______					Incase Busy does not clear, the kill bit will be set to abort the
//							current operation. Prepare for next command.
//_______________________________________________________________________________________

static BOOLEAN SMBus_ClearStatus( IN PDEVICE_EXTENSION pdx )
{
	PUCHAR	StatusPort = pdx->portbase + SMBUS_HOST_STATUS_REGISTER;
	PUCHAR	CtrlPort   = pdx->portbase + SMBUS_HOST_CONTROL_REGISTER;
	
	// wait for busy inactive
	for (int retry = BUSY_WAIT; retry--; )
	{
		if ((READ_PORT_UCHAR( StatusPort ) & SMBUS_HST_STA_HOST_BUSY) == 0)
			break;

		// Time to kill :-) ?
		if (retry == 0)
		{
			DebugPrint(DEBUGLEVEL_ERROR, "SMBus_ClearStatus : Kill Function, HostCtrl = %02X, HostStatus = %02X",
					READ_PORT_UCHAR( CtrlPort ), READ_PORT_UCHAR( StatusPort ));

			WRITE_PORT_UCHAR( CtrlPort, SMBUS_HST_CNT_KILL );
			KeStallExecutionProcessor(50);
			WRITE_PORT_UCHAR( CtrlPort, 0 );
		}
		KeStallExecutionProcessor(50);
	}

	// Clear all interrupt status bits
	WRITE_PORT_UCHAR( StatusPort, SMBUS_HST_STA_INT_MASK);

	if (pdx->UseInterrupt)
	{
		pdx->block_tranfer.mode = pdx->block_tranfer.NONE;
		pdx->block_tranfer.count = 0;
	}
	// Controller should be idle now
	return (READ_PORT_UCHAR( StatusPort ) & (SMBUS_HST_STA_INT_MASK | SMBUS_HST_STA_HOST_BUSY)) ? FALSE : TRUE;
}


//_______SMBus_ReleaseSemaphore___________________________________________________________
//_______
//_______	Functionality:	Releases the INUSE_STS Semaphore (ICH ONLY!!)
//_______
//_______	Parameters:		Input : pdx = device extension
//_______
//_______	Return values:	--
//_______________________________________________________________________________________

static inline VOID	SMBus_ReleaseSemaphore( IN PDEVICE_EXTENSION pdx )
{
	// Leave a clean status.
	SMBus_ClearStatus( pdx );
	if (!pdx->bPIIX4)
		WRITE_PORT_UCHAR( pdx->portbase + SMBUS_HOST_STATUS_REGISTER, SMBUS_HST_STA_INUSE_STS );
}

static VOID SMBus_AcquireSemaphore( IN PDEVICE_EXTENSION pdx )
{
	if (!pdx->bPIIX4)
	{
		for ( int retry = SEMAPHORE_WAIT; retry--; )
		{
			//
			// Acquire INUSE Semaphore
			//
			if ((READ_PORT_UCHAR( pdx->portbase + SMBUS_HOST_STATUS_REGISTER ) & SMBUS_HST_STA_INUSE_STS) == 0)
				return;

			//
			// TODO:
			// Replace by someting better.
			//
			KeStallExecutionProcessor(50);
		}
		DebugPrint(DEBUGLEVEL_ERROR, "SMBus_ByteRead : Semaphore could not be acquired!!");
	}
}

static BOOLEAN CheckAndCopyIn(ULONG cbin, ULONG cbout, PVOID dest, PVOID src, unsigned size)
{
	if (cbin < size || cbout < size) return FALSE;
	RtlCopyMemory( dest, src, size);
	return TRUE;
}

VOID StartIo(PDEVICE_OBJECT fdo, PIRP Irp)
{
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	NTSTATUS status;
	ULONG	info = 0;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG *pulSystemBuffer = (ULONG*)Irp->AssociatedIrp.SystemBuffer;
	PVOID SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
	PUCHAR portbase = pdx->portbase;

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
				*pulSystemBuffer = 1;
				info = sizeof(*pulSystemBuffer);
				status = STATUS_SUCCESS;
			}
			break;

		case IOCTL_FSC_GET_DRIVER_VERSION:
			if (cbin == 0 && cbout == sizeof(*pulSystemBuffer))
			{
				*pulSystemBuffer = DRIVER_VERSION;
				info = sizeof(*pulSystemBuffer);
				status = STATUS_SUCCESS;
			}
			break;

		case IOCTL_GET_HARDWARE_ID:
			if (cbin == 0 && cbout == sizeof(SMBUS_HWID_T))
			{
				*(SMBUS_HWID_T*)SystemBuffer = SMBUS_DRIVER_ID;
				info = sizeof(SMBUS_HWID_T);
				status = STATUS_SUCCESS;
			}
			break;

		case IOCTL_SMBus_ByteDataRead:
			if (CheckAndCopyIn(cbin ,cbout, &pdx->SMBusInfo, SystemBuffer, sizeof(SMB_INFO)))
			{
				SMBus_AcquireSemaphore(pdx);

				SMBus_ClearStatus( pdx );
#if DBG
				pdx->RetryCount = 0;
#endif
				// Write Slave Address
				WRITE_PORT_UCHAR( portbase + SMBUS_HOST_ADDRESS_REGISTER, (UCHAR) ((pdx->SMBusInfo.SlaveAddress<<1) | 1) );
				// Write Command
				WRITE_PORT_UCHAR( portbase + SMBUS_HOST_COMMAND_REGISTER, (UCHAR) pdx->SMBusInfo.CommandCode );
				// Write Control (Command Protocol)
				WRITE_PORT_UCHAR( portbase + SMBUS_HOST_CONTROL_REGISTER, SMBUS_HST_CNT_CMD_BYTE_DATA | pdx->StartCommand );

				status = STATUS_PENDING;
			}
			break;

		case IOCTL_SMBus_ByteDataWrite:
			if (CheckAndCopyIn(cbin ,cbout, &pdx->SMBusInfo, SystemBuffer, sizeof(SMB_INFO)))
			{
				SMBus_AcquireSemaphore(pdx);

				SMBus_ClearStatus( pdx );

#if DBG
				pdx->RetryCount = 0;
#endif
				// Write Slave Address
				WRITE_PORT_UCHAR( portbase + SMBUS_HOST_ADDRESS_REGISTER, (UCHAR) ((pdx->SMBusInfo.SlaveAddress<<1) & ~1) );
				// Write Command
				WRITE_PORT_UCHAR( portbase + SMBUS_HOST_COMMAND_REGISTER, (UCHAR) pdx->SMBusInfo.CommandCode );
				// Write Data Byte
				WRITE_PORT_UCHAR( portbase + SMBUS_HOST_DATA0_REGISTER, (UCHAR) pdx->SMBusInfo.DataByteLow );
				// Write Control (Command Protocol)
				WRITE_PORT_UCHAR( portbase + SMBUS_HOST_CONTROL_REGISTER, SMBUS_HST_CNT_CMD_BYTE_DATA | pdx->StartCommand );

				status = STATUS_PENDING;
			}
			break;

		case IOCTL_SMBus_WordDataRead:
			if (CheckAndCopyIn(cbin ,cbout, &pdx->SMBusInfo, SystemBuffer, sizeof(SMB_INFO)))
			{
				SMBus_AcquireSemaphore(pdx);

				SMBus_ClearStatus( pdx );
#if DBG
				pdx->RetryCount = 0;
#endif
				// Write Slave Address
				WRITE_PORT_UCHAR( portbase + SMBUS_HOST_ADDRESS_REGISTER, (UCHAR) ((pdx->SMBusInfo.SlaveAddress<<1) | 1) );
				// Write Command
				WRITE_PORT_UCHAR( portbase + SMBUS_HOST_COMMAND_REGISTER, (UCHAR) pdx->SMBusInfo.CommandCode );
				// Write Control (Command Protocol)
				WRITE_PORT_UCHAR( portbase + SMBUS_HOST_CONTROL_REGISTER, SMBUS_HST_CNT_CMD_WORD_DATA | pdx->StartCommand );

				status = STATUS_PENDING;
			}
			break;

		case IOCTL_SMBus_BlockRead:
            KdPrint(("IOCTL_SMBus_BlockRead: StartIo\n"));
			if (CheckAndCopyIn(cbin ,cbout, &pdx->SMBusInfo, SystemBuffer, sizeof(SMB_INFO)))
			{
                KdPrint(("IOCTL_SMBus_BlockRead: CheckAndCopyIn() succeeded\n"));

				SMBus_AcquireSemaphore(pdx);

#if DBG
				BOOLEAN Cleared = SMBus_ClearStatus( pdx );
                KdPrint(("IOCTL_SMBus_BlockRead: SMBus_ClearStatus() returned 0x%X\n",Cleared));
#else
                SMBus_ClearStatus( pdx );
#endif

#if DBG
				pdx->RetryCount = 0;
#endif
				// Write Slave Address
				WRITE_PORT_UCHAR( portbase + SMBUS_HOST_ADDRESS_REGISTER, (UCHAR) ((pdx->SMBusInfo.SlaveAddress<<1) | 1) );
				// Write Command
				WRITE_PORT_UCHAR( portbase + SMBUS_HOST_COMMAND_REGISTER, (UCHAR) pdx->SMBusInfo.CommandCode );
				// Write Control (Command Protocol)
				WRITE_PORT_UCHAR( portbase + SMBUS_HOST_CONTROL_REGISTER, SMBUS_HST_CNT_CMD_BLOCK | pdx->StartCommand );

				status = STATUS_PENDING;
			}
			break;

		case IOCTL_GET_SMBIOS_SIZE:
			if (pdx->pucDMI && pdx->ulDMISize && (cbout == sizeof(DMI_SIZE_T)))
			{
					((DMI_SIZE_T*)SystemBuffer)->Size = pdx->ulDMISize;
					((DMI_SIZE_T*)SystemBuffer)->DMIStructCount = pdx->ulDMIStructCount;
					info = sizeof(DMI_SIZE_T);
					status = STATUS_SUCCESS;
			}
			break;

		case IOCTL_GET_SMBIOS:
			if (pdx->pucDMI && pdx->ulDMISize && (cbout >= pdx->ulDMISize))
			{
					RtlCopyMemory(SystemBuffer, pdx->pucDMI, pdx->ulDMISize);
					info = pdx->ulDMISize;
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
		if (!pdx->UseInterrupt) {
			// Run Isr in polling mode every 1 msec;
			LARGE_INTEGER	liInterval;	liInterval.QuadPart = RelativeInterval_500ys;
			KeSetTimer( &pdx->Timer, liInterval, &pdx->PollDpc );
		}
		pdx->TimeOutCounter = 5;
		IoStartTimer( fdo );
	}
}

//  #pragma INITCODE
#pragma PAGEDCODE

VOID ICH_Initialize( IN PDEVICE_EXTENSION pdx )
{
	PAGED_CODE();

	if (!pdx->bPIIX4)
		SMBus_ReleaseSemaphore( pdx );

#if 0
	if (pdx->UseInterrupt)
	{
		UCHAR	Value;
		NTSTATUS status;
		// Use IRQ, not SMI
		status = ReadWritePCISpace( pdx->Pdo, FALSE, &Value, 0x40, sizeof Value);
		if (!NT_SUCCESS(status) || (Value & 2))
		{
			pdx->UseInterrupt = FALSE;

		}
	}
#endif
	pdx->StartCommand = (pdx->UseInterrupt) ? (SMBUS_HST_CNT_START | SMBUS_HST_CNT_INTREN) : SMBUS_HST_CNT_START;
	DebugPrint(DEBUGLEVEL_DEBUG, "bPIIX4 = %x;  UseInterrupt = %x\n", pdx->bPIIX4, pdx->UseInterrupt);
	KdPrint(("ICH_Initialize: bPIIX4 = 0x%X;  UseInterrupt = 0x%X\n", pdx->bPIIX4, pdx->UseInterrupt));
}

#pragma PAGEDCODE

NTSTATUS StartDevice(PDEVICE_OBJECT fdo, PCM_PARTIAL_RESOURCE_LIST /*raw*/, PCM_PARTIAL_RESOURCE_LIST translated)
	{							// StartDevice
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	NTSTATUS status;

	// Identify the I/O resources we're supposed to use.
	
	// TODO: check if new default values work

	ULONG vector = 0;
	KIRQL irql = 0;
	KINTERRUPT_MODE mode = KINTERRUPT_MODE::Latched;
	KAFFINITY affinity = 0;
	BOOLEAN irqshare = FALSE;
	BOOLEAN gotport = FALSE;
	PHYSICAL_ADDRESS portbase = { 0 };

	pdx->UseInterrupt = FALSE;

	if (translated)
	{
		PCM_PARTIAL_RESOURCE_DESCRIPTOR resource = translated->PartialDescriptors;
		ULONG nres = translated->Count;
		for (ULONG i = 0; i < nres; ++i, ++resource)
		{						// for each resource
			switch (resource->Type)
			{					// switch on resource type

			case CmResourceTypePort:
				portbase = resource->u.Port.Start;
				pdx->nports = resource->u.Port.Length;
//				Cannot occur for ICH
//				pdx->mappedport = (resource->Flags & CM_RESOURCE_PORT_IO) == 0;
				gotport = TRUE;
				break;
			
			case CmResourceTypeInterrupt:
				irql = (KIRQL) resource->u.Interrupt.Level;
				vector = resource->u.Interrupt.Vector;
				affinity = resource->u.Interrupt.Affinity;
				mode = (resource->Flags == CM_RESOURCE_INTERRUPT_LATCHED)
					? Latched : LevelSensitive;
				irqshare = resource->ShareDisposition == CmResourceShareShared;

#if !NO_INTERRUPT
				pdx->UseInterrupt = TRUE;
#endif
				break;

			default:
				KdPrint((SMBUS_DRIVER_NAME " - Unexpected I/O resource type %d\n", resource->Type));
			// IWill board returns this resource ???
			case 129:
				break;
			}					// switch on resource type
		}						// for each resource
	}

#if 0
	// map port address for RISC platform
	if (pdx->mappedport)
	{						// map port address for RISC platform
		pdx->portbase = (PUCHAR)MapEntryPoint(portbase, pdx->nports);
		if (!pdx->mappedport)
		{
			KdPrint((SMBUS_DRIVER_NAME " - Unable to map port range %I64X, length %X\n", portbase, pdx->nports));
			return STATUS_INSUFFICIENT_RESOURCES;
		}
	}
	else
#endif
	if (gotport)
	{
		pdx->portbase = (PUCHAR)0 + portbase.LowPart;
	}
	else
	{
		if (pdx->portbase == NULL)
		{
			KdPrint((SMBUS_DRIVER_NAME " - Didn't get expected I/O resources\n"));
			return STATUS_DEVICE_CONFIGURATION_ERROR;
		}
	}

	// Do some HW initialisation
	ICH_Initialize( pdx );
    KdPrint(("StartDevice: pdx->portbase = %p\n", pdx->portbase));

	if (pdx->UseInterrupt)
		{
		// Temporarily prevent device from interrupt if that's possible.
		DisableInterrupt( pdx );
		status = IoConnectInterrupt(&pdx->InterruptObject, (PKSERVICE_ROUTINE) OnInterrupt,
			(PVOID) pdx, NULL, vector, irql, irql, mode, irqshare, affinity, FALSE);
		if (!NT_SUCCESS(status))
			{
			KdPrint((SMBUS_DRIVER_NAME " - IoConnectInterrupt failed - %X\n", status));
#if 0
			if (pdx->portbase && pdx->mappedport)
				MmUnmapIoSpace(pdx->portbase, pdx->nports);
#endif
			pdx->portbase = NULL;
			return status;
			}
		}

	MapDMI(pdx);
	
	return STATUS_SUCCESS;
	}							// StartDevice

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID StopDevice(IN PDEVICE_OBJECT fdo, BOOLEAN /*oktouch = FALSE */)
	{							// StopDevice
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	if (pdx->InterruptObject)
		{						// disconnect interrupt

		// prevent device from generating more interrupts if possible
		DisableInterrupt( pdx );

		IoDisconnectInterrupt(pdx->InterruptObject);
		pdx->InterruptObject = NULL;
		}						// disconnect interrupt
#if 0
	if (pdx->portbase && pdx->mappedport)
		MmUnmapIoSpace(pdx->portbase, pdx->nports);
#endif
	pdx->portbase = NULL;

	UnmapDMI(pdx);
	}							// StopDevice
