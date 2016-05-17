#include "stddcls.h"
#include "driver.h"
#include <stdio.h>


#pragma PAGEDCODE

static VOID IoTimer(PDEVICE_OBJECT pDeviceObject, VOID*)
{
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;
	InterlockedIncrement((PLONG)&pdx->RunTime);
	// update Runtime every 5 minutes
	if (pdx->RunTime % 300 == 0)
		pdx->UpdateRuntime = TRUE;
}

#pragma PAGEDCODE

static VOID IncrementRuntimes(PDEVICE_EXTENSION pdx)
{
	// I hate string processing in kernel, so simple minded
	PCWSTR Keys[] = { L"FscHWMonCfg\\FscTime\\1", L"FscHWMonCfg\\FscTime\\2", L"FscHWMonCfg\\FscTime\\3",
					  L"FscHWMonCfg\\FscTime\\4", L"FscHWMonCfg\\FscTime\\5", L"FscHWMonCfg\\FscTime\\6", 
					  L"FscHWMonCfg\\FscTime\\7", L"FscHWMonCfg\\FscTime\\8", L"FscHWMonCfg\\FscTime\\9",
					  L"FscHWMonCfg\\FscTime\\10", L"FscHWMonCfg\\FscTime\\11", L"FscHWMonCfg\\FscTime\\12", 
					  L"FscHWMonCfg\\FscTime\\13", L"FscHWMonCfg\\FscTime\\14", L"FscHWMonCfg\\FscTime\\15", 
					  L"FscHWMonCfg\\FscTime\\16", 
	};

	KeWaitForSingleObject(&pdx->Semaphore, Executive, KernelMode, FALSE, NULL);

	for (int i=0 ; i<sizeof Keys/sizeof Keys[0]; i++)
	{
		LONG RunTime = 0;
		ULONG zero = 0;
		RTL_QUERY_REGISTRY_TABLE table[2];
		RtlZeroMemory( table, sizeof(table) );

		table[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
		table[0].Name = L"RunTime";
		table[0].EntryContext = (PVOID)&RunTime;
		table[0].DefaultType = REG_DWORD;
		table[0].DefaultData = &zero;
		table[0].DefaultLength = sizeof(RunTime);

		if (RtlQueryRegistryValues( RTL_REGISTRY_SERVICES, Keys[i], table, NULL, NULL) != STATUS_OBJECT_NAME_NOT_FOUND)
		{
			// key exists, we create the value now if it does not exist yet.
			InterlockedExchangeAdd(&RunTime, pdx->RunTime);
			RtlWriteRegistryValue(RTL_REGISTRY_SERVICES, Keys[i], L"RunTime", REG_DWORD, (PVOID)&RunTime, sizeof(RunTime));
		}
	}
	InterlockedExchange(&pdx->RunTime, 0);
	KeReleaseSemaphore(&pdx->Semaphore, 0, 1, FALSE);
}

static VOID RuntimeThread(PVOID param)
{
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)param;

	while (!pdx->StopThread)
	{
		if (pdx->UpdateRuntime)
		{
			pdx->UpdateRuntime = FALSE;
			IncrementRuntimes(pdx);
		}
		LARGE_INTEGER li; li.QuadPart = -(LONGLONG)1000000*10;	// 1 second in 100ns units
		KeDelayExecutionThread(KernelMode, FALSE, &li);
	}

	IncrementRuntimes(pdx);

	PsTerminateSystemThread(STATUS_SUCCESS);
}

#pragma PAGEDCODE

NTSTATUS StartDevice(PDEVICE_OBJECT fdo, PCM_PARTIAL_RESOURCE_LIST /*raw*/, PCM_PARTIAL_RESOURCE_LIST /*translated*/)
	{							// StartDevice
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	KeInitializeSemaphore(&pdx->Semaphore, 1, 1);

	RTL_QUERY_REGISTRY_TABLE table[3];
	RtlZeroMemory( table, sizeof(table) );

	ULONG zero = 0;
	LARGE_INTEGER lzero = {0,0};
	ULONG BootCounter;

#pragma pack(push,1)
	struct {
		LONG iSize;
		LONG iType;
		LARGE_INTEGER FirstInstallTime;
	} IamSoTired;
#pragma pack(pop)

	memset(&IamSoTired, 0, sizeof(IamSoTired)); IamSoTired.iSize = sizeof(IamSoTired); BootCounter = 0;	// make sure :-)

	table[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
	table[0].Name = L"BootCounter";
	table[0].EntryContext = &BootCounter;
	table[0].DefaultType = REG_DWORD;
	table[0].DefaultData = &zero;
	table[0].DefaultLength = sizeof(ULONG);

	table[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
	table[1].Name = L"FirstInstallTime";
	table[1].EntryContext = &IamSoTired;
	table[1].DefaultType = REG_QWORD;
	table[1].DefaultData = &lzero;
	table[1].DefaultLength = sizeof(LARGE_INTEGER);

	NTSTATUS status = RtlQueryRegistryValues( RTL_REGISTRY_SERVICES, L"FscHWMonCfg\\FscTime\\1", table, NULL, NULL );
	pdx->BootCounter = BootCounter;
	pdx->FirstInstallTime = IamSoTired.FirstInstallTime;

	BOOLEAN WriteFirstInstallTime = FALSE;
	// todo: check compatiblility if (RtlLargeIntegerEqualToZero(pdx->FirstInstallTime))
    if (pdx->FirstInstallTime.QuadPart == 0)
    {
		WriteFirstInstallTime = TRUE;
		KeQuerySystemTime(&pdx->FirstInstallTime);
	}

	// may be these do not exist yet
	if (status != STATUS_SUCCESS)
	{
		RtlCreateRegistryKey(RTL_REGISTRY_SERVICES, L"FscHWMonCfg");
		RtlCreateRegistryKey(RTL_REGISTRY_SERVICES, L"FscHWMonCfg\\FscTime");
		RtlCreateRegistryKey(RTL_REGISTRY_SERVICES, L"FscHWMonCfg\\FscTime\\1");
	}
	pdx->BootCounter++;
	RtlWriteRegistryValue(RTL_REGISTRY_SERVICES, L"FscHWMonCfg\\FscTime\\1", L"BootCounter", REG_DWORD, (PUCHAR)&pdx->BootCounter, sizeof(pdx->BootCounter));

	if (WriteFirstInstallTime)
		RtlWriteRegistryValue(RTL_REGISTRY_SERVICES, L"FscHWMonCfg\\FscTime\\1", L"FirstInstallTime", REG_QWORD, (PUCHAR)&pdx->FirstInstallTime, sizeof(pdx->FirstInstallTime));

	HANDLE h;
	PsCreateSystemThread(&h, THREAD_ALL_ACCESS, NULL, NULL, NULL, RuntimeThread, pdx);
	if (NT_ERROR(ObReferenceObjectByHandle(h, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID*)&pdx->pThread, NULL)))
	{
		KdPrint(("FscTime: ObReferenceObjectByHandle failed\n"));
		// hope it stops before we unload, otherwise BSOD
		pdx->StopThread = TRUE;
	}
	// Now that we have an object for the thread we no longer need the handle
	ZwClose(h);

	IoInitializeTimer( fdo, IoTimer, NULL);
	IoStartTimer( fdo );

	return STATUS_SUCCESS;
	}							// StartDevice

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID StopDevice(IN PDEVICE_OBJECT fdo, BOOLEAN /*oktouch = FALSE */)
	{							// StopDevice
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	LARGE_INTEGER li; li.QuadPart = -(LONGLONG)1000000*10;	// 1 second in 100ns units

	// thread handling from NT4 pardrvr.c
	if (pdx->pThread)
	{
		pdx->StopThread = TRUE;
		KeWaitForSingleObject( pdx->pThread, Executive, KernelMode, FALSE, NULL );
		ObDereferenceObject( pdx->pThread );
		pdx->pThread = NULL;
	}
	IoStopTimer( fdo );

	}							// StopDevice
