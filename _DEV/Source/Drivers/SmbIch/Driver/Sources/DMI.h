#ifndef _DMI_H_
#define _DMI_H_

BOOLEAN MapDMI( PDEVICE_EXTENSION pDevExt );
VOID UnmapDMI( PDEVICE_EXTENSION pdx );
#pragma pack(push,1)

// DMI BIOS Structure Type
#define DMI_TYPE_BIOS_INFO						0
#define DMI_TYPE_SYSTEM_INFO					1
#define DMI_TYPE_BASEBOARD_INFO					2
#define DMI_TYPE_SYSTEM_ENCLOSURE_OR_CHASSIS	3
#define DMI_TYPE_PROCESSOR_INFO					4

// DMI Structures corresponding to DMI Spec V2.0
typedef struct _DMI_20_INFORMATION{
    UCHAR   DmiBiosRevision;
    USHORT  NumStructures;
    USHORT  StructureSizeMax;
    ULONG   DmiStorageBase;
    USHORT  DmiStorageSize;
} DMI_20_INFORMATION, * LPDMI_20_INFORMATION;  

typedef struct _DMI_20_HEADER{
    UCHAR   Type;
    UCHAR   Length;
    USHORT  Handle;
} DMI_20_HEADER, * LPDMI_20_HEADER;  

typedef UCHAR DMI_STRING;

typedef struct _DMI_TYPE0_HEADER {
	DMI_20_HEADER	Header;
	DMI_STRING		Vendor;
	DMI_STRING		Version;
	USHORT			BiosStart;
	DMI_STRING		Date;
	// More follows
} DMI_TYPE0_HEADER;

typedef struct _DMI_TYPE1_HEADER {
	DMI_20_HEADER	Header;
	DMI_STRING		Vendor;
	DMI_STRING		Product;
	DMI_STRING		Version;
	// More follows
} DMI_TYPE1_HEADER;

typedef struct _DMI_TYPE2_HEADER {
	DMI_20_HEADER	Header;
	DMI_STRING		Vendor;
	DMI_STRING		Product;
	DMI_STRING		Version;
	// More follows
} DMI_TYPE2_HEADER;

typedef struct _DMI_TYPE3_HEADER {
	DMI_20_HEADER	Header;
	DMI_STRING		Vendor;
	UCHAR			Type;
	DMI_STRING		Version;
	// More follows
} DMI_TYPE3_HEADER;



// SMBIOS Structure Table Entry Point
typedef struct _SMBIOS_TABLE_ENTRY
{
	ULONG	AnchorString;					// 4 Bytes, "_SM_" ASCII-characters
	UCHAR	Checksum;						// checksum
	UCHAR	Length;							// length of structure
	UCHAR	VersionMajor;					// major version
	UCHAR	VersionMinor;					// minor version
	USHORT	MaximumStructureSize;			// size of largest SMBios structure
	UCHAR	Revision;						// revision
	UCHAR	FormattedArea[5];
	UCHAR	IntermediateAnchorString[5];	// 5 Bytes, "_DMI_" ASCII-characters
	UCHAR	IntermediateChecksum;			// checksum of Intermediate Entry Point Structure
	USHORT	StructureTableLength;			// Total length of SMBios structure table
	ULONG	StructureTableAddress;			// 32-bit physical starting address of the structure table
	USHORT	NumberOfStructures;				// Total number of structures in structure table
	UCHAR	SMBiosBCDRevision;				// BCD-revision
} SMBIOS_TABLE_ENTRY	, *LPSMBIOS_TABLE_ENTRY;



#pragma pack(pop)


#define DMI_BIOS_START_ADDRESS		0xe0000
#define DMI_BIOS_MAP_LENGTH			0x20000
//#define BIOS_MAP_LENGTH		0xFF80		// -128 Bytes
#define DMI_BIOS_MAGIC_DWORD	0x494d445f	// "_DMI"

#define SM_BIOS_MAGIC_DWORD		"_SM_"		// "_SM_" 0x5f4d535f


// FSC specific DMI 
#pragma pack(push,1)

#define DMI_DATA_ACCESS_METHOD_MAX_LENGTH	128		// Maximum length of an access method is expected to be 128 Bytes
// temporarily used structures
// DMI Type 185 : System Monitoring
typedef struct _DMI_DATA
{
	UCHAR		Type;
	UCHAR		Length;
	USHORT		Handle;
	UCHAR		SubType;
	UCHAR		HardwareId;

	UCHAR		AccessMethod[DMI_DATA_ACCESS_METHOD_MAX_LENGTH];
} DMI_DATA, *PDMI_DATA;

// DMI Type 188 : BIOS Identification
typedef struct _BIOS_IDENTIFICATION
{
	DMI_20_HEADER	DmiHeader;
	ULONG			MagicBIOSIdentifier;  // has to be '$188'
} BIOS_IDENTIFICATION, *PBIOS_IDENTIFICATION;


typedef struct _DMI_OMF_BOARD_DATA
{
	DMI_20_HEADER	DmiHeader;
	USHORT			Board_HW_Config;
	ULONG			OMFBoardID;
	USHORT			Board_HW_GS;
	USHORT			Board_HW_Variante;
	USHORT			Chassis_Geometrie;
} DMI_OMF_BOARD_DATA, *PDMI_OMF_BOARD_DATA;

typedef struct _DMI_SYSTEM_ENCLOSURE
{
	DMI_20_HEADER	DmiHeader;
	UCHAR			Manufacturer;
	UCHAR			Type;
	UCHAR			Version;
	UCHAR			Fabriknummer;
	UCHAR			AssetTagNumber;
} DMI_SYSTEM_ENCLOSURE, *PDMI_SYSTEM_ENCLOSURE;

#pragma pack(pop)

#endif //_DMI_H_
