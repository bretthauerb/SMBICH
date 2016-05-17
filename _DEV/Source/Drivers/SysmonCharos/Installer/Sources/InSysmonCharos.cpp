// InHWMD2568.cpp
//

#include "stdafx.h"
#include <tchar.h>
#include <Msiquery.h>

#include "..\..\Driver\Sources\driver.h"
#include "FSCIoctl.h"

#define ROOT_DEVICE_ID "ROOT\\Charos"		
#define SHARE_NAME "SysmonCharos.sys"
#define DLL_NAME "InSysmonCharos.dll"
#define INF_NAME "SysmonCharos.inf"
#define DEVICE_NAME "\\\\.\\CHAROS"

// No version probing, rely on Microsoft :-)
#define NO_VERSION_CHECK_I386	1
#define NO_VERSION_CHECK_AMD64	1

#define OLD_SERVICE_NAME "DummyBecauseOldSeviceDidNotExist"

// Here is the real code.
#include "GenericInstallPnp.h"
