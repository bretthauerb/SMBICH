; compiled from bapicall.cpp with: cl /Fa /c /O2 bapicall.cpp

	TITLE	bapicall.cpp
	.386P

; THIS FILE IS NOT COMPILED UNDER VISUAL STUDIO....
; THE OBJECT IS UNDER CLEAR CASE CONTROL

;include listing.inc
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
;	COMDAT _BapiCall
_TEXT	SEGMENT PARA USE32 PUBLIC 'CODE'
_TEXT	ENDS
FLAT	GROUP _DATA, CONST, _BSS
	ASSUME	CS: FLAT, DS: FLAT, SS: FLAT
endif
PUBLIC	_BapiCall
;	COMDAT _BapiCall
_TEXT	SEGMENT
_p$ = 8
_a$ = 12
_b$ = 20
_c$ = 28
_BapiCall PROC NEAR					; COMDAT
; File bapicall.cpp
; Line 15
	mov	eax, DWORD PTR _c$[esp-4]
	mov	ecx, DWORD PTR _b$[esp-4]
	mov	edx, DWORD PTR _a$[esp-4]
	push	eax
	push	ecx
	push	edx
	call	DWORD PTR _p$[esp+8]
	add	esp, 12					; 0000000cH
; Line 16
	ret	0
_BapiCall ENDP
_TEXT	ENDS
END
