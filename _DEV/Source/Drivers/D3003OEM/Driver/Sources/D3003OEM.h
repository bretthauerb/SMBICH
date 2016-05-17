#ifndef _D3003OEM_H
#define _D3003OEM_H

#ifndef _NTDDK_
#include <winioctl.h>
#else
//
// PIIX4, ICHx and compatible - SMBus
//
// SMBus Register
#define SMBUS_HOST_STATUS_REGISTER		0xa00
#define SMBUS_SLAVE_STATUS_REGISTER		0xa01
#define SMBUS_HOST_CONTROL_REGISTER		0xa02
#define SMBUS_HOST_COMMAND_REGISTER		0xa03
#define SMBUS_HOST_ADDRESS_REGISTER		0xa04
#define SMBUS_HOST_DATA0_REGISTER		0xa05
#define SMBUS_HOST_DATA1_REGISTER		0xa06
#define SMBUS_HOST_BLOCKDATA_REGISTER	0xa07


// HST_STA Host Status Register (Offset 00h)
//#define SMBUS_HST_STA_BYTE_DONE_STS		0x80	// NO PIIX4 SUPPORT!!!
//#define SMBUS_HST_STA_INUSE_STS			0x40	// NO PIIX4 SUPPORT!!!
//#define SMBUS_HST_STA_SMBALERT_STS		0x20	// NO PIIX4 SUPPORT!!!
#define SMBUS_HST_STA_FAILED			0x10
#define SMBUS_HST_STA_BUS_ERR			0x08
#define SMBUS_HST_STA_DEV_ERR			0x04
#define SMBUS_HST_STA_INTR				0x02
#define SMBUS_HST_STA_HOST_BUSY			0x01
#define SMBUS_HST_STA_INT_MASK			(SMBUS_HST_STA_INTR | SMBUS_HST_STA_DEV_ERR | SMBUS_HST_STA_BUS_ERR | \
										 SMBUS_HST_STA_FAILED )

// HST_CNT Host Control Register (Offset 02h)
#define SMBUS_HST_CNT_START				0x40
#define SMBUS_HST_CNT_LAST_BYTE			0x20	// NO PIIX4 SUPPORT!!
#define SMBUS_HST_CNT_CMD_QUICK			(0<<2)
#define SMBUS_HST_CNT_CMD_BYTE			(1<<2)
#define SMBUS_HST_CNT_CMD_BYTE_DATA		(2<<2)
#define SMBUS_HST_CNT_CMD_WORD_DATA		(3<<2)
#define SMBUS_HST_CNT_CMD_PROCESS_CALL	(4<<2)	// NO PIIX4 SUPPORT!!
#define SMBUS_HST_CNT_CMD_BLOCK			(5<<2)
#define SMBUS_HST_CNT_CMD_I2C_READ		(6<<2)	// NO PIIX4 SUPPORT!!
#define SMBUS_HST_CNT_CMD_RESERVED		(7<<2)
#define SMBUS_HST_CNT_KILL				0x02
#define SMBUS_HST_CNT_INTREN			0x01


#endif

// Write SB600 watch control register bits 0..7
// Bit 3 is read only and cleared (enable) by the driver
// 8 bit input parameter, no output parameter
#define IOCTL_WRITE_WD_CONTROL \
	CTL_CODE( FILE_DEVICE_UNKNOWN, 0x901, METHOD_BUFFERED,FILE_ANY_ACCESS )

// Read SB600 watch control register bits 0..7
// no input parameter, 8 bit output parameter
#define IOCTL_READ_WD_CONTROL \
	CTL_CODE( FILE_DEVICE_UNKNOWN, 0x902, METHOD_BUFFERED,FILE_ANY_ACCESS )

// Write SB600 watch count register
// 32 bit input parameter, no output parameter
#define IOCTL_WRITE_WD_COUNT \
	CTL_CODE( FILE_DEVICE_UNKNOWN, 0x903, METHOD_BUFFERED,FILE_ANY_ACCESS )

// Read SB600 watch count register
// no input parameter, 32 bit output parameter
#define IOCTL_READ_WD_COUNT \
	CTL_CODE( FILE_DEVICE_UNKNOWN, 0x904, METHOD_BUFFERED,FILE_ANY_ACCESS )

// Re-trigger the SB600 watchdog
// no in- or out parameter
#define IOCTL_TRIGGER_WD \
	CTL_CODE( FILE_DEVICE_UNKNOWN, 0x905, METHOD_BUFFERED,FILE_ANY_ACCESS )


#endif