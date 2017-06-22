#ifdef __cplusplus
	extern "C" {
#endif

#pragma warning ( disable : 4201 )
#pragma warning ( disable : 4514 )

#include <stdio.h>
#include <stdarg.h>			// for variable argument list in DebugPrint function
#include <ntddk.h>
#include "FSCIoctl.h"
#include "AcpiGabi.h"

#pragma warning ( default : 4201 )

#ifdef __cplusplus
	}
#endif


#include "FscGabi.h"
#include "FscGabiUEFI.h"
#include "stddcls.h"
#include "driver.h"
#include "asm_prototypes.h"

#ifndef min
#define min(x, y) (((x) > (y)) ? (y) : (x))
#endif
#ifndef max
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif

#pragma LOCKEDCODE 

//static void x(char * s, PUCHAR p, int i)
//{
//
//	DbgPrint("--- %s ----\n", s);
//
//	for (int indx = 0; indx < i; indx++)
//	{
//		DbgPrint(" %02x", p[indx]);
//		if (indx % 8)
//		{
//			DbgPrint("   ");
//
//		}
//		if (indx % 16)
//		{
//			DbgPrint("\n");
//
//		}
//	}
//	DbgPrint("--- END of %s ----\n", s);
//
//	/*DbgPrint("%s%02x %02x %02x %02x %02x %02x %02x %02x\n", s,p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
//	if (i>8)
//	DbgPrint("        %02x %02x %02x %02x %02x %02x %02x %02x\n", p[8],p[9],p[10],p[11],p[12],p[13],p[14],p[15]);
//	if (i>16)
//	DbgPrint("        %02x %02x %02x %02x %02x %02x %02x %02x\n", p[16],p[17],p[18],p[19],p[20],p[21],p[22],p[23]);
//	if (i>24)
//	DbgPrint("        %02x %02x %02x %02x %02x %02x %02x %02x\n", p[24],p[25],p[26],p[27],p[28],p[29],p[30],p[31]);*/
//}

static const PHYSICAL_ADDRESS Phys4GB = {/*LowPart*/~0UL, /*HighPart*/0};
static const PHYSICAL_ADDRESS ZeroAddr = {/*LowPart*/0UL, /*HighPart*/0 };

static PVOID AllocateContiguousMemory(SIZE_T NumberOfBytes, PHYSICAL_ADDRESS HighestAcceptableAddress)
{
	typedef PVOID(*PFN_MmAllocateContiguousNodeMemory)(_In_ SIZE_T NumberOfBytes, _In_ PHYSICAL_ADDRESS LowestAcceptableAddress, _In_ PHYSICAL_ADDRESS HighestAcceptableAddress, _In_opt_ PHYSICAL_ADDRESS BoundaryAddressMultiple, _In_ ULONG Protect, _In_ NODE_REQUIREMENT PreferredNode);

	static PFN_MmAllocateContiguousNodeMemory pfnMmAllocateContiguousNodeMemory = NULL;
	static BOOLEAN bAlreadyGot = FALSE;

	if (!bAlreadyGot)
	{
		UNICODE_STRING uniFuncName;
		RtlInitUnicodeString(&uniFuncName, L"MmAllocateContiguousNodeMemory");

#pragma warning(push)
#pragma warning(disable : 4055)
		pfnMmAllocateContiguousNodeMemory = (PFN_MmAllocateContiguousNodeMemory)MmGetSystemRoutineAddress(&uniFuncName);
#pragma warning(pop)
		bAlreadyGot = TRUE;
	}

	if (pfnMmAllocateContiguousNodeMemory != NULL)
	{
		return pfnMmAllocateContiguousNodeMemory(NumberOfBytes, ZeroAddr, HighestAcceptableAddress, ZeroAddr, PAGE_READWRITE, MM_ANY_NODE_OK);
	}
	else
	{
		return MmAllocateContiguousMemory(NumberOfBytes, HighestAcceptableAddress);
	}
}

static BOOLEAN AllocateContiguousBuffer(DriverBufferDescriptor_T * pB, ULONG Size)
{
	// not yet allocated or not same size ?
	if (pB->pVirtual == NULL || pB->ulSize != Size)
	{
		if (pB->pVirtual)
			MmFreeContiguousMemory(pB->pVirtual);
		ULONG new_size = max(1024, Size);
		pB->pVirtual = (PUCHAR)AllocateContiguousMemory(new_size, Phys4GB);
		if (pB->pVirtual == NULL)
			return FALSE;
		pB->ulSize = (USHORT)new_size;
	}
	// Maybe it is necessary to lock the memory.
	// Untill that has been fixed we get the physical address every time.
	if (pB->pVirtual) pB->Physical = MmGetPhysicalAddress(pB->pVirtual);

	/* The design is brain-damaged and specifies that all buffers must be cleared */
	RtlZeroMemory(pB->pVirtual, pB->ulSize);
	return TRUE;
}

// Does this request use a descriptor list in the out-buffer ?
typedef	enum { FLASH_UPDATE, FLASH_ARCHIVE, READ_SYS_DATA, WRITE_SYS_DATA,
			   OTHER_UEFI,
			   OTHERS } Command_T;
static Command_T Command(GabiGenericAPIHeader_T *r)
{
	if (r->ServiceCategory == 3 || // Japan FLASH
		r->ServiceCategory == 8)	// UEFI NVRAM data 
		switch (r->Service)
		{
			case 3:	return FLASH_UPDATE;
			case 4: return FLASH_ARCHIVE;
			default: return OTHERS;
		}
	if (r->ServiceCategory == 0x8000 || r->ServiceCategory == 0x5)	// Japan system data
		switch (r->Service)
		{
			case 4:	return READ_SYS_DATA;
			case 5: return WRITE_SYS_DATA;
			default: return OTHERS;
		}
	if ((r->ServiceCategory == 7) ||  // UEFI NVRAM data
	   ((r->ServiceCategory & 0xfe00) == 0x8200))	// Assume these are all UEFI services
			return OTHER_UEFI;		// All UEFI commands use buffer descriptors when more then 0x10 data.

	return OTHERS;
}

static VOID FreeBuffers(DriverBufferDescriptor_T *p)
{
	DriverBufferDescriptor_T *temp = p;
	if (p)
	{
		while (p->ulSize)
		{
			MmFreeContiguousMemory(p->pVirtual);
			p++;
		}
		ExFreePool(temp);
	}
}
//
// Allocate memory for indirect GABI buffers.
// Puts size and physical addresses of the segments in the GABI in- or outbuffer.
// It also allocates an array DriverBufferDescriptor_T[] that holds size and phys. and virtual addresses.
//
// Returns NULL when out of resources
//
static DriverBufferDescriptor_T * BuildDescriptorList(DEVICE_EXTENSION *pDevExt, PUCHAR pInOutBuffer, ULONG ulSize, ULONG StupidSize)
{
	const ULONG ulMaxSegCount = PAGE_SIZE/16 - 2;		// max. # of descriptors in *pInOutBuffer (max.data is ~2Mbyte @ 4k page)
	ULONG ulSegmentCount = 0;

	ULONG ulSegSize = ulSize;
	if (pDevExt->ulMaxSegmentSize >= PAGE_SIZE)			// testing.. allocate small segments
		ulSegSize = min(ulSegSize, pDevExt->ulMaxSegmentSize);

	ULONG ulAllocSize = ulMaxSegCount*sizeof DriverBufferDescriptor_T;

	// Allocate array to hold descriptors
	DriverBufferDescriptor_T *pMyDescr = (DriverBufferDescriptor_T*)ExAllocatePool(NonPagedPool, ulAllocSize);
	if (!pMyDescr) return NULL;

	RtlZeroMemory(pMyDescr, ulAllocSize);

	DriverBufferDescriptor_T *p = pMyDescr;
	PUCHAR pGabiDescr = pInOutBuffer;
	while (ulSize)
	{
		ULONG s = min(ulSegSize, ulSize);
		PVOID a = AllocateContiguousMemory(s, Phys4GB);
		if (!a)
		{
			ulSegSize = (ULONG)(ROUND_TO_PAGES(ulSegSize))/2;	// is an ULONG_PTR on 64 bit
			if (ulSegSize < PAGE_SIZE)
			{
				FreeBuffers(pMyDescr);
				return NULL;
			}
			continue;
		}
		p->ulSize = s;
		p->pVirtual = (PUCHAR)a;
		p->Physical = MmGetPhysicalAddress(a);

		// fill gabi descriptor
		*(PHYSICAL_ADDRESS*)pGabiDescr = p->Physical;
		pGabiDescr += sizeof(PHYSICAL_ADDRESS);
		// 2 variants of the length field DWORD or QWORD
		if (StupidSize == 8)
		{
			*(ULLONG*)pGabiDescr = s;
			pGabiDescr += 8;
		}
		else /* assume 4 */
		{
			*(ULONG*)pGabiDescr = s;
			pGabiDescr += 4;
		}
		p++;
		ulSize -= s;
		ulSegmentCount++;
		// No space for more descriptors ?
		if ((ulSegmentCount > ulMaxSegCount) && ulSize)
		{
			FreeBuffers(pMyDescr);
			return NULL;
		}
	}
	// pMyDescr was cleared so no need for terminating.
	((PHYSICAL_ADDRESS*)pGabiDescr)->LowPart = 0;
	((PHYSICAL_ADDRESS*)pGabiDescr)->HighPart = 0;

#if 0
	KdPrint(("BuildDescriptorList Dump\n"));
	pGabiDescr = pInOutBuffer;
	p = pMyDescr;
	while (p->ulSize)
	{
		
		KdPrint(("size=%x SIZE=%x phys=%x PHYS=%x virt=%p \n", p->ulSize, *((PULONG)pGabiDescr+2),
					p->Physical.LowPart, ((PHYSICAL_ADDRESS*)pGabiDescr)->LowPart, p->pVirtual));
		p++;
		pGabiDescr += sizeof PHYSICAL_ADDRESS + StupidSize;
	};
#endif

	return pMyDescr;
}
static ULONG CopyFromBuffers(PUCHAR p, DriverBufferDescriptor_T *d, ULONG Max)
{
	ULONG rv = 0;
	while (d->ulSize && Max)
	{
		ULONG c = min(Max, d->ulSize);
		RtlCopyMemory(p, d->pVirtual, c);
		rv += c;
		Max -= c;
		p += c;
		d++;
	}
	return rv;
}
static void CopyToBuffers(DriverBufferDescriptor_T *d, PUCHAR p, ULONG Max)
{
	while (d->ulSize && Max)
	{
		ULONG c = min(Max, d->ulSize);
		RtlCopyMemory(d->pVirtual, p, c);
		Max -= c;
		p += c;
		d++;
	}
}

NTSTATUS
AcpiCompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);

	KEVENT *pEvent = (KEVENT *)Context;
	KeSetEvent(pEvent, IO_NO_INCREMENT, FALSE);

	return STATUS_MORE_PROCESSING_REQUIRED;
}

BOOLEAN HasGabiInterface(PDEVICE_EXTENSION	pDevExt)
{
	return (pDevExt->GabiCallAddress != NULL || (pDevExt->ulUseACPI != 0 && pDevExt->ACPIDevice != NULL)) ? TRUE : FALSE;
}

NTSTATUS
DriverIOCTL ( PDEVICE_OBJECT pDeviceObject, PIRP pIrp )
{
	PDEVICE_EXTENSION	pDevExt	  = (PDEVICE_EXTENSION) pDeviceObject->DeviceExtension;
	PIO_STACK_LOCATION	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	NTSTATUS			ntStatus = STATUS_INVALID_DEVICE_REQUEST;
	ULONG				Information = 0;

	ULONG ulIoctlInputLength  = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG ulIoctlOutputLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	PVOID pSystemBuffer = (PVOID)pIrp->AssociatedIrp.SystemBuffer;
	ULONG *pulUlongSystemBuffer = (ULONG*)pSystemBuffer;
	DriverBufferDescriptor_T *InputBufferDescriptor = NULL;
	DriverBufferDescriptor_T *OutputBufferDescriptor = NULL;
	ULONG UEFI_InCnt, UEFI_OutCnt = 0;

	KdPrint((DRIVER_NAME " - DriverIOCTL entered\n"));

	switch ( pIrpStack->Parameters.DeviceIoControl.IoControlCode )
	{
	case IOCTL_FSC_HARDWARE_PRESENT:
		if (ulIoctlOutputLength == sizeof(*pulUlongSystemBuffer))
		{
			*(ULONG*)pIrp->AssociatedIrp.SystemBuffer = (HasGabiInterface(pDevExt)) ? 1 : 0;
			Information = sizeof(*pulUlongSystemBuffer);
			ntStatus = STATUS_SUCCESS;
		}
		break;

	case IOCTL_FSC_GET_DRIVER_VERSION:
		if (ulIoctlOutputLength == sizeof(*pulUlongSystemBuffer))
		{
			*pulUlongSystemBuffer = DRIVER_VERSION;
			Information = sizeof(*pulUlongSystemBuffer);
			ntStatus = STATUS_SUCCESS;
		}
		break;

	case IOCTL_GABI_GET_GABI_VERSION:
		if (ulIoctlOutputLength == sizeof(*pulUlongSystemBuffer) && HasGabiInterface(pDevExt))
		{
			*pulUlongSystemBuffer = pDevExt->ulGabiVersion;
			Information = sizeof(*pulUlongSystemBuffer);
			ntStatus = STATUS_SUCCESS;
		}
		break;

	case IOCTL_GABI_EXECUTE_REQUEST:
		{
			KdPrint((DRIVER_NAME " - IOCTL_GABI_EXECUTE_REQUEST entered\n"));
			// Gabi not here
			if (!HasGabiInterface(pDevExt)) break;

			ULONG min_in_and_out_size = (Command((GabiGenericAPIHeader_T*)pSystemBuffer) == OTHER_UEFI) ?
											sizeof UEFI_GabiKernelRequest_T : sizeof GabiServiceAPIRequest_T;

			KdPrint((DRIVER_NAME " - ulIoctlInputLength = 0x%x\n", ulIoctlInputLength));
			KdPrint((DRIVER_NAME " - ulIoctlOutputLength = 0x%x\n", ulIoctlOutputLength));
			KdPrint((DRIVER_NAME " - min_in_and_out_size = 0x%x\n", min_in_and_out_size));

			// Too small to hold the control buffer
			if ((ulIoctlInputLength < min_in_and_out_size) ||
				(ulIoctlOutputLength < min_in_and_out_size))
			{
				ntStatus = STATUS_INVALID_PARAMETER;
				break;
			}
		}
		{
			PUCHAR pRequestAndResponseData = (PUCHAR)pSystemBuffer + sizeof(GabiGenericAPIHeader_T);
			GabiGenericAPIHeader_T *request = (GabiGenericAPIHeader_T*)pSystemBuffer;

			ntStatus = STATUS_SUCCESS;

			/*
			 * TODO:
			 * Explain this brain-damaged interface ....
			 *
			 */		
			
			// claim mutex
			KeWaitForMutexObject( &(pDevExt->Mutex), Executive, KernelMode, FALSE, NULL );

			if (!AllocateContiguousBuffer(&pDevExt->ControlBuffer, 1024))
			{
				KeReleaseMutex( &(pDevExt->Mutex), FALSE );
				ntStatus = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}
			/*
			 * Copy the control buffer and add the Allocated size field.
			 */
			RtlCopyMemory(pDevExt->ControlBuffer.pVirtual+2, pSystemBuffer, sizeof(UEFI_GabiKernelControl_T));
			*(PUSHORT)(pDevExt->ControlBuffer.pVirtual) = (USHORT)pDevExt->ControlBuffer.ulSize;	// = 1024

			
			switch (Command(request))
			{
			case OTHER_UEFI:
				{
					UEFI_GabiIO_T *pInBuffer = (UEFI_GabiIO_T*)(pDevExt->InBuffer.pVirtual);
					UEFI_GabiIO_T *pOutBuffer = (UEFI_GabiIO_T*)(pDevExt->OutBuffer.pVirtual);
					if (!AllocateContiguousBuffer(&pDevExt->InBuffer,  PAGE_SIZE) ||
						!AllocateContiguousBuffer(&pDevExt->OutBuffer, PAGE_SIZE))
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						break;
					}
					UEFI_InCnt = ulIoctlInputLength - sizeof(UEFI_GabiKernelRequest_T);
					if (UEFI_InCnt)
					{
						InputBufferDescriptor = BuildDescriptorList(pDevExt, &(pInBuffer->Data[0]), UEFI_InCnt, 8);
						if (!InputBufferDescriptor)
						{
							ntStatus = STATUS_INSUFFICIENT_RESOURCES;
							break;
						}
					}
					UEFI_OutCnt = ulIoctlOutputLength - sizeof(UEFI_GabiKernelResponse_T);
					if (UEFI_OutCnt)
					{
						OutputBufferDescriptor = BuildDescriptorList(pDevExt, &(pOutBuffer->Data[0]), UEFI_OutCnt, 8);
						if (!OutputBufferDescriptor)
						{
							ntStatus = STATUS_INSUFFICIENT_RESOURCES;
							break;
						}
					}
					/* Copy request data */
					RtlCopyMemory(pDevExt->InBuffer.pVirtual, pRequestAndResponseData, sizeof(UEFI_GabiIO_T));
					/* Allocated buffer size */
					*(PUSHORT)pDevExt->InBuffer.pVirtual = (USHORT)pDevExt->InBuffer.ulSize;
					*(PUSHORT)(pDevExt->OutBuffer.pVirtual) = (USHORT)pDevExt->OutBuffer.ulSize;

					if (InputBufferDescriptor)
					{
						CopyToBuffers(InputBufferDescriptor, pRequestAndResponseData+0x10, UEFI_InCnt);
					}
					break;
				}

			case FLASH_UPDATE:
				{
					if (!AllocateContiguousBuffer(&pDevExt->InBuffer,  PAGE_SIZE) ||
						!AllocateContiguousBuffer(&pDevExt->OutBuffer, PAGE_SIZE))
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						break;
					}
					ULONG FlashDataSize = ulIoctlInputLength - sizeof(GabiGenericAPIHeader_T) - 8;
					InputBufferDescriptor = BuildDescriptorList(pDevExt, pDevExt->InBuffer.pVirtual+8, FlashDataSize, 8);
					if (!InputBufferDescriptor)
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						break;
					}
					/* Copy request data */
					RtlCopyMemory(pDevExt->InBuffer.pVirtual, pRequestAndResponseData, 8);
					/* Allocated buffer size, brain ... */
					*(PUSHORT)pDevExt->InBuffer.pVirtual = (USHORT)pDevExt->InBuffer.ulSize;
					/* Copy flash data */
					CopyToBuffers(InputBufferDescriptor, pRequestAndResponseData+8, FlashDataSize);
				}
				break;

			case WRITE_SYS_DATA:
				{
					if (!AllocateContiguousBuffer(&pDevExt->InBuffer,  PAGE_SIZE) ||
						!AllocateContiguousBuffer(&pDevExt->OutBuffer, max(PAGE_SIZE, ulIoctlOutputLength + 256)))
						{
							ntStatus = STATUS_INSUFFICIENT_RESOURCES;
							break;
						}
					ULONG WriteDataSize = ulIoctlInputLength - sizeof(GabiGenericAPIHeader_T) - 0x14;
					// Could check WriteDataSize here.
#if 1
					if (WriteDataSize != *(PULONG)(pRequestAndResponseData+0x10))
					{
						ntStatus = STATUS_INVALID_PARAMETER;
						DbgPrint("FscGabi.sys: inconsistent WriteDataSize\n");
						break;
					}
#endif
					InputBufferDescriptor = BuildDescriptorList(pDevExt, pDevExt->InBuffer.pVirtual+0x14, WriteDataSize, 4);
					if (!InputBufferDescriptor)
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						break;
					}
					/* Copy request data */
					RtlCopyMemory(pDevExt->InBuffer.pVirtual, pRequestAndResponseData, 0x14);
					/* Allocated buffer size, brain ... */
					*(PUSHORT)(pDevExt->InBuffer.pVirtual) = (USHORT)pDevExt->InBuffer.ulSize;
					*(PUSHORT)(pDevExt->OutBuffer.pVirtual) = (USHORT)pDevExt->OutBuffer.ulSize;
					/* Copy system data */
					CopyToBuffers(InputBufferDescriptor, pRequestAndResponseData+0x14, WriteDataSize);
				}
				break;

			case FLASH_ARCHIVE:
				{
					if (!AllocateContiguousBuffer(&pDevExt->InBuffer,  PAGE_SIZE) ||
						!AllocateContiguousBuffer(&pDevExt->OutBuffer, PAGE_SIZE))
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						break;
					}
					ULONG FlashDataSize = ulIoctlOutputLength - sizeof(GabiGenericAPIHeader_T) - 8;
					/* Clear and copy the request buffer */
					RtlZeroMemory(pDevExt->InBuffer.pVirtual, pDevExt->InBuffer.ulSize);
					RtlCopyMemory(pDevExt->InBuffer.pVirtual, pRequestAndResponseData, ulIoctlInputLength - sizeof(GabiGenericAPIHeader_T));
					/* overwrite length with allocated size */
					*(PUSHORT)(pDevExt->InBuffer.pVirtual) = (USHORT)pDevExt->InBuffer.ulSize;
					/* Build descriptor list in output buffer ... */
					OutputBufferDescriptor = BuildDescriptorList(pDevExt, pDevExt->OutBuffer.pVirtual+8, FlashDataSize, 8);
					if (!OutputBufferDescriptor)
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						break;
					}
				}
				break;

			case READ_SYS_DATA:
				{
					if (!AllocateContiguousBuffer(&pDevExt->InBuffer,  PAGE_SIZE) ||
						!AllocateContiguousBuffer(&pDevExt->OutBuffer, PAGE_SIZE))
					{
						KdPrint(("READ_SYS_DATA STATUS_INSUFFICIENT_RESOURCES\n"));
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						break;
					}
					ULONG ReadDataSize = ulIoctlOutputLength - sizeof(GabiGenericAPIHeader_T) - 0xc;
					/* Clear and copy the request buffer */
					RtlZeroMemory(pDevExt->InBuffer.pVirtual, pDevExt->InBuffer.ulSize);
					RtlCopyMemory(pDevExt->InBuffer.pVirtual, pRequestAndResponseData, ulIoctlInputLength - sizeof(GabiGenericAPIHeader_T));
					/* overwrite length with allocated size */
					*(PUSHORT)(pDevExt->InBuffer.pVirtual) = (USHORT)pDevExt->InBuffer.ulSize;
					*(PUSHORT)(pDevExt->OutBuffer.pVirtual) = (USHORT)pDevExt->OutBuffer.ulSize;
					/* Who sets ReadDataSize in the response buffer ? Or is it part of the request ? */
					*(PULONG)(pDevExt->OutBuffer.pVirtual+0x8) = ReadDataSize;
					/* Build desciptor list in output buffer ... */
					OutputBufferDescriptor = BuildDescriptorList(pDevExt, pDevExt->OutBuffer.pVirtual+0xc, ReadDataSize, 4);
					if (!OutputBufferDescriptor)
					{
						KdPrint(("READ_SYS_DATA !BufferDescriptor\n"));
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						break;
					}
				}
				break;

			case OTHERS:
				const ULONG AllocContSize = PAGE_SIZE;
				if (!AllocateContiguousBuffer(&pDevExt->InBuffer,  max(AllocContSize, ulIoctlInputLength + 256)) ||
					!AllocateContiguousBuffer(&pDevExt->OutBuffer, max(AllocContSize, ulIoctlOutputLength + 256)))
				{
					ntStatus = STATUS_INSUFFICIENT_RESOURCES;
					break;
				}
				/*
				 * Copy the request buffer
				 * The design is brain-damaged and specifies that unused fields in the request buffer must be cleared.
				 */
				RtlZeroMemory(pDevExt->InBuffer.pVirtual, pDevExt->InBuffer.ulSize);
				RtlCopyMemory(pDevExt->InBuffer.pVirtual, pRequestAndResponseData, ulIoctlInputLength - sizeof(GabiGenericAPIHeader_T));

				/*
				 * And the settings API specifies that the length is a useless value but at least 512.
				 * Did I already say it's brain-damaged ?
				 * And because the settings API was such a success we copied the specification
				 * when defining FLASH and SYSTEM DATA ACCESS APIs, ough. So assume this is valid for all APIs.
				 */
				*(PUSHORT)(pDevExt->InBuffer.pVirtual) = (USHORT)pDevExt->InBuffer.ulSize;
				*(PUSHORT)(pDevExt->OutBuffer.pVirtual) = (USHORT)pDevExt->OutBuffer.ulSize;

				/*
				 * Size of data is NOT passed to BIOS.
				 */
				break;
			}

			/* Everything ok upto now ? */
			if (ntStatus != STATUS_SUCCESS)
			{
				KeReleaseMutex( &(pDevExt->Mutex), FALSE );
				break;
			}

DbgPrint("ulIoctlInputLength=%x In.ulSize=%x  ulIoctlOutputLength=%x Out.ulSize=%x\n", ulIoctlInputLength,pDevExt->InBuffer.ulSize,ulIoctlOutputLength,pDevExt->OutBuffer.ulSize);
//x("Ctrl", pDevExt->ControlBuffer.pVirtual, 16);
//x("In", pDevExt->InBuffer.pVirtual, pDevExt->InBuffer.ulSize);
//x("Out", pDevExt->OutBuffer.pVirtual, 64);

DbgPrint(" ----- GABI CALL ------- \n", ulIoctlInputLength,pDevExt->InBuffer.ulSize,ulIoctlOutputLength,pDevExt->OutBuffer.ulSize);

			if (pDevExt->ulUseACPI != 0 && pDevExt->ACPIDevice != NULL)
			{
				DbgPrint("---- ACPI call ----");

				KEVENT Event;
				PIO_STACK_LOCATION nextStack;
				PIRP Irp = IoAllocateIrp(pDevExt->ACPIDevice->StackSize, FALSE);
				PGabiAcpiCmd pCmd = NULL;

				if (pIrp == NULL)
				{
					ntStatus = STATUS_INSUFFICIENT_RESOURCES;
					KeReleaseMutex(&(pDevExt->Mutex), FALSE);
					break;
				}

				nextStack = IoGetNextIrpStackLocation(Irp);

				nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
				nextStack->MinorFunction = 0;
				nextStack->DeviceObject = pDevExt->ACPIDevice;
				nextStack->Parameters.DeviceIoControl.OutputBufferLength = 0;
				nextStack->Parameters.DeviceIoControl.InputBufferLength = sizeof(GabiAcpiCmd);
				nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_GABI_ACPI_CMD;

				Irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithQuotaTag(NonPagedPool, sizeof(GabiAcpiCmd), 'AcGi');
				pCmd = (PGabiAcpiCmd)Irp->AssociatedIrp.SystemBuffer;

				if (Irp->AssociatedIrp.SystemBuffer == NULL)
				{
					/* Free the IRP and fail */
					IoFreeIrp(Irp);
					ntStatus = STATUS_INSUFFICIENT_RESOURCES;
					KeReleaseMutex(&(pDevExt->Mutex), FALSE);
					break;
				}

				nextStack->Parameters.DeviceIoControl.Type3InputBuffer = Irp->AssociatedIrp.SystemBuffer;

				Irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
				Irp->MdlAddress = NULL;

				KeInitializeEvent(&Event, NotificationEvent, FALSE);

				IoSetCompletionRoutine(Irp, AcpiCompletionRoutine, &Event, TRUE, TRUE, TRUE);

				//fill cmd buffer
				RtlZeroMemory(pCmd, sizeof(GabiAcpiCmd));

				pCmd->FunctionIndex = 1;
				pCmd->RequestBuffer = pDevExt->InBuffer.Physical;
				pCmd->ResponseBuffer = pDevExt->OutBuffer.Physical;
				pCmd->ControlBuffer = pDevExt->ControlBuffer.Physical;

				//call ACPI driver
				ntStatus = IoCallDriver(pDevExt->ACPIDevice, Irp);

				if (NT_SUCCESS(ntStatus))
				{
					KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

					ntStatus = Irp->IoStatus.Status;
				}

				if (Irp->AssociatedIrp.SystemBuffer != NULL)
				{
					ExFreePoolWithTag(Irp->AssociatedIrp.SystemBuffer, 'AcGi');
				}

				IoFreeIrp(Irp);

				if (ntStatus != STATUS_SUCCESS)
				{
					KeReleaseMutex(&(pDevExt->Mutex), FALSE);
					break;
				}
			}
			else if(pDevExt->pInterpreterContext != NULL)
			{
				DbgPrint("---- Interpreter call ----");
				eInterpreterReturn r = AnalyzeInterpreter(pDevExt->pInterpreterContext);

				if (r == INTERPRETER_OK)
				{
					r = ExecuteInterpreter(pDevExt->pInterpreterContext, pDevExt->InBuffer.Physical, pDevExt->OutBuffer.Physical, pDevExt->ControlBuffer.Physical);
				}

				if (r != INTERPRETER_OK)
				{
					ntStatus = STATUS_ILLEGAL_INSTRUCTION;
					KeReleaseMutex(&(pDevExt->Mutex), FALSE);
					break;
				}
			}
			else
			{
				DbgPrint("---- ASM call ----");

				BapiCall(pDevExt->GabiCallAddress,
					pDevExt->InBuffer.Physical,
					pDevExt->OutBuffer.Physical,
					pDevExt->ControlBuffer.Physical);
			}

//x("Ctrl", pDevExt->ControlBuffer.pVirtual, 16);
//x("In", pDevExt->InBuffer.pVirtual, pDevExt->InBuffer.ulSize);
//x("Out", pDevExt->OutBuffer.pVirtual, pDevExt->OutBuffer.ulSize);

			// copy the control buffer without the length field
			RtlCopyMemory(pSystemBuffer, pDevExt->ControlBuffer.pVirtual+2, sizeof(GabiGenericAPIHeader_T));
			Information = sizeof(GabiGenericAPIHeader_T);

			switch (Command(request))
			{

			case OTHER_UEFI:
				{
					UEFI_GabiIO_T *pOutBuffer = (UEFI_GabiIO_T*)(pDevExt->OutBuffer.pVirtual);
					RtlCopyMemory(pRequestAndResponseData, pOutBuffer, sizeof(*pOutBuffer));
					Information += sizeof(*pOutBuffer);
					if (OutputBufferDescriptor)
					{
						Information += 
								CopyFromBuffers(pRequestAndResponseData + sizeof(UEFI_GabiIO_T), OutputBufferDescriptor, UEFI_OutCnt);
					}
					ntStatus = STATUS_SUCCESS;
				}
				break;

			case FLASH_ARCHIVE:
				{
					ULONG FlashDataSize = ulIoctlOutputLength - sizeof(GabiGenericAPIHeader_T) - 8;
					RtlCopyMemory(pRequestAndResponseData, pDevExt->OutBuffer.pVirtual, 8);
					Information += 8;
					// overwrite "Response Data Size", the specification is unclear and Japan implemented something
					// but not what they wanted to implement, whatever that was .....
					*(PUSHORT)pRequestAndResponseData = 8;
					Information += CopyFromBuffers(pRequestAndResponseData + 8, OutputBufferDescriptor, FlashDataSize);
					ntStatus = STATUS_SUCCESS;
				}
				break;

			case READ_SYS_DATA:
				{
					// Hopefully this includes a usefull ReadDataSize field
					RtlCopyMemory(pRequestAndResponseData, pDevExt->OutBuffer.pVirtual, 0xc);
					Information += 0xc;
					// overwrite "Response Data Size", the specification is unclear and Japan implemented something
					// but not what they wanted to implement, whatever that was .....
					*(PUSHORT)pRequestAndResponseData = 0xc;
					/* Brain damaged design */
					ULONG ReadDataSizeShouldBe = ulIoctlOutputLength - sizeof(GabiGenericAPIHeader_T) - 0xc;
					Information += CopyFromBuffers(pRequestAndResponseData + 0xc, OutputBufferDescriptor, ReadDataSizeShouldBe);
					ntStatus = STATUS_SUCCESS;
				}
				break;

			default:
			case FLASH_UPDATE:
				// fall thru
			case WRITE_SYS_DATA:
				// fall thru
			case OTHERS:
				{
					// compute amount of response data, take care of size. Minimum size is 2 bytes (the length field)
					// It would have been soooo nice, but again the ResponseDataSize seems to be wrong, so deliver the amount asked for.
					// ULONG maxdata = min(ulIoctlOutputLength - SizeOfStatusHeader, *(PUSHORT)(pDevExt->OutBuffer.pVirtual));
					ULONG maxdata = min(ulIoctlOutputLength - sizeof(GabiGenericAPIHeader_T), pDevExt->OutBuffer.ulSize);
					RtlCopyMemory(pRequestAndResponseData, pDevExt->OutBuffer.pVirtual, maxdata);
					Information += maxdata;
					ntStatus = STATUS_SUCCESS;
				}
				break;
			}

			KeReleaseMutex( &(pDevExt->Mutex), FALSE );
			break;
		}

	default:
		ntStatus = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	if (InputBufferDescriptor) FreeBuffers(InputBufferDescriptor);
	if (OutputBufferDescriptor) FreeBuffers(OutputBufferDescriptor);

	pIrp->IoStatus.Information = Information;
	pIrp->IoStatus.Status = ntStatus;
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);

	return ntStatus;
}

#pragma LOCKEDCODE

// This driver does all IOCTL without queueing, so this is a dummy
VOID StartIo ( PDEVICE_OBJECT fdo, PIRP pIrp )
{
	UNREFERENCED_PARAMETER(fdo);
	UNREFERENCED_PARAMETER(pIrp);
}