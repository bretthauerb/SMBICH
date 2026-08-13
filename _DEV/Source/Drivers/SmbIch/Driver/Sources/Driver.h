// Declarations for sources driver
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#ifndef DRIVER_H
#define DRIVER_H

#define MAJOR_VERSION	7
#define MINOR_VERSION	0
#define RELEASE		    0
#define DRIVER_VERSION ((MAJOR_VERSION<<24) + (MINOR_VERSION<<16) + RELEASE)


#if defined(_NTDDK_) || defined(_WDMDDK_)

#define SMBUS_DRIVER_ID SMBUS_HWID_SmbIch

// Disable interrup usage
// Not all BIOSes route the interrupt correctly
#define NO_INTERRUPT 0


#include "SmBusIoctl.h"
#include "devqueue.h"

///////////////////////////////////////////////////////////////////////////////
// Device extension structure

enum DEVSTATE {
	STOPPED,								// device stopped
	WORKING,								// started and working
	PENDINGSTOP,							// stop pending
	PENDINGREMOVE,							// remove pending
	SURPRISEREMOVED,						// removed by surprise
	REMOVED,								// removed
	};

typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT DeviceObject;			// device object this extension belongs to
	PDEVICE_OBJECT LowerDeviceObject;		// next lower driver in same stack
	PDEVICE_OBJECT Pdo;						// the PDO
	IO_REMOVE_LOCK RemoveLock;				// removal control locking structure
	UNICODE_STRING devname;
	UNICODE_STRING uniSymbolicLinkName;
	DEVSTATE state;							// current state of device
	DEVSTATE prevstate;						// state prior to removal query
	DEVICE_POWER_STATE devpower;			// current device power state
	SYSTEM_POWER_STATE syspower;			// current system power state
	DEVICE_CAPABILITIES devcaps;			// copy of most recent device capabilities
	DEVQUEUE dqReadWrite;					// queue for reads and writes
	LONG handles;							// # open handles
	PKINTERRUPT InterruptObject;			// address of interrupt object
	PUCHAR portbase;						// I/O port base address
	ULONG nports;							// number of assigned ports

	// Additional per-device declarations
	struct block_tranfer {
		enum { NONE, READ, WRITE }	mode;
		PUCHAR					address;
		int						count;
	} block_tranfer;
	BOOLEAN StalledForPower;
	BOOLEAN	bPIIX4;							// has HW semaphore
	BOOLEAN UseInterrupt;
	KTIMER	Timer;
	KDPC	PollDpc;
	UCHAR	StartCommand;
	int		TimeOutCounter;
	ULONG	RetryCount;
//	KSPIN_LOCK TimeoutLock;

	SMB_INFO			SMBusInfo;

//	BOOLEAN mappedport;						// true if we mapped port addr in StartDevice
//	BOOLEAN busy;							// true if device busy with a request
	PUCHAR	pucDMI;
	ULONG	ulDMISize;
	ULONG	ulDMIStructCount;
	} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

///////////////////////////////////////////////////////////////////////////////
// Global functions
VOID StartIo(IN PDEVICE_OBJECT fdo, PIRP Irp);
VOID RemoveDevice(IN PDEVICE_OBJECT fdo);
NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status, IN ULONG_PTR info);
NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status);
NTSTATUS ForwardAndWait(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
VOID SendAsyncNotification(PVOID context);
VOID EnableAllInterfaces(PDEVICE_EXTENSION pdx, BOOLEAN enable);
VOID DeregisterAllInterfaces(PDEVICE_EXTENSION pdx);
NTSTATUS StartDevice(PDEVICE_OBJECT fdo, PCM_PARTIAL_RESOURCE_LIST raw, PCM_PARTIAL_RESOURCE_LIST translated);
VOID StopDevice(PDEVICE_OBJECT fdo, BOOLEAN oktouch = FALSE);
IO_DPC_ROUTINE DpcForIsr;
KDEFERRED_ROUTINE DpcForPoll;
IO_TIMER_ROUTINE IoTimer;
VOID DisableInterrupt( PDEVICE_EXTENSION );
BOOLEAN OnInterrupt(PKINTERRUPT, PDEVICE_EXTENSION);
VOID ICH_Initialize(PDEVICE_EXTENSION pdx);
VOID UnstallQueue(PDEVICE_OBJECT fdo);
BOOLEAN StallQueueAndNotify(PDEVICE_OBJECT, PVOID, VOID (*)(PVOID));
BOOLEAN CheckQueueStalled( PDEVICE_OBJECT fdo );
NTSTATUS SendDeviceSetPower(PDEVICE_EXTENSION fdo, DEVICE_POWER_STATE state, BOOLEAN wait = FALSE);
// I/O request handlers

_Dispatch_type_(IRP_MJ_CREATE) DRIVER_DISPATCH DispatchCreate;
_Dispatch_type_(IRP_MJ_CLOSE) DRIVER_DISPATCH DispatchClose;
_Dispatch_type_(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH DispatchControl;
_Dispatch_type_(IRP_MJ_CLEANUP) DRIVER_DISPATCH DispatchCleanup;
_Dispatch_type_(IRP_MJ_POWER) DRIVER_DISPATCH DispatchPower;
_Dispatch_type_(IRP_MJ_PNP) DRIVER_DISPATCH DispatchPnp;
_Dispatch_type_(IRP_MJ_SYSTEM_CONTROL) DRIVER_DISPATCH DispatchSystemControl;

extern UNICODE_STRING servkey;

#endif // defined(_NTDDK_) || defined(_WDMDDK_)

#endif // DRIVER_H
