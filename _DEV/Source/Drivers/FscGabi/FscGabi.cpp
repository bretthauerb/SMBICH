#include "stddcls.h"
#include "driver.h"
#include <intrin.h>
#include <Ntstrsafe.h>
#include <initguid.h>
#include <wdmguid.h>
#include "AcpiGabi.h"

#pragma PAGEDCODE

#pragma pack(push,1)

#define GABI_SIGNATURE "_GABIEP_"

#pragma warning ( disable : 4200 )	// nonstandard extension used

typedef struct tagGABIHeader
{
	UCHAR           ucSignature[8];			// stucture signature
	UCHAR           ucLength;				// structure length
	UCHAR           ucCheckSum;				// checksum of structure data
	UCHAR           ucVersion;				// structure version
	UCHAR           reserved1[5];
	USHORT          usApi16Ofs;				// entry point 16-bit Protected Mode offset
	ULONG           ulApi16Seg;				// entry point 16-bit Protected Mode segment
	ULONG			ulEntryPoint32;			// entry point for 32 bit
	PHYSICAL_ADDRESS	paEntryPoint64;		// entry point for 64 bit
	UCHAR			UselessStuff[0];
} GABI_HEADER, *PGABI_HEADER;

#define MAP_SIZE 0x20000
#define MAP_BIOS_SIZE 0x20000
#define MAP_MASK 0x1ffff

#pragma pack(pop)

extern ULONG ulForceInterpreter;

static UCHAR compute_cs(PUCHAR p, unsigned l)
{
	UCHAR rv = 0;
	unsigned x = l;
	while (l--)
		rv += *p++;
	if (rv) DbgPrint("GABI.sys:" __FILE__ " Length %d, Checksum: %x\n", x, rv);
	return rv;
}

BOOLEAN IsHyperVOn()
{
	// cpuid
	//   EAX=1 CPUID feature bits
	//   ECX bit 31: Running on a hypervisor (always 0 on a real CPU, but also with some hypervisors)

	const UINT32 hypervisorBit = 0x80000000u;

	int cpuInfo[4] = { 0 };  // ret EAX, EBX, ECX, EDX
	__cpuid(cpuInfo, 1);

	return  (((UINT32)cpuInfo[2]) & hypervisorBit) != 0;
}

PVOID MapGabiEntryPoint(_In_ PDEVICE_EXTENSION pdx, _In_ PHYSICAL_ADDRESS PhysicalAddress, _In_ SIZE_T NumberOfBytes)
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

		if (pfnMmMapIoSpaceEx != NULL)
		{
			//OS >= Win 10  => check Code Integrity
#if defined(_AMD64_) || defined(_IA64_)
			if (IsHyperVOn())
			{
				pdx->ulCodeIntegrityCheck = 1;
			}
#endif
		}
	}

	if (pfnMmMapIoSpaceEx != NULL)
	{
		ULONG flags = PAGE_EXECUTE_READ | PAGE_NOCACHE;

		if (pdx->ulCodeIntegrityCheck != 0)
		{
			flags = PAGE_READONLY | PAGE_NOCACHE;
		}

		return pfnMmMapIoSpaceEx(PhysicalAddress, NumberOfBytes, flags);
	}
	else
	{
		return MmMapIoSpace(PhysicalAddress, NumberOfBytes, MmNonCached);
	}
}

typedef struct
{
	PIO_WORKITEM Item;
	PDEVICE_EXTENSION pdx;
	PUNICODE_STRING SymbolicLinkName;	
}
MY_WORK_CONTEXT, *PMY_WORK_CONTEXT;

VOID ArrivalWorker(PVOID IoObject, PVOID Context, PIO_WORKITEM IoWorkItem)
{	
	PMY_WORK_CONTEXT workItem = (PMY_WORK_CONTEXT)Context;

	UNREFERENCED_PARAMETER(IoObject);

	if (workItem)
	{
		if (workItem->pdx->ulUseACPI != 0)
		{
			IoGetDeviceObjectPointer(workItem->SymbolicLinkName,
				FILE_ALL_ACCESS,
				&(workItem->pdx->ACPIFileObject),
				&(workItem->pdx->ACPIDevice));
		}

		ExFreePoolWithTag(workItem, 'cWbD');
	}

	IoFreeWorkItem(IoWorkItem);
}

NTSTATUS ACPIInterfaceNotificationCallback(PVOID NotificationStructure, PVOID Context)
{
	PDEVICE_INTERFACE_CHANGE_NOTIFICATION notify = (PDEVICE_INTERFACE_CHANGE_NOTIFICATION)NotificationStructure;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)Context;

	if (IsEqualGUID(notify->Event, GUID_DEVICE_INTERFACE_ARRIVAL) && notify->SymbolicLinkName != NULL)
	{
		PIO_WORKITEM workItem;
		PMY_WORK_CONTEXT myWorkContext;
		SIZE_T len = sizeof(MY_WORK_CONTEXT) + sizeof(UNICODE_STRING) + (notify->SymbolicLinkName->Length + 2);
		PUCHAR pBase = NULL;
		pdx->ulUseACPI = 1;
		pdx->ulGabiVersion = 1; //fix value because acpi system may not contain any gabi header

		workItem = IoAllocateWorkItem(pdx->DeviceObject);

        pBase = (PUCHAR)ExAllocatePool2(POOL_FLAG_NON_PAGED, len, 'cWbD');
		myWorkContext = (PMY_WORK_CONTEXT)pBase;

		if (!myWorkContext) 
		{
			IoFreeWorkItem(workItem);
			return(STATUS_SUCCESS);
		}

		RtlZeroMemory(myWorkContext, len);
		
		myWorkContext->pdx = pdx;
		myWorkContext->Item = workItem;
		myWorkContext->SymbolicLinkName = (PUNICODE_STRING)(pBase + sizeof(MY_WORK_CONTEXT));
		myWorkContext->SymbolicLinkName->Buffer = (PWCH)(pBase + sizeof(MY_WORK_CONTEXT) + sizeof(UNICODE_STRING));
		myWorkContext->SymbolicLinkName->Length = notify->SymbolicLinkName->Length;
		myWorkContext->SymbolicLinkName->MaximumLength = myWorkContext->SymbolicLinkName->Length + 2;

		RtlUnicodeStringCopy(myWorkContext->SymbolicLinkName, notify->SymbolicLinkName);

		IoQueueWorkItemEx(workItem, ArrivalWorker, DelayedWorkQueue, myWorkContext);
	}

	if (IsEqualGUID(notify->Event, GUID_DEVICE_INTERFACE_REMOVAL))
	{
		pdx->ulUseACPI = 0;

		if (pdx->ACPIFileObject != NULL)
		{
			ObDereferenceObject(pdx->ACPIFileObject);
		}

		pdx->ACPIFileObject = NULL;
		pdx->ACPIDevice = NULL;
	}

	return(STATUS_SUCCESS);
}

NTSTATUS RegisterACPIInterfaceNotification(PDEVICE_EXTENSION pdx)
{
	NTSTATUS status = IoRegisterPlugPlayNotification(
							EventCategoryDeviceInterfaceChange,
							PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
							(LPGUID)&GUID_DEVINTERFACE_GabiAcpi,
							pdx->DriverObject,
							ACPIInterfaceNotificationCallback,
							pdx,
							&pdx->pvNotificationEntry);

	return status;
}

NTSTATUS UnregisterACPIInterfaceNotification(PDEVICE_EXTENSION pdx)
{
	NTSTATUS status = STATUS_SUCCESS;

	if (pdx != NULL && pdx->pvNotificationEntry != NULL)
	{
		status = IoUnregisterPlugPlayNotificationEx(pdx->pvNotificationEntry);
		pdx->pvNotificationEntry = NULL;
	}
	return status;
}

NTSTATUS StartDevice(PDEVICE_OBJECT fdo, PCM_PARTIAL_RESOURCE_LIST raw, PCM_PARTIAL_RESOURCE_LIST translated)
{							// StartDevice
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	UNREFERENCED_PARAMETER(translated);
	UNREFERENCED_PARAMETER(raw);	

	// locate GABI header
	PUCHAR MappedBios;
	PGABI_HEADER pHeader = NULL;
	PHYSICAL_ADDRESS pa;
	pa.HighPart = 0;
	pa.LowPart = 0xe0000;

	pdx->ulGabiVersion = 0;

	RegisterACPIInterfaceNotification(pdx);

	// Map BIOS 0xe0000-0xfffff, size and addresses are fixed.
	if ((MappedBios = (PUCHAR)MapGabiEntryPoint(pdx, pa, MAP_SIZE)) == 0)
		return STATUS_INSUFFICIENT_RESOURCES;

	// Search for valid GABI  header
	PUCHAR p, MappedBiosEnd = MappedBios + MAP_SIZE;
	for (p = MappedBios; (p + sizeof(*pHeader)) < MappedBiosEnd; p += 16)
	{
		// header found ?
		if ((memcmp(((PGABI_HEADER)p)->ucSignature, GABI_SIGNATURE, sizeof(pHeader->ucSignature)) == 0) &&
		// not too long ?
			((p + ((PGABI_HEADER)p)->ucLength) < MappedBiosEnd) &&
		// checksum ok ?
			(compute_cs(p, ((PGABI_HEADER)p)->ucLength) == 0) )
		{
				pHeader = (PGABI_HEADER)p;
				break;
		}
	}
	// Does it contain a valid entry point ?
	if (pHeader &&
#if defined(_AMD64_) || defined(_IA64_)
		pHeader->ucLength >= (FIELD_OFFSET(GABI_HEADER,paEntryPoint64) + sizeof(pHeader->paEntryPoint64)) &&
		pHeader->paEntryPoint64.LowPart != 0 &&
#else
		pHeader->ucLength >= (FIELD_OFFSET(GABI_HEADER,ulEntryPoint32) + sizeof(pHeader->ulEntryPoint32)) &&
		pHeader->ulEntryPoint32 != 0 &&
#endif
		pHeader->ucVersion == 1)
	{
		ULONG ulEP_Offset;
		UCHAR ucAddressingMode;

#if defined(_AMD64_) || defined(_IA64_)
		ucAddressingMode = INTERPRETER_64BIT;
		pa.HighPart = pHeader->paEntryPoint64.HighPart;
		pa.LowPart = pHeader->paEntryPoint64.LowPart & ~MAP_MASK;
		ulEP_Offset = pHeader->paEntryPoint64.LowPart & MAP_MASK;
#else
		ucAddressingMode = INTERPRETER_32BIT;
		pa.HighPart = 0;
		pa.LowPart = pHeader->ulEntryPoint32 & ~MAP_MASK;
		ulEP_Offset = pHeader->ulEntryPoint32 & MAP_MASK;
#endif

		pdx->ulGabiVersion = pHeader->ucVersion;

		MmUnmapIoSpace(MappedBios, MAP_SIZE);

		// Map block around GABI entry
		if ((MappedBios = (PUCHAR)MapGabiEntryPoint(pdx, pa, MAP_BIOS_SIZE)) != 0)
		{
			pdx->MappedBios = MappedBios;
			pdx->GabiCallAddress = (void (__stdcall *)(PHYSICAL_ADDRESS,PHYSICAL_ADDRESS,PHYSICAL_ADDRESS))(MappedBios + ulEP_Offset);

#if defined(_AMD64_) || defined(_IA64_)
			if (pdx->ulCodeIntegrityCheck != 0 || ulForceInterpreter != 0)
			{
				DbgPrint("---- Interpreter is active ----");
				pdx->pInterpreterContext = InitInterpreter(MappedBios + ulEP_Offset, (USHORT)(MAP_BIOS_SIZE - ulEP_Offset), ucAddressingMode);
			}
#endif
		}
	}
	else
	{
		MmUnmapIoSpace(MappedBios, MAP_SIZE);
		pdx->MappedBios = MappedBios = NULL;		
	}

	KeInitializeMutex( &(pdx->Mutex), 1 );

	return STATUS_SUCCESS;
}							// StartDevice

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID StopDevice(IN PDEVICE_OBJECT fdo, BOOLEAN oktouch /* = FALSE */)
{							// StopDevice
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	UNREFERENCED_PARAMETER(oktouch);

	UnregisterACPIInterfaceNotification(pdx);

	if (pdx != NULL)
	{
		CleanupInterpreter(pdx->pInterpreterContext);
		pdx->pInterpreterContext = NULL;

		if (pdx->ControlBuffer.pVirtual)
		{
			MmFreeContiguousMemory(pdx->ControlBuffer.pVirtual);
			pdx->ControlBuffer.pVirtual = NULL;
		}
		if (pdx->InBuffer.pVirtual)
		{
			MmFreeContiguousMemory(pdx->InBuffer.pVirtual);
			pdx->InBuffer.pVirtual = NULL;
		}
		if (pdx->OutBuffer.pVirtual)
		{
			MmFreeContiguousMemory(pdx->OutBuffer.pVirtual);
			pdx->OutBuffer.pVirtual = NULL;
		}
		if (pdx->MappedBios)
		{
			MmUnmapIoSpace(pdx->MappedBios, MAP_BIOS_SIZE);
			pdx->MappedBios = NULL;
		}
		if (pdx->SyncEvent)
		{
			ZwClose(pdx->SyncEvent);
			pdx->SyncEvent = NULL;
		}
	}
}							// StopDevice
