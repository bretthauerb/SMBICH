// Control.cpp -- IOCTL handlers for sources driver
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"

#pragma LOCKEDCODE

// Cancel routine for Irps managed by StartIo
static VOID OnCancelIrp(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{							// OnCancelReadWrite
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	CancelRequest(&pdx->RemoveLock, &pdx->dqReadWrite, Irp);
}							// OnCancelReadWrite

#pragma PAGEDCODE

_Use_decl_annotations_
NTSTATUS DispatchControl(PDEVICE_OBJECT fdo, PIRP Irp)
{
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	NTSTATUS status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);

	PAGED_CODE();

	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status, 0);

	IoMarkIrpPending(Irp);
	StartPacket(&pdx->dqReadWrite, fdo, Irp, OnCancelIrp);
	return STATUS_PENDING;
}

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

_Use_decl_annotations_
NTSTATUS DispatchCleanup(PDEVICE_OBJECT fdo, PIRP Irp)
{							// DispatchCleanup
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	stack;
	pdx;

	return CompleteRequest(Irp, STATUS_SUCCESS, 0);
}							// DispatchCleanup

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

_Use_decl_annotations_
NTSTATUS DispatchCreate(PDEVICE_OBJECT fdo, PIRP Irp)
{							// DispatchCreate
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	// Claim the remove lock in Win2K so that removal waits until the
	// handle closes. Don't do this in Win98, however, because this
	// device might be removed by surprise with handles open, whereupon
	// we'll deadlock in HandleRemoveDevice waiting for a close that
	// can never happen because we can't run the user-mode code that
	// would do the close.

	NTSTATUS status;
	status = IoAcquireRemoveLock(&pdx->RemoveLock, stack->FileObject);

	if (NT_SUCCESS(status))
	{						// okay to open
		if (InterlockedIncrement(&pdx->handles) == 1)
		{					// first open handle
		}					// okay to open
	}					// first open handle
	return CompleteRequest(Irp, status, 0);
}							// DispatchCreate

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

_Use_decl_annotations_
NTSTATUS DispatchClose(PDEVICE_OBJECT fdo, PIRP Irp)
{							// DispatchClose
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	if (InterlockedDecrement(&pdx->handles) == 0)
	{						// no more open handles
	}						// no more open handles

// Release the remove lock to match the acquisition done in DispatchCreate
	IoReleaseRemoveLock(&pdx->RemoveLock, stack->FileObject);

	return CompleteRequest(Irp, STATUS_SUCCESS, 0);
}							// DispatchClose
