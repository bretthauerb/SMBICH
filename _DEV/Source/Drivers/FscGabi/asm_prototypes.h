#ifdef __cplusplus
extern "C" {
#endif

#if defined(_AMD64_)
#define CALLING_CONVENTION __fastcall
#else
#define CALLING_CONVENTION __cdecl
#endif

	/*
	 * Returns 0 when cpuid instruction is supported, !=0 when not.
	 * functioncode is contents of eax before the cpuid instruction.
	 * The contents of eax after the cpuid instruction is written to *eax. The pointer may be NULL.
	 * The cpuid string that is in ebx, edx, ecx after the cpuid instruction is written to *cpuid (12 bytes).
	 * The pointer may be NULL.
	 */
	int  CALLING_CONVENTION GetCpuid(ULONG functioncode, ULONG * eax, char * cpuid);
	/*
	 * functioncode is the contents of ecx before the rdmsr instruction.
	 * The contents of eax and edx after the rdmsr instruction are written to *eax and *edx. Both may be NULL.
	 */
	void CALLING_CONVENTION GetMSR(ULONG functioncode, ULONG * eax, ULONG * edx);

	/*
	 * See BAPI spec ....
	 */
	void CALLING_CONVENTION BapiCall(void (*BapiCallAddress)(PHYSICAL_ADDRESS, PHYSICAL_ADDRESS, PHYSICAL_ADDRESS),
						  PHYSICAL_ADDRESS input_buffer,
						  PHYSICAL_ADDRESS output_buffer,
						  PHYSICAL_ADDRESS control_buffer); 
#ifdef __cplusplus
}
#endif
