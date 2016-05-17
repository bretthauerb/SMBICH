; extern "C" DWORD __cdecl GetCpuid(DWORD funtioncode, DWORD * eax, char * cpuid);

; THIS FILE IS NOT COMPILED UNDER VISUAL STUDIO....
; THE OBJECT IS UNDER CLEAR CASE CONTROL

	.586P

if @Version gt 510
.model FLAT
else
_TEXT	SEGMENT PARA USE32 PUBLIC 'CODE'
_TEXT	ENDS
_DATA	SEGMENT DWORD USE32 PUBLIC 'DATA'
_DATA	ENDS
CONST	SEGMENT DWORD USE32 PUBLIC 'CONST'
CONST	ENDS
_BSS	SEGMENT DWORD USE32 PUBLIC 'BSS'
_BSS	ENDS
_TLS	SEGMENT DWORD USE32 PUBLIC 'TLS'
_TLS	ENDS
FLAT	GROUP _DATA, CONST, _BSS
	ASSUME	CS: FLAT, DS: FLAT, SS: FLAT
endif

_TEXT	SEGMENT
PUBLIC	_GetCpuid
; THIS FILE IS NOT COMPILED UNDER VISUAL STUDIO....
; THE OBJECT IS UNDER CLEAR CASE CONTROL

; all parameters are on the stack
_GetCpuid	PROC NEAR
	pushad
	pushfd					; does cpu support cpuid ?
	pop	eax
	mov	ebx, eax
	xor	eax, 00200000h			; can we toggle this bit in the flags register ?
	push	eax
	popfd
	pushfd
	pop	eax
	cmp	ebx, eax
	jz	NO_CPUID_SUPPORT			; no	

	mov	eax, DWORD PTR [esp+4+8*4]		; function code
	cpuid
	mov	edi, DWORD PTR [esp+8+8*4]		; &eax
	or	edi,edi
	jz	NO_EAX
	mov	DWORD PTR [edi], eax
NO_EAX:
	mov	edi, DWORD PTR [esp+12+8*4]		; &cpuid
	or	edi,edi
	jz	NO_CPUID
	mov	DWORD PTR [edi], ebx
	mov	DWORD PTR [edi+8], ecx
	mov	DWORD PTR [edi+4], edx
NO_CPUID:
	popad
	xor eax,eax
	ret	0

NO_CPUID_SUPPORT:
	popad
	mov eax, 1
	ret	0

_GetCpuid	ENDP
_TEXT	ENDS
END
