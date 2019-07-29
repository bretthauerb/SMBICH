//***************************************************************************************
// SMBUSIOCTL.h - include file for SNISMBUSDrv / smbusdrv device for Windows NT
//***************************************************************************************
//
//
//      Author:     Klaus Greiner
//
//      Date:       Sept. 1999
//
//      Changes:    date    name
//                             
//
//***************************************************************************************

#ifndef _SMBUSIOCTL_H
#define _SMBUSIOCTL_H

#define SMBUS_DRIVER_NAME			"SMBus0"
#define SMBUS_DRIVER_NAME_L			L"SMBus0"

#define FILE_DEVICE_SNISMBDRV		0xCBA9	// costum number in the range 0x8000 to 0xFFFF
#define FILE_FUNCTION_BASE			0xC00	// costum number in the range 0x800 to 0xFFF



// INTEL IOControlCodes

#define IOCTL_SMBus_ByteRead \
	CTL_CODE( FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED,FILE_ANY_ACCESS )
#define IOCTL_SMBus_ByteWrite \
	CTL_CODE( FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED,FILE_ANY_ACCESS )
#define IOCTL_SMBus_ByteDataRead \
	CTL_CODE( FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED,FILE_ANY_ACCESS )
#define IOCTL_SMBus_ByteDataWrite \
	CTL_CODE( FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED,FILE_ANY_ACCESS )
#define IOCTL_SMBus_WordDataRead \
	CTL_CODE( FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED,FILE_ANY_ACCESS )
#define IOCTL_SMBus_WordDataWrite \
	CTL_CODE( FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED,FILE_ANY_ACCESS )
#define IOCTL_SMBus_BlockRead \
	CTL_CODE( FILE_DEVICE_UNKNOWN, 0x807, METHOD_BUFFERED,FILE_ANY_ACCESS )
#define IOCTL_SMBus_BlockWrite \
	CTL_CODE( FILE_DEVICE_UNKNOWN, 0x808, METHOD_BUFFERED,FILE_ANY_ACCESS )

// That is how the driver was compiled
#pragma pack(push,4)
typedef struct
{
	unsigned char	SlaveAddress;
	unsigned char	CommandCode;
	unsigned char	DataByteLow;
	unsigned char	DataByteHigh;
	unsigned char	Count;
	unsigned char	BlockBuf[32];
	unsigned char	CommandCodeHigh;
	ULONG			Status;
} SMB_INFO, *PSMB_INFO;
#pragma pack(pop)

#if 0
These is old and should not be used anymore

#pragma pack(push,1)
typedef struct _SMBUS_DATA
{
	ULONG		Command;
	ULONG		DeviceAddress;
	ULONG		Data;
	ULONG		ReturnStatus;
} SMBUS_DATA, *PSMBUS_DATA;

#ifndef BYTE
#define BYTE UCHAR
#endif

typedef struct _SMBUS_DATA_BLOCK
{
	ULONG		DeviceAddress;
	ULONG		WriteLength;
	ULONG		ReadLength;
	BYTE		ReadWriteData[32];
	ULONG		ReturnStatus;
} SMBUS_DATA_BLOCK, *PSMBUS_DATA_BLOCK;

#pragma pack(pop)

// SNI IOControlCodes
#define IOCTL_SMBUS_QUICK_READ \
	(ULONG) CTL_CODE(FILE_DEVICE_SNISMBDRV,FILE_FUNCTION_BASE | 0x0,\
	METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SMBUS_QUICK_WRITE \
	(ULONG) CTL_CODE(FILE_DEVICE_SNISMBDRV,FILE_FUNCTION_BASE | 0x1,\
	METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SMBUS_BYTE_READ \
	(ULONG) CTL_CODE(FILE_DEVICE_SNISMBDRV,FILE_FUNCTION_BASE | 0x2,\
	METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SMBUS_BYTE_WRITE \
	(ULONG) CTL_CODE(FILE_DEVICE_SNISMBDRV,FILE_FUNCTION_BASE | 0x3,\
	METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SMBUS_BYTE_DATA_READ \
	(ULONG) CTL_CODE(FILE_DEVICE_SNISMBDRV,FILE_FUNCTION_BASE | 0x4,\
	METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SMBUS_BYTE_DATA_WRITE \
	(ULONG) CTL_CODE(FILE_DEVICE_SNISMBDRV,FILE_FUNCTION_BASE | 0x5,\
	METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SMBUS_WORD_DATA_READ \
	(ULONG) CTL_CODE(FILE_DEVICE_SNISMBDRV,FILE_FUNCTION_BASE | 0x6,\
	METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SMBUS_WORD_DATA_WRITE \
	(ULONG) CTL_CODE(FILE_DEVICE_SNISMBDRV,FILE_FUNCTION_BASE | 0x7,\
	METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SMBUS_BLOCK_READ \
	(ULONG) CTL_CODE(FILE_DEVICE_SNISMBDRV,FILE_FUNCTION_BASE | 0x8,\
	METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SMBUS_BLOCK_WRITE \
	(ULONG) CTL_CODE(FILE_DEVICE_SNISMBDRV,FILE_FUNCTION_BASE | 0x9,\
	METHOD_BUFFERED,FILE_ANY_ACCESS)

#if 0
moved to FSCIoctl.h, now called IOCTL_FSC_GET_DRIVER_VERSION
#define IOCTL_GET_DRIVER_VERSION \
	(ULONG) CTL_CODE(FILE_DEVICE_SNISMBDRV,FILE_FUNCTION_BASE | 0x9b,\
	METHOD_BUFFERED,FILE_ANY_ACCESS)
#endif
#endif

#define IOCTL_GET_SMBIOS_SIZE \
	(ULONG) CTL_CODE(FILE_DEVICE_SNISMBDRV,FILE_FUNCTION_BASE | 0x99,\
	METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_GET_SMBIOS \
	(ULONG) CTL_CODE(FILE_DEVICE_SNISMBDRV,FILE_FUNCTION_BASE | 0x9a,\
	METHOD_BUFFERED,FILE_ANY_ACCESS)


// Values returned by the next IOCTL
#define SMBUS_HWID_Smbus_2k		0	// Or when this IOCTL is not supported
#define SMBUS_HWID_SmbIch		1	// Or SmbIchW
#define SMBUS_HWID_SmbD1818		2
#define SMBUS_HWID_SmbVIA82		3
#define SMBUS_HWID_SmbSIS96		4
#define SMBUS_HWID_SmbD1692		5	// AMD8111 Smbus controller in LPC
#define SMBUS_HWID_SmbNV		6
#define SMBUS_HWID_SmbATI		7
#define SMBUS_HWID_Argestes		8
#define SMBUS_HWID_SCH5627AMD	9

#define IOCTL_GET_HARDWARE_ID \
	(ULONG) CTL_CODE(FILE_DEVICE_SNISMBDRV,FILE_FUNCTION_BASE | 0x9c,\
	METHOD_BUFFERED,FILE_ANY_ACCESS)

//_______typedefs________________________________________________________________________

#pragma pack(push,1)
typedef struct
{
	ULONG	Size;
	ULONG	DMIStructCount;
} DMI_SIZE_T;
#pragma pack(pop)

typedef ULONG SMBUS_HWID_T;

#pragma pack(push,1)
typedef struct
{
	UCHAR ConfigRegAdr;
	UCHAR ConfigRegData;
} GET_CONFIG_REG_T;
#pragma pack(pop)

#define IOCTL_GET_CONFIG_REG \
	(ULONG) CTL_CODE(FILE_DEVICE_SNISMBDRV,FILE_FUNCTION_BASE | 0x9d,\
	METHOD_BUFFERED,FILE_ANY_ACCESS)

#ifdef _NTDDK_
#ifndef DRIVER_NAME
#define DRIVER_NAME SMBUS_DRIVER_NAME
#endif
#endif

#endif //_SMBUSIOCTL_H
