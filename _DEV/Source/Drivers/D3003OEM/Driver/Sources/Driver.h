// Declarations for sources driver
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#ifndef DRIVER_H
#define DRIVER_H

#define MAJOR_VERSION	2
#define MINOR_VERSION	1
#define RELEASE			2
#define DRIVER_VERSION ((MAJOR_VERSION<<24) + (MINOR_VERSION<<16) + RELEASE)

#ifdef _NTDDK_

// Disable interrupt usage
// The BIOS does not route the interrupt correctly
#define NO_INTERRUPT 1

#define DRIVER_NAME "D3003OEM"

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
	PUCHAR AcpiMMioAddr;					// I/O port base address
	BOOLEAN bIoMapped;						// above address is io mapped

	// interrupt stuff
	ULONG vector;
	BOOLEAN UseInterrupt;
	PKINTERRUPT InterruptObject;			// address of interrupt object
	UCHAR StartCommand;

	// Additional per-device declarations
	volatile PUCHAR watchdogcontrol;
	volatile PULONG watchdogcount;
	ULONG	LastCountWritten;

	BOOLEAN StalledForPower;
	KTIMER	Timer;
	KDPC	PollDpc;
	unsigned TimeOutCounter;

	SMB_INFO			SMBusInfo;

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
VOID DpcForIsr(_In_ PKDPC Dpc, _In_ struct _DEVICE_OBJECT *fdo, _Inout_ struct _IRP *junk, _In_opt_ PVOID pContext);
VOID DpcForPoll(PKDPC Dpc, PDEVICE_OBJECT fdo, PVOID, PVOID);
VOID DisableInterrupt( PDEVICE_EXTENSION );
BOOLEAN OnInterrupt(PKINTERRUPT, PDEVICE_EXTENSION);
VOID UnstallQueue(PDEVICE_OBJECT fdo);
BOOLEAN StallQueueAndNotify(PDEVICE_OBJECT, PVOID, VOID (*)(PVOID));
BOOLEAN CheckQueueStalled( PDEVICE_OBJECT fdo );
NTSTATUS SendDeviceSetPower(PDEVICE_EXTENSION fdo, DEVICE_POWER_STATE state, BOOLEAN wait = FALSE);
// I/O request handlers

NTSTATUS DispatchCreate(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchClose(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchControl(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchCleanup(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchPower(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchPnp(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchSystemControl(PDEVICE_OBJECT fdo, PIRP Irp);

extern UNICODE_STRING servkey;

#endif // _NTDDK_

#endif // DRIVER_H
