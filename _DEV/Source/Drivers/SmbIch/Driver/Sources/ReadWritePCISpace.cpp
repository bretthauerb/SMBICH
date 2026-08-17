#include "stddcls.h"
#include "Driver.h"

#pragma LOCKEDCODE

static NTSTATUS ReadWritePCISpace_OnComplete(IN PDEVICE_OBJECT /*pdo*/, IN PIRP /*pIrp*/, IN PVOID Context)
{
	KEVENT *event = (KEVENT*) Context;
	KeSetEvent( event, 0, FALSE );
	return STATUS_MORE_PROCESSING_REQUIRED;
}

//
// General replacement for Hal[GS]etBusDataByOffset under Windows 2000/XP.
//
NTSTATUS ReadWritePCISpace(IN PDEVICE_OBJECT pdo, IN BOOLEAN Write, IN OUT PVOID Buffer, IN ULONG Offset, IN ULONG Length )
{
#ifndef NT4
	if (!pdo)
#endif
		return STATUS_INVALID_DEVICE_REQUEST;
#ifndef NT4
	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

	KEVENT		event;
	NTSTATUS	status;
	PIRP		pIrp = IoAllocateIrp( pdo->StackSize, FALSE );
	if (!pIrp) return STATUS_INSUFFICIENT_RESOURCES;

	PIO_STACK_LOCATION	stack = IoGetNextIrpStackLocation( pIrp );

	stack->DeviceObject = pdo;
	stack->MajorFunction = IRP_MJ_PNP;
	stack->MinorFunction = (Write) ? IRP_MN_WRITE_CONFIG : IRP_MN_READ_CONFIG;
	pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	stack->Parameters.ReadWriteConfig.WhichSpace = PCI_WHICHSPACE_CONFIG;
	stack->Parameters.ReadWriteConfig.Buffer = Buffer;
	stack->Parameters.ReadWriteConfig.Offset = Offset;
	stack->Parameters.ReadWriteConfig.Length = Length;

	KeInitializeEvent(&event, NotificationEvent, FALSE);
	IoSetCompletionRoutine( pIrp, ReadWritePCISpace_OnComplete, &event, TRUE, TRUE, TRUE);
	

	IoCallDriver( pdo, pIrp);
	KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

	status = pIrp->IoStatus.Status;

	IoFreeIrp( pIrp );

	return status;
#endif
}
