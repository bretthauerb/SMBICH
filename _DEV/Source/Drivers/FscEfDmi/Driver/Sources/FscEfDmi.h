#ifndef _EFDMI_H_
#define _EFDMI_H_

#define EfdmiServiceName "FscEfdmi"
#define EfdmiDriverImage "FscEfdmi.sys"
#define EfdmiDriverName	"\\\\.\\FSC_EFDMI"

#ifndef _NTDDK_
#include <winioctl.h>
#endif

// ============
// IOCTL:
// ============

#define FILE_DEVICE_EFDMI	0xA900	// custom number in the range 0x8000 to 0xFFFF


// IOCTL_EXECUTE_REQUEST -----------------------------------------------
#define IOCTL_EFDMI_GET_E_AND_F_SEGMENT CTL_CODE(FILE_DEVICE_EFDMI, 0, METHOD_BUFFERED,FILE_ANY_ACCESS)

// IOCTL_EFDMI_GET_DMI_SIZE has 3 variants (see DMI Specification, SMBIOS Structure Table Entry Point
// output size = 4:
//		returns ULONG Structure_Table_Length (USHORT in DMI spec.)
// output size = 8:
//		returns packed(1) struct{
//			ULONG Structure_Table_Length (USHORT in DMI spec.)
//			ULONG Number_Of_Structures	 (USHORT in DMI spec.)
//		}
// output size = 10:
//		returns packed(1) struct{
//			ULONG Structure_Table_Length (USHORT in DMI spec.)
//			ULONG Number_Of_Structures	 (USHORT in DMI spec.)
//			UCHAR Major_Version
//			UCHAR Minor_Version
//		}
#define IOCTL_EFDMI_GET_DMI_SIZE		CTL_CODE(FILE_DEVICE_EFDMI, 1, METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_EFDMI_GET_DMI_DATA		CTL_CODE(FILE_DEVICE_EFDMI, 2, METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_EFDMI_GET_PHYSICAL_MEMORY CTL_CODE(FILE_DEVICE_EFDMI, 3, METHOD_BUFFERED,FILE_ANY_ACCESS)

// For testing only
#define IOCTL_EFDMI_GET_PHYSICAL_ADDRESS CTL_CODE(FILE_DEVICE_EFDMI, 13, METHOD_BUFFERED,FILE_ANY_ACCESS)

#endif
