// Main program for sources driver
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"
#include <ntstrsafe.h>

static DRIVER_ADD_DEVICE AddDevice;
static DRIVER_UNLOAD DriverUnload;
static NTSTATUS OnRequestComplete(IN PDEVICE_OBJECT fdo, IN PIRP Irp, IN PKEVENT pev);

extern "C" DRIVER_INITIALIZE DriverEntry;

#if 0
UNICODE_STRING servkey;
#endif

///////////////////////////////////////////////////////////////////////////////


#pragma INITCODE

_Use_decl_annotations_
extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath)
{
	RegistryPath;  // prevent compiler warning "'RegistryPath': unreferenced formal parameter" which is treated as error.

	// Insist that OS support at least the WDM level of the DDK we use
	if (!RtlIsNtDdiVersionAvailable(NTDDI_WIN2K))
	{
		KdPrint((SMBUS_DRIVER_NAME " - Expected version of WDM (%d.%2.2d) not available\n", 1, 0));
		return STATUS_UNSUCCESSFUL;
	}

	ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

#if 0
	// Save the name of the service key
	servkey.Buffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, RegistryPath->Length + sizeof(WCHAR), 'SMBS');
	if (!servkey.Buffer)
	{
		KdPrint((SMBUS_DRIVER_NAME " - Unable to allocate %d bytes for copy of service key name\n", RegistryPath->Length + sizeof(WCHAR)));
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	servkey.MaximumLength = RegistryPath->Length + sizeof(WCHAR);
	RtlCopyUnicodeString(&servkey, RegistryPath);
#endif

	// Initialize function pointers

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->DriverExtension->AddDevice = AddDevice;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchControl;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = DispatchCleanup;
	DriverObject->MajorFunction[IRP_MJ_POWER] = DispatchPower;
	DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
	DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = DispatchSystemControl;

	return STATUS_SUCCESS;
}							// DriverEntry

_Use_decl_annotations_
extern NTSTATUS DispatchSystemControl(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{							// DispatchSystemControl
	IoSkipCurrentIrpStackLocation(Irp);
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	return IoCallDriver(pdx->LowerDeviceObject, Irp);
}							// DispatchSystemControl

///////////////////////////////////////////////////////////////////////////////
#pragma PAGEDCODE

_Use_decl_annotations_
static VOID DriverUnload(PDRIVER_OBJECT /*DriverObject*/)
{							// WdmDriverUnload
	PAGED_CODE();
#if 0
	RtlFreeUnicodeString(&servkey);
#endif
}							// WdmDriverUnload

#pragma PAGEDCODE

_Use_decl_annotations_
static NTSTATUS AddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT pdo)
{							// AddDevice
	PAGED_CODE();

	NTSTATUS status;

	// Create a functional device object to represent the hardware we're managing.

	PDEVICE_OBJECT fdo;
#define xsize sizeof(DEVICE_EXTENSION)

	UNICODE_STRING devname;
	WCHAR namebuf[32];
	status = RtlStringCchPrintfW(namebuf, ARRAYSIZE(namebuf), L"\\Device\\" SMBUS_DRIVER_NAME_L);
	if (!NT_SUCCESS(status))
	{
		KdPrint((SMBUS_DRIVER_NAME " - Unable to format device name - %X\n", status));
		return status;
	}
	RtlInitUnicodeString(&devname, namebuf);

	status = IoCreateDevice(DriverObject, xsize, &devname,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		FALSE, &fdo);
	if (!NT_SUCCESS(status))
	{						// can't create device object
		KdPrint((SMBUS_DRIVER_NAME " - IoCreateDevice failed - %X\n", status));
		return status;
	}						// can't create device object

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;

	KeInitializeTimer(&pdx->Timer);
	KeInitializeDpc(&pdx->PollDpc, DpcForPoll, fdo);

	// From this point forward, any error will have side effects that need to
	// be cleaned up. Using a try-finally block allows us to modify the program
	// easily without losing track of the side effects.

	__try
	{						// finish initialization
		RtlInitUnicodeString(&(pdx->uniSymbolicLinkName), L"\\DosDevices\\" SMBUS_DRIVER_NAME_L);

		status = IoCreateSymbolicLink(&(pdx->uniSymbolicLinkName), &devname);
		if (!NT_SUCCESS(status)) {
			KdPrint((SMBUS_DRIVER_NAME " - Unable to create symbolic link\n"));
			pdx->uniSymbolicLinkName.Buffer = NULL;
			__leave;
		}

		pdx->DeviceObject = fdo;
		pdx->Pdo = pdo;

		IoInitializeRemoveLock(&pdx->RemoveLock, 0, 0, 0);
		pdx->state = STOPPED;		// device starts in the stopped state

		InitializeQueue(&pdx->dqReadWrite, StartIo);

		// Make a copy of the device name

		pdx->devname.Buffer = static_cast<PWCHAR>(
			ExAllocatePoolZero(NonPagedPoolNx, devname.MaximumLength, 'SMBN'));
		if (!pdx->devname.Buffer)
		{					// can't allocate buffer
			status = STATUS_INSUFFICIENT_RESOURCES;
			KdPrint((SMBUS_DRIVER_NAME " - Unable to allocate %d bytes for copy of name\n", devname.MaximumLength));
			__leave;
		}					// can't allocate buffer
		pdx->devname.MaximumLength = devname.MaximumLength;
		RtlCopyUnicodeString(&pdx->devname, &devname);

		// Initialize DPC object

		IoInitializeDpcRequest(fdo, DpcForIsr);
#pragma warning(suppress:28133)
		IoInitializeTimer(fdo, IoTimer, NULL);

		// Link our device object into the stack leading to the PDO
		if (pdo)
		{
			pdx->LowerDeviceObject = IoAttachDeviceToDeviceStack(fdo, pdo);
			if (!pdx->LowerDeviceObject)
			{						// can't attach device
				KdPrint((SMBUS_DRIVER_NAME " - IoAttachDeviceToDeviceStack failed\n"));
				status = STATUS_DEVICE_REMOVED;
				__leave;
			}						// can't attach device
		// Set power management flags in the device object

			fdo->Flags |= DO_POWER_PAGABLE;

			// Indicate that our initial power state is D0 (fully on). Also indicate that
			// we have a pagable power handler (otherwise, we'll never get idle shutdown
			// messages!)

			pdx->syspower = PowerSystemWorking;
			pdx->devpower = PowerDeviceD0;
			POWER_STATE state;
			state.DeviceState = PowerDeviceD0;
			PoSetPowerState(fdo, DevicePowerState, state);

			// Clear the "initializing" flag so that we can get IRPs
			fdo->Flags &= ~DO_DEVICE_INITIALIZING;
		}
	}

	__finally
	{						// cleanup side effects
		if (!NT_SUCCESS(status))
		{					// need to cleanup
			if (pdx->uniSymbolicLinkName.Buffer)
				IoDeleteSymbolicLink(&(pdx->uniSymbolicLinkName));
			if (pdx->devname.Buffer)
			{
				ExFreePoolWithTag(pdx->devname.Buffer, 'SMBN');
				pdx->devname.Buffer = NULL;
				pdx->devname.Length = 0;
				pdx->devname.MaximumLength = 0;
			}
			if (pdx->LowerDeviceObject)
				IoDetachDevice(pdx->LowerDeviceObject);
			IoDeleteDevice(fdo);
		}					// need to cleanup
		else
			KdPrint(("AddDevice SUCCESS\n"));
	}
	// cleanup side effects

	return status;
}							// AddDevice

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status, IN ULONG_PTR info)
{							// CompleteRequest
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}							// CompleteRequest

NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status)
{							// CompleteRequest
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}							// CompleteRequest

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS ForwardAndWait(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{							// ForwardAndWait
	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
	PAGED_CODE();

	KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);

	IoCopyCurrentIrpStackLocationToNext(Irp);
	IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE)OnRequestComplete,
		(PVOID)&event, TRUE, TRUE, TRUE);

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	IoCallDriver(pdx->LowerDeviceObject, Irp);
	KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
	return Irp->IoStatus.Status;
}							// ForwardAndWait

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

static NTSTATUS OnRequestComplete(IN PDEVICE_OBJECT /*fdo*/, IN PIRP /*Irp*/, IN PKEVENT pev)
{							// OnRequestComplete
	KeSetEvent(pev, 0, FALSE);
	return STATUS_MORE_PROCESSING_REQUIRED;
}							// OnRequestComplete

///////////////////////////////////////////////////////////////////////////////

VOID EnableAllInterfaces(PDEVICE_EXTENSION /*pdx*/, BOOLEAN /*enable*/)
{							// EnableAllInterfaces
}							// EnableAllInterfaces

///////////////////////////////////////////////////////////////////////////////

VOID DeregisterAllInterfaces(PDEVICE_EXTENSION /*pdx*/)
{							// DeregisterAllInterfaces
}							// DeregisterAllInterfaces

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID RemoveDevice(IN PDEVICE_OBJECT fdo)
{							// RemoveDevice
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	if (pdx->uniSymbolicLinkName.Buffer)
		IoDeleteSymbolicLink(&(pdx->uniSymbolicLinkName));

	if (pdx->devname.Buffer)
	{
		ExFreePoolWithTag(pdx->devname.Buffer, 'SMBN');
		pdx->devname.Buffer = NULL;
		pdx->devname.Length = 0;
		pdx->devname.MaximumLength = 0;
	}

	if (pdx->LowerDeviceObject)
	{
		IoDetachDevice(pdx->LowerDeviceObject);
		pdx->LowerDeviceObject = NULL;
	}

	if (fdo)
		IoDeleteDevice(fdo);
}							// RemoveDevice

///////////////////////////////////////////////////////////////////////////////

#if DBG && defined(_X86_)
#pragma LOCKEDCODE

extern "C" void __declspec(naked) __cdecl _chkesp()
{
	_asm je okay
	ASSERT(!SMBUS_DRIVER_NAME " - Stack pointer mismatch!");
okay:
	_asm ret
}

#endif // DBG
