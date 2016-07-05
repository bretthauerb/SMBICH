#ifndef _DMI_H_
#define _DMI_H_

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

#endif //_DMI_H_
