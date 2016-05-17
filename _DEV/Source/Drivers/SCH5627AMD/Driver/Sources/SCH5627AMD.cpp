#include "stddcls.h"
#include "driver.h"
#include "DebugPrint.h"

#include "SCH5627AMD.h"
#include "FSCIoctl.h"


// all in ms
#define SEMAPHORE_WAIT	500	// 500 ms
#define STATUS_WAIT		100
#define BUSY_WAIT		100


// This is nearly all locked code
#pragma LOCKEDCODE

#define RelativeInterval_1ms	((LONGLONG)(-10*1000))


///////////////////////////////////////////////////////////////////////////////
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

//
// The polling stuff.
//
VOID DpcForPoll(PKDPC /*Dpc*/, PDEVICE_OBJECT fdo, PVOID, PVOID)
{
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;

	pdx->InterruptSource = READ_PORT_UCHAR(&pdx->IOBase[8]);
	switch (pdx->InterruptSource & 0x51)
	{
	case 0:
		// Polling not yet done: re-schedule this DPC again in 1 msec.
		LARGE_INTEGER	liInterval;	liInterval.QuadPart = RelativeInterval_1ms;
		KeSetTimer( &pdx->Timer, liInterval, &pdx->PollDpc );
		break;
	
	default:
		// Polling done: run DpcForIsr
		KeInsertQueueDpc( &fdo->Dpc, NULL, pdx );
		break;
	}
}

// I don't understand this stuff, SMSC documentation is written by people from mars
static void SetupMailbox(PDEVICE_EXTENSION pdx, BOOLEAN write)
{
	unsigned short Address = pdx->SMBusInfo.CommandCode + (pdx->SMBusInfo.CommandCodeHigh<<8);
	unsigned char  packetheader = write ? 1 : 0;
	if      (Address < 0x1000) { packetheader |= 2; Address &= 0xfff; }	// VREG
	else if (Address < 0x2000) { packetheader |= 8; Address &= 0x3ff; }	// VBAT
	else if (Address < 0x4000) { packetheader |= 4; Address &= 0x1fff;}	// GPIO
	else                       { packetheader |= 2; Address &= 0xfff; }	// assume VREG

	PUCHAR port = pdx->IOBase;

	if (write && (Address == 0x58B))
        if (READ_PORT_UCHAR(&port[9]) & 1)
			WRITE_PORT_UCHAR(&port[9], 0x1);		// clear WDT interrupt

	WRITE_PORT_UCHAR(&port[8], 0x31);		// clear interrupt source register

	UCHAR tmp = READ_PORT_UCHAR(&port[1]);	// clear EC-to-HOST mailbox
	WRITE_PORT_UCHAR(&port[1], tmp);

	// Mailbox address pointer to region 1
	WRITE_PORT_UCHAR(&port[2], 0);
	WRITE_PORT_UCHAR(&port[3], 0x80);
	
	// write the request packet header
	WRITE_PORT_UCHAR(&port[4], packetheader);
	WRITE_PORT_UCHAR(&port[5], 1);			// 1 byte

	// Mailbox address pointer to 1st data entry region 1
	WRITE_PORT_UCHAR(&port[2], 4);

	// write the request packet data
	if (write)
		WRITE_PORT_UCHAR(&port[4], pdx->SMBusInfo.DataByteLow);
	WRITE_PORT_UCHAR(&port[6], (UCHAR)Address);	// register address
	WRITE_PORT_UCHAR(&port[7], (UCHAR)(Address>>8));

	WRITE_PORT_UCHAR(&port[0], 1);			// random read/write
}

VOID DpcForIsr(PKDPC /*Dpc*/, PDEVICE_OBJECT fdo, PIRP /*junk*/, PVOID pVoid)
{		
	ULONG info;
	NTSTATUS status;
	PDEVICE_EXTENSION pdx = static_cast<PDEVICE_EXTENSION>(pVoid);

	PIRP Irp = GetCurrentIrp(&pdx->dqReadWrite);
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	PVOID SystemBuffer = Irp->AssociatedIrp.SystemBuffer;	

	info = 0;

	switch (stack->Parameters.DeviceIoControl.IoControlCode)
	// the mailbox address pointer should still be the same as set in FillRegs()
	{
		case IOCTL_SMBus_ByteDataRead:
			if (pdx->InterruptSource & 1)
			{
				// The documentation is so confusing that we just return whatever there is at this address
				// I assume it is the error response
				pdx->SMBusInfo.Status = READ_PORT_UCHAR(&pdx->IOBase[1]);
				// what is this for ??
				WRITE_PORT_UCHAR(&pdx->IOBase[0], 0xc0);

				// get data byte
				UCHAR ucReadByte = READ_PORT_UCHAR(&pdx->IOBase[4]);

				unsigned short Address = pdx->SMBusInfo.CommandCode + (pdx->SMBusInfo.CommandCodeHigh<<8);
				if (Address == 0x58d)
				{
					// fake read of WDT_CTRL register, return WDT timeout status as force-timeout bit
					ucReadByte = (UCHAR)((ucReadByte & ~4) | ((pdx->WatchdogTimedout) ? 4 : 0));
				}
				pdx->SMBusInfo.DataByteLow = ucReadByte;

				RtlCopyMemory( SystemBuffer, &pdx->SMBusInfo, sizeof(SMB_INFO) );
				status = STATUS_SUCCESS;
				info = sizeof(SMB_INFO);
				break;
			}
			// WDT hit or ????
			if (pdx->RetryCount--)
			{
				// retry command
				SetupMailbox(pdx, false);
				status = STATUS_PENDING;
				break;
			}
			status = STATUS_IO_DEVICE_ERROR;
			break;

		case IOCTL_SMBus_ByteDataWrite:
			if (pdx->InterruptSource & 1)
			{
				// The documentation is so confusing that we just return whatever there is at this address
				// I assume it is the error response, so you better not make errors
				pdx->SMBusInfo.Status = READ_PORT_UCHAR(&pdx->IOBase[1]);
				// what is this for ??
				WRITE_PORT_UCHAR(&pdx->IOBase[0], 0xc0);
				status = STATUS_SUCCESS;
				info = sizeof(SMB_INFO);
				break;
			}
			// WDT hit or ????
			if (pdx->RetryCount--)
			{
				// retry command
				SetupMailbox(pdx, true);
				status = STATUS_PENDING;
				break;
			}
			status = STATUS_IO_DEVICE_ERROR;
			break;

		default:
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;

	}
	if (status != STATUS_PENDING)
	{
		IoStopTimer( fdo );
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = info;
		IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
		StartNextPacket(&pdx->dqReadWrite, fdo);
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
	else
	{
		// start polling again
		LARGE_INTEGER	liInterval;	liInterval.QuadPart = RelativeInterval_1ms;
		KeSetTimer( &pdx->Timer, liInterval, &pdx->PollDpc );
	}

}							// DpcForIsr

static BOOLEAN LocateSCH5627EMI(PDEVICE_EXTENSION pdx)
{
	// Called from Init only, no need for spinlocks
	ULONG IOBase = 0,IOBaseMask = 0;
	PUCHAR PnP_Port = (PUCHAR)0x4E;
	
	unsigned short CRDeviceID;
	unsigned short CRDeviceRevision;

	// enter configuration mode
	WRITE_PORT_UCHAR( PnP_Port, 0x55 );

	// get Device ID and Device Revision
    WRITE_PORT_UCHAR(PnP_Port,0x20); 
	CRDeviceID       = (unsigned short) READ_PORT_UCHAR(PnP_Port+1); // CR20=CRDeviceId
    
	WRITE_PORT_UCHAR(PnP_Port,0x21); 
	CRDeviceRevision = (unsigned short) READ_PORT_UCHAR(PnP_Port+1); // CR21=CRDeviceRev
    
	// check for known controllers */
		if ( (CRDeviceID != SMSCECHM_CRDEVICEID_5627) &&   // SMsC SCH5627 (Antiope)
			(CRDeviceID != SMSCECHM_CRDEVICEID_5636)    ) // SMsC SCH5636 (Theseus)
		{
			WRITE_PORT_UCHAR(PnP_Port, 0xAA); // Exit configuration mode 

			PnP_Port = (PUCHAR) 0x2E;
			WRITE_PORT_UCHAR( PnP_Port, 0x55 );
			
			// get Device ID and Device Revision
			WRITE_PORT_UCHAR(PnP_Port,0x20); 
			CRDeviceID       = (unsigned short) READ_PORT_UCHAR(PnP_Port+1); // CR20=CRDeviceId
    
			WRITE_PORT_UCHAR(PnP_Port,0x21); 
			CRDeviceRevision = (unsigned short) READ_PORT_UCHAR(PnP_Port+1); // CR21=CRDeviceRev

			// check for known controllers */
			if ( (CRDeviceID != SMSCECHM_CRDEVICEID_5627) &&   // SMsC SCH5627 (Antiope)
				(CRDeviceID != SMSCECHM_CRDEVICEID_5636)    ) // SMsC SCH5636 (Theseus)
			{
				WRITE_PORT_UCHAR(PnP_Port, 0xAA); // Exit configuration mode
				return(false);
			}
		}

	// The EMI itself is logical device  0, is has no registers ....
	// Maybe ist should be illogical device ?

	// select logical device C=LPC Interface
	WRITE_PORT_UCHAR( PnP_Port, 0x07 ); WRITE_PORT_UCHAR( PnP_Port+1, 0x0C );
	// get io base of EM Interface
	WRITE_PORT_UCHAR( PnP_Port, 0x65 ); IOBaseMask = READ_PORT_UCHAR( PnP_Port+1 ) << 8;
	WRITE_PORT_UCHAR( PnP_Port, 0x64 ); IOBaseMask |= READ_PORT_UCHAR( PnP_Port+1 );

	WRITE_PORT_UCHAR( PnP_Port, 0x67 ); IOBase = READ_PORT_UCHAR( PnP_Port+1 ) << 8;
	WRITE_PORT_UCHAR( PnP_Port, 0x66 ); IOBase |= READ_PORT_UCHAR( PnP_Port+1 );

#if 1
	DbgPrint("BIOS initializes SMSC5627 I/O base = %x, Mask = %x\n", IOBase, IOBaseMask);
#endif

	// valid base address ?
	if (!(IOBaseMask & 0x8000)) return FALSE;

	IOBase &= ~(IOBaseMask & 0x7f);
	if (IOBase == 0) return FALSE;

	pdx->IOBase = (PUCHAR)0 + IOBase;

	WRITE_PORT_UCHAR( PnP_Port, 0xAA );				// exit configuration mode

	return TRUE;
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
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
	ULONG	info = 0;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG *pulSystemBuffer = (ULONG*)Irp->AssociatedIrp.SystemBuffer;
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
				*(SMBUS_HWID_T*)SystemBuffer = SMBUS_HWID_SCH5627AMD;
				info = sizeof(SMBUS_HWID_T);
				status = STATUS_SUCCESS;
			}
			break;

		case IOCTL_SMBus_ByteDataRead:
			if (CheckAndCopyIn(cbin ,cbout, &pdx->SMBusInfo, SystemBuffer, sizeof(SMB_INFO)))
			{
				if (pdx->SMBusInfo.SlaveAddress<<1 == 0x5c)
				{
					SetupMailbox(pdx, false);
					status = STATUS_PENDING;
				}
			}
			break;

		case IOCTL_SMBus_ByteDataWrite:
			if (CheckAndCopyIn(cbin ,cbout, &pdx->SMBusInfo, SystemBuffer, sizeof(SMB_INFO)))
			{
				if (pdx->SMBusInfo.SlaveAddress<<1 == 0x5c)
				{
					SetupMailbox(pdx, true);
					status = STATUS_PENDING;
				}
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
		if (pdx->WinSysMonMode)
		{
			// System engineers do not want to wait :-)
			pdx->TimeOutCounter = 5;
			pdx->RetryCount = 5;
			IoStartTimer( fdo );
			KeStallExecutionProcessor(500);
			DpcForPoll(NULL, fdo, NULL, NULL);
		}
		else
		{
			// Run Isr in polling mode every 1 msec;
			LARGE_INTEGER	liInterval;	liInterval.QuadPart = RelativeInterval_1ms;
			KeSetTimer( &pdx->Timer, liInterval, &pdx->PollDpc );
			pdx->TimeOutCounter = 5;
			pdx->RetryCount = 5;
			IoStartTimer( fdo );
		}
	}
}
#pragma PAGEDCODE

NTSTATUS StartDevice(PDEVICE_OBJECT fdo, PCM_PARTIAL_RESOURCE_LIST /*raw*/, PCM_PARTIAL_RESOURCE_LIST translated)
	{							// StartDevice
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	//NTSTATUS status;

	// we dont expect any resources
	if (translated)
	{
		//PCM_PARTIAL_RESOURCE_DESCRIPTOR resource = translated->PartialDescriptors;
		ULONG nres = translated->Count;
		if (nres)
		{
			return STATUS_UNSUCCESSFUL;
		}
	}
	if (RtlCheckRegistryKey(RTL_REGISTRY_SERVICES, L"SCH5627AMD\\Poll") == STATUS_SUCCESS)
		pdx->WinSysMonMode = 1;

	IoInitializeTimer(pdx->DeviceObject, IoTimer, NULL);

	if (!LocateSCH5627EMI(pdx)) return STATUS_UNSUCCESSFUL;

	if (READ_PORT_UCHAR(&pdx->IOBase[9]) & 1)
		pdx->WatchdogTimedout = 1;

	return STATUS_SUCCESS;
	}							// StartDevice

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID StopDevice(IN PDEVICE_OBJECT fdo, BOOLEAN /*oktouch = FALSE */)
	{							// StopDevice
	/*PDEVICE_EXTENSION pdx =*/ (PDEVICE_EXTENSION) fdo->DeviceExtension;
	}							// StopDevice
