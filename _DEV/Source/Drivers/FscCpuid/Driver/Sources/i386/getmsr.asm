; extern "C" void __cdecl GetMSR(DWORD functioncode, DWORD * eax, DWORD * edx);

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

PUBLIC	_GetMSR
_TEXT	SEGMENT
; all parameters are on stack
_GetMSR	PROC NEAR
	pushad
	mov	ecx, DWORD PTR [esp+4+8*4]	; functioncode
	rdmsr
	mov	edi, DWORD PTR [esp+8+8*4]	; &eax
	or	edi,edi
	jz	NO_EAX
	mov	DWORD PTR [edi], eax
NO_EAX:	mov	edi, DWORD PTR [esp+12+8*4]	; @edx
	or	edi,edi
	jz	NO_EDX
	mov	DWORD PTR [edi], edx
NO_EDX:	popad
	ret	0

_GetMSR	ENDP
_TEXT	ENDS
END
