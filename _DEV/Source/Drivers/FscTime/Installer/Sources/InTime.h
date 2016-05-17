// DO NOT INCLUDE THIS FILE IN THE DLL SOURCES.

#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllimport) UINT Install(MSIHANDLE hInstall);
__declspec(dllimport) UINT UnInstall(MSIHANDLE hInstall);

#ifdef __cplusplus
}
#endif
