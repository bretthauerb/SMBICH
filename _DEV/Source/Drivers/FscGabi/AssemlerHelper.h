#ifdef __cplusplus
extern "C" {
#endif

#if defined(_AMD64_)
#define CALLING_CONVENTION __fastcall
#else
#define CALLING_CONVENTION __cdecl
#endif

	void CALLING_CONVENTION CallOUT(PHYSICAL_ADDRESS stack, psInterpreterCPUState state, ULONGLONG outLocation);
	void CALLING_CONVENTION CallIN(PHYSICAL_ADDRESS stack, psInterpreterCPUState state, ULONGLONG outLocation);

#ifdef __cplusplus
}
#endif
