#include "FscGabi.h"
#include "Interpreter.h"
#define DRIVER_NAME					GabiServiceName
#define DRIVER_DEVICE_NAME			L"\\Device\\FSC_GABI"
#define DRIVER_LINK_NAME_L			L"\\DosDevices\\FSC_GABI"

#define MAJOR_VERSION	6
#define MINOR_VERSION	1
#define RELEASE			0
#define DRIVER_VERSION ((MAJOR_VERSION<<24) + (MINOR_VERSION<<16) + RELEASE)

#pragma warning ( disable : 4390 )

#ifdef _NTDDK_
typedef struct
{
	PUCHAR	pVirtual;
	PHYSICAL_ADDRESS	Physical;
	ULONG	ulSize;
} DriverBufferDescriptor_T;

#include "devqueue.h"

enum DEVSTATE {
	STOPPED,								// device stopped
	WORKING,								// started and working
	PENDINGSTOP,							// stop pending
	PENDINGREMOVE,							// remove pending
	SURPRISEREMOVED,						// removed by surprise
	REMOVED,								// removed
	};

typedef struct _DEVICE_EXTENSION
{
	PDRIVER_OBJECT DriverObject;
	PDEVICE_OBJECT DeviceObject;			// device object this extension belongs to
	PDEVICE_OBJECT LowerDeviceObject;		// next lower driver in same stack
	PDEVICE_OBJECT Pdo;						// the PDO
	IO_REMOVE_LOCK RemoveLock;				// removal control locking structure
	UNICODE_STRING devname;
	UNICODE_STRING uniSymbolicLinkName;
	DEVSTATE state;							// current state of device
	DEVSTATE prevstate;						// state prior to removal query
	DEVICE_POWER_STATE devpower;			// current device power state
	SYSTEM_POWER_STATE syspower;			// current system power state
	DEVICE_CAPABILITIES devcaps;			// copy of most recent device capabilities
	DEVQUEUE dqReadWrite;					// queue for reads and writes
	LONG handles;							// # open handles
//	PKINTERRUPT InterruptObject;			// address of interrupt object
//	PUCHAR portbase;						// I/O port base address
//	ULONG nports;							// number of assigned ports
	BOOLEAN StalledForPower;

	// Additional per-device declarations
	KMUTEX				Mutex;
	PUCHAR				MappedBios;
	DriverBufferDescriptor_T ControlBuffer;
	DriverBufferDescriptor_T InBuffer;
	DriverBufferDescriptor_T OutBuffer;
	ULONG				ulGabiVersion;
	ULONG				ulMaxSegmentSize;	// For testing
	ULONG				ulCodeIntegrityCheck;
	ULONG				ulUseACPI;
	PVOID				pvNotificationEntry;
	PFILE_OBJECT		ACPIFileObject;
	PDEVICE_OBJECT		ACPIDevice;

	void (*GabiCallAddress)(PHYSICAL_ADDRESS,PHYSICAL_ADDRESS,PHYSICAL_ADDRESS);
	psInterpreterContext pInterpreterContext;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;
///////////////////////////////////////////////////////////////////////////////
// Global functions
VOID StartIo(IN PDEVICE_OBJECT fdo, PIRP Irp);
VOID RemoveDevice(IN PDEVICE_OBJECT fdo);
NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status, IN ULONG_PTR info);
NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status);
NTSTATUS ForwardAndWait(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
VOID SendAsyncNotification(PVOID context);
VOID EnableAllInterfaces(PDEVICE_EXTENSION pdx, BOOLEAN enable);
VOID DeregisterAllInterfaces(PDEVICE_EXTENSION pdx);
NTSTATUS StartDevice(PDEVICE_OBJECT fdo, PCM_PARTIAL_RESOURCE_LIST raw, PCM_PARTIAL_RESOURCE_LIST translated);
VOID StopDevice(PDEVICE_OBJECT fdo, BOOLEAN oktouch = FALSE);
VOID UnstallQueue(PDEVICE_OBJECT fdo);
BOOLEAN StallQueueAndNotify(PDEVICE_OBJECT, PVOID, VOID (*)(PVOID));
BOOLEAN CheckQueueStalled( PDEVICE_OBJECT fdo );
NTSTATUS SendDeviceSetPower(PDEVICE_EXTENSION fdo, DEVICE_POWER_STATE state, BOOLEAN wait = FALSE);
// I/O request handlers

NTSTATUS DispatchCreate(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchClose(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchControl(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchCleanup(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchPower(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchPnp(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchSystemControl(PDEVICE_OBJECT fdo, PIRP Irp);

//extern UNICODE_STRING servkey;

NTSTATUS DriverIOCTL(PDEVICE_OBJECT fdo, PIRP Irp);


#endif