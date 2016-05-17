#include "stdafx.h"
#include "FSCIoctl.h"
#include "..\..\Driver\Sources\driver.h"


#define DLL_NAME "InSmbIch.dll"
#define INF_NAME "SmbIch.inf"
#define DEVICE_NAME "\\\\.\\SMBus0"

// Do version probing, do not rely on Microsoft :-)
#define HAS_CAT_FILE_I386	0
#define HAS_CAT_FILE_AMD64	0

#include "GenericInstallPnp.h"
