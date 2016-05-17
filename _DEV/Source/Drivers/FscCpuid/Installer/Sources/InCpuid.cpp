// InCpuid.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <tchar.h>
#include <Msiquery.h>

#include "..\..\Driver\Sources\driver.h"
#include "FSCIoctl.h"

#define ROOT_DEVICE_ID "ROOT\\FscCpuid"		
#define SHARE_NAME "FscCpuid.sys"
#define DLL_NAME "InCpuid.dll"
#define INF_NAME "FscCpuid.inf"
#define DEVICE_NAME "\\\\.\\FSC_CPUID"

// Only used to remove the old service installed with CreateService
#define OLD_SERVICE_NAME "FscCpuid"

// No version probing, rely on Microsoft :-)
#define NO_VERSION_CHECK_I386	1
#define NO_VERSION_CHECK_AMD64	1

// Here is the real code.
#include "GenericInstallPnp.h"
