; extern "C" void __cdecl GetMSR(DWORD functioncode, DWORD * eax, DWORD * edx);

; THIS FILE IS NOT COMPILED UNDER VISUAL STUDIO....
; THE OBJECT IS UNDER CLEAR CASE CONTROL

_TEXT	SEGMENT 

PUBLIC	GetMSR

; function code is in rcx and that is where rdmsr expects it
; address where to put eax is in rdx
; address where to put edx is in r8
GetMSR	PROC

; save used registers
	push rbx
	push rcx
	push rdx
	push rdi
	pushfq

	mov	rdi, rdx
	rdmsr
	or	rdi,rdi
	jz	NO_EAX
	mov	DWORD PTR [rdi], eax
NO_EAX:
	or	r8,r8
	jz	NO_EDX
	mov	DWORD PTR [r8], edx
NO_EDX:


; restore used registers
	popfq
	pop rdi
	pop rdx
	pop rcx
	pop rbx

	ret	0

GetMSR	ENDP
_TEXT	ENDS
END
