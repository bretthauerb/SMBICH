// InEfDmi.cpp
//

#include "stdafx.h"
#include <tchar.h>
#include <Msiquery.h>

#include "..\..\Driver\Sources\driver.h"
#include "FSCIoctl.h"

#define ROOT_DEVICE_ID "ROOT\\FscEfDmi"		
#define SHARE_NAME "FscEfDmi.sys"
#define DLL_NAME "InEfDmi.dll"
#define INF_NAME "FscEfDmi.inf"
#define DEVICE_NAME "\\\\.\\FSC_EFDMI"


// Only used to remove the old service installed with CreateService
#define OLD_SERVICE_NAME "FscEfDmi"

// No version probing, rely on Microsoft :-)
#define NO_VERSION_CHECK_I386	1
#define NO_VERSION_CHECK_AMD64	1

// Here is the real code.
#include "GenericInstallPnp.h"
