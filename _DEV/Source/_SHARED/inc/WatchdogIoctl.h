
#define IOCTL_WATCHDOG_READCONFIG_REG \
	CTL_CODE( FILE_DEVICE_UNKNOWN, 0x901, METHOD_BUFFERED,FILE_ANY_ACCESS )

#define IOCTL_WATCHDOG_WRITECONFIG_REG \
	CTL_CODE( FILE_DEVICE_UNKNOWN, 0x902, METHOD_BUFFERED,FILE_ANY_ACCESS )

// That is how the driver was compiled
#pragma pack(push,4)
typedef struct
{
	unsigned char	ConfigRegisterAdr;
	unsigned char	Data;
} WATCHDOG_INFO, *pWATCHDOG_INFO;
#pragma pack(pop)