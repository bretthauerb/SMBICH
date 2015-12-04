// GabiAcpiTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>

typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;

#include <initguid.h>
#include <Strsafe.h>
#include <Setupapi.h>
#include "AcpiGabi.h"

BOOL GetDeviceHandle(GUID guidDeviceInterface, PHANDLE hDeviceHandle)
{
	if (guidDeviceInterface == GUID_NULL)
	{
		return FALSE;
	}

	BOOL bResult = TRUE;
	HDEVINFO hDeviceInfo;
	SP_DEVINFO_DATA DeviceInfoData;

	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA pInterfaceDetailData = NULL;

	ULONG requiredLength = 0;
	LPTSTR lpDevicePath = NULL;
	DWORD index = 0;

	// Get information about all the installed devices for the specified
	// device interface class.
	hDeviceInfo = SetupDiGetClassDevs(&guidDeviceInterface, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	if (hDeviceInfo == INVALID_HANDLE_VALUE)
	{
		// ERROR 
		return FALSE;
	}

	//Enumerate all the device interfaces in the device information set.
	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	for (index = 0; SetupDiEnumDeviceInfo(hDeviceInfo, index, &DeviceInfoData); index++)
	{
		//Reset for this iteration
		if (lpDevicePath)
		{
			LocalFree(lpDevicePath);
		}

		if (pInterfaceDetailData)
		{
			LocalFree(pInterfaceDetailData);
		}

		deviceInterfaceData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);

		//Get information about the device interface.
		bResult = SetupDiEnumDeviceInterfaces(hDeviceInfo, &DeviceInfoData, &guidDeviceInterface, 0, &deviceInterfaceData);

		// Check if last item
		if (GetLastError() == ERROR_NO_MORE_ITEMS)
		{
			break;
		}

		//Check for some other error
		if (!bResult)
		{
			break;
		}

		//Interface data is returned in SP_DEVICE_INTERFACE_DETAIL_DATA
		//which we need to allocate, so we have to call this function twice.
		//First to get the size so that we know how much to allocate
		//Second, the actual call with the allocated buffer	
		bResult = SetupDiGetDeviceInterfaceDetail(hDeviceInfo, &deviceInterfaceData, NULL, 0, &requiredLength, NULL);

		//Check for some other error
		if (!bResult)
		{
			if ((ERROR_INSUFFICIENT_BUFFER == GetLastError()) && (requiredLength > 0))
			{
				//we got the size, allocate buffer
				pInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR, requiredLength);

				if (!pInterfaceDetailData)
				{
					// ERROR 
					bResult = FALSE;
				}
			}
			else
			{
				bResult = FALSE;
			}
		}

		//get the interface detailed data
		pInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		//Now call it with the correct size and allocated buffer
		bResult = SetupDiGetDeviceInterfaceDetail(hDeviceInfo, &deviceInterfaceData, pInterfaceDetailData, requiredLength, NULL, &DeviceInfoData);

		//Check for some other error
		if (!bResult)
		{
			break;
		}

		//copy device path			
		size_t nLength = wcslen(pInterfaceDetailData->DevicePath) + 1;
		lpDevicePath = (TCHAR *)LocalAlloc(LPTR, nLength * sizeof(TCHAR));
		StringCchCopy(lpDevicePath, nLength, pInterfaceDetailData->DevicePath);
		lpDevicePath[nLength - 1] = 0;
	}

	if (bResult)
	{
		if (!lpDevicePath)
		{
			//Error.
			bResult = FALSE;
		}

		//Open the device
		*hDeviceHandle = CreateFile(lpDevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

		if (*hDeviceHandle == INVALID_HANDLE_VALUE)
		{
			DWORD err = GetLastError();

			//Error.
			bResult = FALSE;
		}
	}

	LocalFree(lpDevicePath);
	LocalFree(pInterfaceDetailData);
	BOOL bTmp = SetupDiDestroyDeviceInfoList(hDeviceInfo);

	if (bResult)
	{
		bResult = bTmp;
	}

	return bResult;
}

BOOL GabiCmd(BYTE* pControlBuffer, ULONG ulControlBufferLength, BYTE* pRequestBuffer, ULONG ulRequestBufferLength, BYTE* pResponseBuffer, ULONG ulResponseBufferLength)
{
	HANDLE handle;

	if (GetDeviceHandle(GUID_DEVINTERFACE_GabiAcpi, &handle))
	{
		GabiAcpiCmd cmd;
		DWORD junk;

		memset(&cmd, 0, sizeof(cmd));		

		cmd.FunctionIndex = 1;
		memcpy(&cmd.ControlBuffer, &pControlBuffer, sizeof(pControlBuffer));
		cmd.ControlBufferLen = ulControlBufferLength;
		memcpy(&cmd.RequestBuffer, &pRequestBuffer, sizeof(pRequestBuffer));
		cmd.RequestBufferLen = ulRequestBufferLength;
		memcpy(&cmd.ResponseBuffer, &pResponseBuffer, sizeof(pResponseBuffer));
		cmd.ResponseBufferLen = ulResponseBufferLength;

		BOOL ret = DeviceIoControl(handle, IOCTL_GABI_ACPI_CMD, &cmd, sizeof(cmd), NULL, 0, &junk, (LPOVERLAPPED)NULL);

		CloseHandle(handle);
		return ret;
	}
	else
	{
		printf("no driver\r\n");
	}

	return FALSE;
}

int main()
{
	BYTE controlBuffer[1024];
	BYTE requestBuffer[1024];
	BYTE responseBuffer[1024];

	memset(controlBuffer, 0, sizeof(controlBuffer));
	memset(requestBuffer, 0, sizeof(requestBuffer));
	memset(responseBuffer, 0, sizeof(responseBuffer));

	if (!GabiCmd(controlBuffer, sizeof(controlBuffer), requestBuffer, sizeof(requestBuffer), responseBuffer, sizeof(responseBuffer)))
	{
		printf("GabiCmd failed\r\n");
	}

	return 0;
}

