; extern "C" void __cdecl GetCpuid(DWORD funtioncode, DWORD * eax, char * cpuid);

; THIS FILE IS NOT COMPILED UNDER VISUAL STUDIO....
; THE OBJECT IS UNDER CLEAR CASE CONTROL

_TEXT	SEGMENT

PUBLIC	GetCpuid

; function code is in rcx
; address where to put eax is in rdx
; address where to put 12 byte id string is in r8
GetCpuid	PROC

; save used registers
	push rbx
	push rcx
	push rdx
	push rdi
	push r8
	pushfq

; does cpu support cpuid ?
	pushfq
	pop	rax
	mov	rbx, rax
	xor	rax, 00200000h			; can we toggle this bit in the flags register ?
	push	rax
	popfq
	pushfq
	pop	rax
	cmp	rbx, rax
	jz	CPU_NO_CPUID				; no	

; do the real work ()
	push rdi
	mov	eax, ecx				; function code
	mov	rdi, rdx				; save rdx for later
	cpuid

	or	rdi,rdi
	jz	NO_EAX
	mov	DWORD PTR [rdi], eax
NO_EAX:
	or	r8,r8
	jz	NO_CPUID
	mov	DWORD PTR [r8], ebx
	mov	DWORD PTR [r8+8], ecx
	mov	DWORD PTR [r8+4], edx

NO_CPUID:
	pop	rdi
	xor eax,eax

; restore used registers
	popfq
	pop r8
	pop rdi
	pop rdx
	pop rcx
	pop rbx

	ret	0

CPU_NO_CPUID:
	mov eax, 1

; restore used registers
	popfq
	pop r8
	pop rdi
	pop rdx
	pop rcx
	pop rbx

	ret	0

GetCpuid	ENDP
_TEXT	ENDS
END
