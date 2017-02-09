#include "stddcls.h"
#include "driver.h"
#include "DebugPrint.h"

#include "FSCIoctl.h"
#include "WatchdogIoctl.h"

// all in ms
#define SEMAPHORE_WAIT	500	// 500 ms
#define STATUS_WAIT		100
#define BUSY_WAIT		100

#define RelativeInterval_1ms	((LONGLONG)(-10*1000))


static inline VOID	SMBus_ReleaseSemaphore( IN PDEVICE_EXTENSION pdx );

// This is nearly all locked code
#pragma LOCKEDCODE

///////////////////////////////////////////////////////////////////////////////

static BOOLEAN LocateW83627(PDEVICE_EXTENSION pdx)
{
	// Called from Init only, no need for spinlocks
	ULONG IOBase = 0;
	PUCHAR PnP_Port;
	
	PUCHAR CRBase[2] = { (PUCHAR) 0x2E, (PUCHAR) 0x4E };
	UCHAR CRDeviceID;

	// check the two possible CRBase Addresses (0x2E and 0x4E)
	for (int i=0;i<2;i++)
	{
		PnP_Port = CRBase[i];

		/* get/set IOBase by access p&p configuration registers of logical device B=Hardware Monitoring */
		// enter extended function mode 
		WRITE_PORT_UCHAR( PnP_Port, 0x87 ); WRITE_PORT_UCHAR( PnP_Port, 0x87 );
		// select logical device B=HardwareMonitoring
		WRITE_PORT_UCHAR( PnP_Port, 0x07 ); WRITE_PORT_UCHAR( PnP_Port+1, 0x0B );
		// get io base
		WRITE_PORT_UCHAR( PnP_Port, 0x60 ); IOBase = READ_PORT_UCHAR( PnP_Port+1 ) << 8;
		WRITE_PORT_UCHAR( PnP_Port, 0x61 ); IOBase |= READ_PORT_UCHAR( PnP_Port+1 ) & 0xF8;

		if ((IOBase == 0 || IOBase >= 0xFFF8) && (i==0)) continue;
		IOBase &= 0xFFF8;

#if 1
	KdPrint(("BIOS initializes W83627 I/O base = %x\n", IOBase));
	if (IOBase == 0)
	{
		IOBase = 0x290;
		WRITE_PORT_UCHAR( PnP_Port, 0x60 ); WRITE_PORT_UCHAR( PnP_Port+1, UCHAR(IOBase >> 8) );
		WRITE_PORT_UCHAR( PnP_Port, 0x61 ); WRITE_PORT_UCHAR( PnP_Port+1, UCHAR(IOBase & 0xff) );
	}

#endif
		pdx->indexport = (PUCHAR)0 + IOBase + 5;
		pdx->dataport = (PUCHAR)0 + IOBase + 6;

		WRITE_PORT_UCHAR( PnP_Port, 0x30 );				// select device activation register
		if ((READ_PORT_UCHAR( PnP_Port+1) & 1 )==0x00)	// device not activated
		{
			KdPrint(("activate device\n"));
			WRITE_PORT_UCHAR( PnP_Port+1, 1 );			// activate device
		}

		// identify the device
		WRITE_PORT_UCHAR( PnP_Port, 0x20 );
		CRDeviceID = (UCHAR) READ_PORT_UCHAR( PnP_Port+1 );
		pdx->CRDeviceID = CRDeviceID;

		WRITE_PORT_UCHAR( PnP_Port, 0x22 );				// power down
		KdPrint(("W83627 index 0x22 = %x\n", READ_PORT_UCHAR( PnP_Port+1)));

		WRITE_PORT_UCHAR( PnP_Port, 0x26 );				// power down
		KdPrint(("W83627 index 0x26 = %x\n", READ_PORT_UCHAR( PnP_Port+1)));
		WRITE_PORT_UCHAR( PnP_Port, 0x2a );				// power down
		KdPrint(("W83627 index 0x2a = %x\n", READ_PORT_UCHAR( PnP_Port+1)));
	
		WRITE_PORT_UCHAR( PnP_Port, 0xAA );				// exit extended function mode

		// if we have reached this line in the source code -> end loop
		pdx->Pnp_Port = *PnP_Port;
		break;
	}

	return TRUE;
}

static UCHAR WB_READ(PDEVICE_EXTENSION pdx, UCHAR index)
{
	WRITE_PORT_UCHAR( pdx->indexport, index );
	return READ_PORT_UCHAR( pdx->dataport );
}

static VOID WB_WRITE(PDEVICE_EXTENSION pdx, UCHAR index, UCHAR data)
{
	WRITE_PORT_UCHAR( pdx->indexport, index );
	WRITE_PORT_UCHAR( pdx->dataport, data );
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

	WATCHDOG_INFO myWDInfo;
	PUCHAR PnP_Port = &pdx->Pnp_Port;

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
			if (cbin == 0 && cbout == sizeof(*pulSystemBuffer))
			{
				*pulSystemBuffer = pdx->CRDeviceID;
				info = sizeof(*pulSystemBuffer);
				status = STATUS_SUCCESS;
			}
			break;

		case IOCTL_SMBus_ByteDataRead:
			if (CheckAndCopyIn(cbin ,cbout, &pdx->SMBusInfo, SystemBuffer, sizeof(SMB_INFO)))
			{
				if (pdx->SMBusInfo.SlaveAddress<<1 == 0x5c)
				{
					pdx->SMBusInfo.DataByteLow = WB_READ(pdx, (UCHAR)pdx->SMBusInfo.CommandCode);
					RtlCopyMemory( SystemBuffer, &pdx->SMBusInfo, sizeof(SMB_INFO) );
					status = STATUS_SUCCESS;
					info = sizeof(SMB_INFO);
				}
			}
			break;

		case IOCTL_SMBus_ByteDataWrite:
			if (CheckAndCopyIn(cbin ,cbout, &pdx->SMBusInfo, SystemBuffer, sizeof(SMB_INFO)))
			{
				if (pdx->SMBusInfo.SlaveAddress<<1 == 0x5c)
				{
					WB_WRITE(pdx, (UCHAR)pdx->SMBusInfo.CommandCode, pdx->SMBusInfo.DataByteLow);
					status = STATUS_SUCCESS;
					info = sizeof(SMB_INFO);
				}
			}
			break;

		case IOCTL_WATCHDOG_READCONFIG_REG:
			{
				if (CheckAndCopyIn(cbin, cbout, &myWDInfo, SystemBuffer, sizeof(WATCHDOG_INFO)))
				{
					// enter extended function mode 
					WRITE_PORT_UCHAR(PnP_Port, 0x87); 
					WRITE_PORT_UCHAR(PnP_Port, 0x87);

					// select logical device 8=WatchdogDevice
					WRITE_PORT_UCHAR(PnP_Port, 0x07); WRITE_PORT_UCHAR(PnP_Port + 1, 0x08);
					
					// select ConfigRegister
					WRITE_PORT_UCHAR(PnP_Port, myWDInfo.ConfigRegisterAdr);
					
					// read data from config register
					myWDInfo.Data = READ_PORT_UCHAR(PnP_Port + 1);
					
					// exit extended function mode
					WRITE_PORT_UCHAR(PnP_Port, 0xAA);

					// copy retrieved data to SystemBuffer
					RtlCopyMemory(SystemBuffer, &myWDInfo, sizeof(WATCHDOG_INFO));
					status = STATUS_SUCCESS;
					info = sizeof(WATCHDOG_INFO);
				}
			}
			break;

		case IOCTL_WATCHDOG_WRITECONFIG_REG:
			{
				if (CheckAndCopyIn(cbin, cbout, &myWDInfo, SystemBuffer, sizeof(WATCHDOG_INFO)))
				{
					// enter extended function mode 
					WRITE_PORT_UCHAR(PnP_Port, 0x87);
					WRITE_PORT_UCHAR(PnP_Port, 0x87);

					// select logical device 8=WatchdogDevice
					WRITE_PORT_UCHAR(PnP_Port, 0x07); WRITE_PORT_UCHAR(PnP_Port + 1, 0x08);

					// select ConfigRegister
					WRITE_PORT_UCHAR(PnP_Port, myWDInfo.ConfigRegisterAdr);

					// write data to config register
					WRITE_PORT_UCHAR(PnP_Port + 1, myWDInfo.Data);

					// exit extended function mode
					WRITE_PORT_UCHAR(PnP_Port, 0xAA);

					// read data from config register
					myWDInfo.Data = READ_PORT_UCHAR(PnP_Port + 1);

					// copy retrieved data to SystemBuffer
					RtlCopyMemory(SystemBuffer, &myWDInfo, sizeof(WATCHDOG_INFO));
					status = STATUS_SUCCESS;
					info = sizeof(WATCHDOG_INFO);
				}
			}
			break;

		default:
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;

		}						// process request
	}
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	StartNextPacket(&pdx->dqReadWrite, fdo);
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

#pragma PAGEDCODE

NTSTATUS StartDevice(PDEVICE_OBJECT fdo, PCM_PARTIAL_RESOURCE_LIST /*raw*/, PCM_PARTIAL_RESOURCE_LIST translated)
	{							// StartDevice
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	//NTSTATUS status;

	// we dont expect any resources
	if (translated)
	{
		/*PCM_PARTIAL_RESOURCE_DESCRIPTOR resource =*/ translated->PartialDescriptors;
		ULONG nres = translated->Count;
		if (nres)
		{
			return STATUS_UNSUCCESSFUL;
		}
	}

	if (!LocateW83627(pdx)) return STATUS_UNSUCCESSFUL;
	return STATUS_SUCCESS;
	}							// StartDevice

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID StopDevice(IN PDEVICE_OBJECT /*fdo*/, BOOLEAN /*oktouch = FALSE */)
	{							// StopDevice
	//PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	}							// StopDevice
