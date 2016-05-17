#include "stddcls.h"
#include "driver.h"
#include "DebugPrint.h"
#include "dmi.h"

BOOLEAN MapDMI( PDEVICE_EXTENSION pdx )
{
	PUCHAR						BaseAddress_DMIStructures;
	PHYSICAL_ADDRESS			PhAddress;
	SMBIOS_TABLE_ENTRY			*SMBiosTableEntry = NULL;
	BOOLEAN						bSMBiosFound				= FALSE;

	//
	// First map BIOS memory
	//
	PhAddress.HighPart = 0;
	PhAddress.LowPart = DMI_BIOS_START_ADDRESS;

	PUCHAR SearchBase = (PUCHAR) MmMapIoSpace (PhAddress, DMI_BIOS_MAP_LENGTH, MmNonCached);
	if ( SearchBase == NULL )
	{
		DebugPrint(DEBUGLEVEL_ERROR, "DriverGetDataStructuresSMBios : MmMapIoSpace Unsuccess");
		return FALSE;
	}

	// search for SM_BIOS Magic DWORD ("_SM_")
	// Look for signature on 16 byte boundaries
#define STEP_SIZE 16
	for( int i=0 ; i < DMI_BIOS_MAP_LENGTH/STEP_SIZE ; i++ )
	{
		if ( RtlCompareMemory( SearchBase + i*STEP_SIZE , SM_BIOS_MAGIC_DWORD, sizeof(ULONG) ) == sizeof(ULONG) )
		{
			SMBiosTableEntry = (SMBIOS_TABLE_ENTRY *) (SearchBase + i*STEP_SIZE);

			// Is intermediate anchor string ("_DMI_") present and correct ?
			if ( RtlCompareMemory(SMBiosTableEntry->IntermediateAnchorString, "_DMI_", 5) != 5 )
				// No
				continue;

			// Signature for SMBios found
			DebugPrint(DEBUGLEVEL_DEBUG, "DriverGetDataStructuresSMBios : Signature for SMBios found at %p!", SMBiosTableEntry);

			//
			// checksum match?
			//
			ULONG	ulCount		= 0;
			UCHAR	Checksum	= 0;
			PUCHAR	p = (PUCHAR)SMBiosTableEntry;
			for ( ulCount=0 ; ulCount < SMBiosTableEntry->Length ; ulCount++)
			{
				Checksum = (UCHAR) (Checksum + *p++);
			}
			
			if ( Checksum == 0 )
			{
				// Signature for SMBios found
				DebugPrint(DEBUGLEVEL_DEBUG, "DriverGetDataStructuresSMBios : Valid SMBios found at %p!", SMBiosTableEntry);
				bSMBiosFound = TRUE;
				break;
			}
		}
	}

	// If SM Bios could not be found, then return
	if ( !bSMBiosFound )
	{
		MmUnmapIoSpace( SearchBase, DMI_BIOS_MAP_LENGTH );
		DebugPrint(DEBUGLEVEL_INFO, "DriverGetDataStructuresSMBios : SMBios not detected!");
		return FALSE;
	}

	// map the DMI structures physical address range
    if(SMBiosTableEntry == NULL)
    {
        MmUnmapIoSpace(SearchBase, DMI_BIOS_MAP_LENGTH);
        DebugPrint(DEBUGLEVEL_INFO, "SMBiosTableEntry not initialized");
        return FALSE;
    }

//	PhAddress.HighPart = 0;
	PhAddress.LowPart = SMBiosTableEntry->StructureTableAddress;
	BaseAddress_DMIStructures = (PUCHAR) MmMapIoSpace (PhAddress, SMBiosTableEntry->StructureTableLength, MmNonCached);

	if ( BaseAddress_DMIStructures == NULL )
	{
		MmUnmapIoSpace( SearchBase, DMI_BIOS_MAP_LENGTH );
		DebugPrint(DEBUGLEVEL_ERROR, "DriverGetDataStructuresSMBios : MmMapIoSpace of DMI structures Unsuccess");
		return FALSE;
	}

	// Remains mapped
	pdx->pucDMI = BaseAddress_DMIStructures;
	pdx->ulDMISize = SMBiosTableEntry->StructureTableLength;
	pdx->ulDMIStructCount = SMBiosTableEntry->NumberOfStructures;

	// Unmap the BIOS F-Segment, don't need it anymore.
	MmUnmapIoSpace( SearchBase, DMI_BIOS_MAP_LENGTH );

	return TRUE;
}

VOID UnmapDMI( PDEVICE_EXTENSION pdx )
{
	if (pdx->pucDMI)
	{
		MmUnmapIoSpace( pdx->pucDMI, pdx->ulDMISize );
		pdx->pucDMI = NULL;
	}
}