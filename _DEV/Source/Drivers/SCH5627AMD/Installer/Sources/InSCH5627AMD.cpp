// InSCH5627AMD.cpp
//

#include "stdafx.h"
#include <tchar.h>
#include <Msiquery.h>

#include "..\..\Driver\Sources\driver.h"
#include "FSCIoctl.h"

#define ROOT_DEVICE_ID "ROOT\\SCH5627AMD"		
#define SHARE_NAME "SCH5627AMD.sys"
#define DLL_NAME "InSCH5627AMD.dll"
#define INF_NAME "SCH5627AMD.inf"
#define DEVICE_NAME "\\\\.\\SMBus0"

// No version probing, rely on Microsoft :-)
#define NO_VERSION_CHECK_I386	1
#define NO_VERSION_CHECK_AMD64	1

#define OLD_SERVICE_NAME "DummyBecauseOldSeviceDidNotExist"

// Here is the real code.
#include "GenericInstallPnp.h"
