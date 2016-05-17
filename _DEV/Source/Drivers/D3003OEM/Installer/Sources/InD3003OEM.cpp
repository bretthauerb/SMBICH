// InD3003OEM.cpp
//

#include "stdafx.h"
#include <tchar.h>
#include <Msiquery.h>

#include "..\..\Driver\Sources\driver.h"
#include "FSCIoctl.h"

#define ROOT_DEVICE_ID "ROOT\\D3003OEM"		
#define SHARE_NAME "D3003OEM.sys"
#define DLL_NAME "InD3003OEM.dll"
#define INF_NAME "D3003OEM.inf"
#define DEVICE_NAME "\\\\.\\SMBUS_OEM"


// Only used to remove the old service installed with CreateService
#define OLD_SERVICE_NAME "DummyBecauseOldSeviceDidNotExist"

// No version probing, rely on Microsoft :-)
#define NO_VERSION_CHECK_I386	1
#define NO_VERSION_CHECK_AMD64	1

// Here is the real code.
#include "GenericInstallPnp.h"
