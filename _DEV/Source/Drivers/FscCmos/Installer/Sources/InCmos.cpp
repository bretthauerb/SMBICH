// InCmos.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <tchar.h>
#include <Msiquery.h>

#include "..\..\Driver\Sources\driver.h"
#include "FSCIoctl.h"

#define ROOT_DEVICE_ID "ROOT\\FscCmos"		
#define SHARE_NAME "FscCmos.sys"
#define DLL_NAME "InCmos.dll"
#define INF_NAME "FscCmos.inf"
#define DEVICE_NAME "\\\\.\\FSC_CMOS"

// Only used to remove the old service installed with CreateService
#define OLD_SERVICE_NAME "FscCmos"

// No version probing, rely on Microsoft :-)
#define NO_VERSION_CHECK_I386	1
#define NO_VERSION_CHECK_AMD64	1

// Here is the real code.
#include "GenericInstallPnp.h"
