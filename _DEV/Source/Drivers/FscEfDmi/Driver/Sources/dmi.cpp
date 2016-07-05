#ifdef __cplusplus
	extern "C" {
#endif

#include <ntddk.h>
#ifdef __cplusplus
	}
#endif

#include "stddcls.h"
#include "driver.h"
#include "dmi.h"

BOOLEAN MapDMI( PDEVICE_EXTENSION pdx )
{
	PUCHAR						BaseAddress_DMIStructures;
	SMBIOS_TABLE_ENTRY			*SMBiosTableEntry = NULL;
	BOOLEAN						bSMBiosFound	= FALSE;

	// search for SM_BIOS Magic DWORD ("_SM_")
	// Look for signature on 16 byte boundaries
#define STEP_SIZE 16
	for( int i=0 ; i < DMI_BIOS_MAP_LENGTH/STEP_SIZE ; i++ )
	{
		if ( RtlCompareMemory( pdx->pMappedESegment + i*STEP_SIZE , SM_BIOS_MAGIC_DWORD, sizeof(ULONG) ) == sizeof(ULONG) )
		{
			SMBiosTableEntry = (SMBIOS_TABLE_ENTRY *) (pdx->pMappedESegment + i*STEP_SIZE);

			// Is intermediate anchor string ("_DMI_") present and correct ?
			if ( RtlCompareMemory(SMBiosTableEntry->IntermediateAnchorString, "_DMI_", 5) != 5 )
				// No
				continue;

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
				bSMBiosFound = TRUE;
				break;
			}
			else
				DbgPrint("EFDMI.sys:" __FILE__ "Invalid SMBIOS checksum ???\n");
		}
	}

	// If SM Bios could not be found, then return
	if ( !bSMBiosFound )
	{
		DbgPrint("EFDMI.sys:" __FILE__ " SMBios not detected\n");
		return FALSE;
	}

    if (SMBiosTableEntry == NULL)
    {
        DbgPrint("EFDMI.sys:" __FILE__ " SMBiosTableEntry error\n");
        return FALSE;
    }

	// map the DMI structures physical address range
	PHYSICAL_ADDRESS			PhAddress;
	PhAddress.HighPart = 0;
	PhAddress.LowPart = SMBiosTableEntry->StructureTableAddress;
	BaseAddress_DMIStructures = (PUCHAR) MmMapIoSpace (PhAddress, SMBiosTableEntry->StructureTableLength, MmNonCached);

	if ( BaseAddress_DMIStructures == NULL )
	{
		return FALSE;
	}

	// Remains mapped
	pdx->pucDMI = BaseAddress_DMIStructures;
	pdx->ulDMISize = SMBiosTableEntry->StructureTableLength;
	pdx->ulDMIStructCount = SMBiosTableEntry->NumberOfStructures;
	pdx->ucDMIVersionMajor = SMBiosTableEntry->VersionMajor;
	pdx->ucDMIVersionMinor = SMBiosTableEntry->VersionMinor;

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
