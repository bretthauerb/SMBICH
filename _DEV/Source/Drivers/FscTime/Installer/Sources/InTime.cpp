// InTime.cpp
//

#include "stdafx.h"
#include <tchar.h>
#include <Msiquery.h>

#include "..\..\Driver\Sources\driver.h"
#include "FSCIoctl.h"

#define ROOT_DEVICE_ID "ROOT\\FscTime"		
#define SHARE_NAME "FscTime.sys"
#define DLL_NAME "InTime.dll"
#define INF_NAME "FscTime.inf"
#define DEVICE_NAME "\\\\.\\FSC_TIME"

// Only used to remove the old service installed with CreateService
#define OLD_SERVICE_NAME "FscTime"

// No version probing, rely on Microsoft :-)
#define NO_VERSION_CHECK_I386	1
#define NO_VERSION_CHECK_AMD64	1

// Here is the real code.
#include "GenericInstallPnp.h"

#if 0

// "Blah.sys"
#define SERVICE_IMAGE FscTimeDriverImage
// "Blah"
#define OLD_SERVICE_NAME FscTimeServiceName
// "\\\\.\\Blah"
#define DRIVER_NAME FscTimeDriverName
// Here is the real code.
#include "Imports\Includes\GenericInstall.h"
#endif