#include "stddcls.h"
#include "driver.h" 
#include "DMI.h"

#pragma PAGEDCODE

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
		return pfnMmMapIoSpaceEx(PhysicalAddress, NumberOfBytes, PAGE_READONLY | PAGE_NOCACHE);
	}
	else
	{
		return MmMapIoSpace(PhysicalAddress, NumberOfBytes, MmNonCached);
	}
}

NTSTATUS StartDevice(PDEVICE_OBJECT fdo, PCM_PARTIAL_RESOURCE_LIST /*raw*/, PCM_PARTIAL_RESOURCE_LIST /*translated*/)
{							// StartDevice
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	// NTSTATUS status;
	PHYSICAL_ADDRESS PhAddressESegment;

	// Map E and F Segment, remains mapped untill unload
	PhAddressESegment.LowPart = 0xe0000;
	PhAddressESegment.HighPart = 0;
	pdx->pMappedESegment = (PUCHAR)MapEntryPoint(PhAddressESegment, 0x20000);
	if (!pdx->pMappedESegment)
		return STATUS_INSUFFICIENT_RESOURCES;

	MapDMI(pdx);

	return STATUS_SUCCESS;
}							// StartDevice

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID StopDevice(IN PDEVICE_OBJECT fdo, BOOLEAN /*oktouch*/ /* = FALSE */)
{							// StopDevice
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	UnmapDMI(pdx);
	// Should have been mapped, but you never know
	if (pdx->pMappedESegment)
	{
		MmUnmapIoSpace(pdx->pMappedESegment, 0x20000);
		pdx->pMappedESegment = NULL;
	}

}							// StopDevice
