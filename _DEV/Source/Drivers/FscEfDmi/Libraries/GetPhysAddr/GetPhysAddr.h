// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the GETPHYSADDR_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// GETPHYSADDR_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef GETPHYSADDR_EXPORTS
#define GETPHYSADDR_API __declspec(dllexport)
#else
#define GETPHYSADDR_API __declspec(dllimport)
#endif

#if 0
// This class is exported from the GetPhysAddr.dll
class GETPHYSADDR_API CGetPhysAddr {
public:
	CGetPhysAddr(void);
	// TODO: add your methods here.
};
#endif

extern "C" GETPHYSADDR_API BOOL __stdcall fnGetPhysAddr(void*va, ULONG *low, ULONG *high);
