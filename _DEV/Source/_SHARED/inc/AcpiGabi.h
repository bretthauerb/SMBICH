/*++

Module Name:

	public.h

Abstract:

	This module contains the common declarations shared by driver
	and user applications.

Environment:
	user and kernel

--*/
#pragma once

//
// Define an Interface Guid so that app can find the device and talk to it.
//
DEFINE_GUID (GUID_DEVINTERFACE_GabiAcpi,
	0x7db1997c,0x5376,0x4917,0x85,0x2e,0xe7,0x8c,0x28,0x00,0x44,0x01);
// {7db1997c-5376-4917-852e-e78c28004401}

#pragma pack(push,1)

typedef struct
{
	ULONG Revision;
	ULONG FunctionIndex;
	PHYSICAL_ADDRESS ControlBuffer;
	ULONG ControlBufferLen;
	PHYSICAL_ADDRESS RequestBuffer;
	ULONG RequestBufferLen;
	PHYSICAL_ADDRESS ResponseBuffer;
	ULONG ResponseBufferLen;
	ULONG AddressLength;  //Possible values: 0 or 8. If 8 then struct contains sub memory blocks
}
GabiAcpiCmd, *PGabiAcpiCmd;

#pragma pack(pop)

#define IOCTL_INDEX             0x800
#define IOCTL_GABI_ACPI_CMD		CTL_CODE( FILE_DEVICE_ACPI,   \
														IOCTL_INDEX + 1,    \
														METHOD_BUFFERED,    \
														FILE_WRITE_DATA)