_TEXT	SEGMENT

PUBLIC	CallOUT

CallOUT	PROC
; new stack location is in rcx
; Context is in rdx
; out parameter is in r8
	push rdx
	push rcx

	;routine is not reentrant, so lock it
	lea rdx, CallOUTLock	
	xor ecx, ecx
	inc ecx

@@:		
	xor eax, eax
	lock cmpxchg [rdx], ecx
	jz @f

	pause
	jmp @b
	
@@:

	pop rcx
	lea rdx, CallDxLocation	
	mov [rdx], r8

	lea rdx, SaveCallOUTContext
	pop rax
	mov [rdx], rax		

	lea rdx, SaveCallOUTContextSP
	mov [rdx], rsp
	mov rdx, rax

	;set virtual processore stack
	mov rsp, rcx
	
	;set virtual processore register	
	;swap flags
	pushfq
	pop rax
	xchg  rax, [rdx +  1 * 8]
	push rax
	popfq

	;swap registers
	xchg rax, [rdx +  2 * 8]
	xchg rbx, [rdx +  3 * 8]
	xchg rcx, [rdx +  4 * 8]
	xchg rsi, [rdx +  6 * 8]
	xchg rdi, [rdx +  7 * 8]
	xchg rbp, [rdx +  8 * 8]
	xchg r8,  [rdx + 10 * 8]
	xchg r9,  [rdx + 11 * 8]
	xchg r10, [rdx + 12 * 8]
	xchg r11, [rdx + 13 * 8]
	xchg r12, [rdx + 14 * 8]
	xchg r13, [rdx + 15 * 8]
	xchg r14, [rdx + 16 * 8]
	xchg r15, [rdx + 17 * 8]
	;rdx last
	xchg rdx, [rdx +  5 * 8]

	;out dx parameter decision
	push rdx
	pushfq

	lea rdx, CallDxLocation
	mov rdx, [rdx]	
	cmp edx, 0FFFFFFFFh
	je outDefault

	;offset calculation based on parameter to jmp to right out
	push rax
	movzx rax, dl
	mov rdx, 9  ; popfq, pop, out and jmp  => 9 Bytes
	mul dx	
	lea rdx, outIm
	add rdx, rax
	pop rax
	jmp rdx

	;SMI trap
outIm:
	popfq
	pop rdx
	out 0h, al
	jmp outwait
	popfq
	pop rdx
	out 1h, al
	jmp outwait
	popfq
	pop rdx
	out 2h, al
	jmp outwait
	popfq
	pop rdx
	out 3h, al
	jmp outwait
	popfq
	pop rdx
	out 4h, al
	jmp outwait
	popfq
	pop rdx
	out 5h, al
	jmp outwait
	popfq
	pop rdx
	out 6h, al
	jmp outwait
	popfq
	pop rdx
	out 7h, al
	jmp outwait
	popfq
	pop rdx
	out 8h, al
	jmp outwait
	popfq
	pop rdx
	out 9h, al
	jmp outwait
	popfq
	pop rdx
	out 0ah, al
	jmp outwait
	popfq
	pop rdx
	out 0bh, al
	jmp outwait
	popfq
	pop rdx
	out 0ch, al
	jmp outwait
	popfq
	pop rdx
	out 0dh, al
	jmp outwait
	popfq
	pop rdx
	out 0eh, al
	jmp outwait
	popfq
	pop rdx
	out 0fh, al
	jmp outwait
	popfq
	pop rdx
	out 10h, al
	jmp outwait
	popfq
	pop rdx
	out 11h, al
	jmp outwait
	popfq
	pop rdx
	out 12h, al
	jmp outwait
	popfq
	pop rdx
	out 13h, al
	jmp outwait
	popfq
	pop rdx
	out 14h, al
	jmp outwait
	popfq
	pop rdx
	out 15h, al
	jmp outwait
	popfq
	pop rdx
	out 16h, al
	jmp outwait
	popfq
	pop rdx
	out 17h, al
	jmp outwait
	popfq
	pop rdx
	out 18h, al
	jmp outwait
	popfq
	pop rdx
	out 19h, al
	jmp outwait
	popfq
	pop rdx
	out 1ah, al
	jmp outwait
	popfq
	pop rdx
	out 1bh, al
	jmp outwait
	popfq
	pop rdx
	out 1ch, al
	jmp outwait
	popfq
	pop rdx
	out 1dh, al
	jmp outwait
	popfq
	pop rdx
	out 1eh, al
	jmp outwait
	popfq
	pop rdx
	out 1fh, al
	jmp outwait
	popfq
	pop rdx
	out 20h, al
	jmp outwait
	popfq
	pop rdx
	out 21h, al
	jmp outwait
	popfq
	pop rdx
	out 22h, al
	jmp outwait
	popfq
	pop rdx
	out 23h, al
	jmp outwait
	popfq
	pop rdx
	out 24h, al
	jmp outwait
	popfq
	pop rdx
	out 25h, al
	jmp outwait
	popfq
	pop rdx
	out 26h, al
	jmp outwait
	popfq
	pop rdx
	out 27h, al
	jmp outwait
	popfq
	pop rdx
	out 28h, al
	jmp outwait
	popfq
	pop rdx
	out 29h, al
	jmp outwait
	popfq
	pop rdx
	out 2ah, al
	jmp outwait
	popfq
	pop rdx
	out 2bh, al
	jmp outwait
	popfq
	pop rdx
	out 2ch, al
	jmp outwait
	popfq
	pop rdx
	out 2dh, al
	jmp outwait
	popfq
	pop rdx
	out 2eh, al
	jmp outwait
	popfq
	pop rdx
	out 2fh, al
	jmp outwait
	popfq
	pop rdx
	out 30h, al
	jmp outwait
	popfq
	pop rdx
	out 31h, al
	jmp outwait
	popfq
	pop rdx
	out 32h, al
	jmp outwait
	popfq
	pop rdx
	out 33h, al
	jmp outwait
	popfq
	pop rdx
	out 34h, al
	jmp outwait
	popfq
	pop rdx
	out 35h, al
	jmp outwait
	popfq
	pop rdx
	out 36h, al
	jmp outwait
	popfq
	pop rdx
	out 37h, al
	jmp outwait
	popfq
	pop rdx
	out 38h, al
	jmp outwait
	popfq
	pop rdx
	out 39h, al
	jmp outwait
	popfq
	pop rdx
	out 3ah, al
	jmp outwait
	popfq
	pop rdx
	out 3bh, al
	jmp outwait
	popfq
	pop rdx
	out 3ch, al
	jmp outwait
	popfq
	pop rdx
	out 3dh, al
	jmp outwait
	popfq
	pop rdx
	out 3eh, al
	jmp outwait
	popfq
	pop rdx
	out 3fh, al
	jmp outwait
	popfq
	pop rdx
	out 40h, al
	jmp outwait
	popfq
	pop rdx
	out 41h, al
	jmp outwait
	popfq
	pop rdx
	out 42h, al
	jmp outwait
	popfq
	pop rdx
	out 43h, al
	jmp outwait
	popfq
	pop rdx
	out 44h, al
	jmp outwait
	popfq
	pop rdx
	out 45h, al
	jmp outwait
	popfq
	pop rdx
	out 46h, al
	jmp outwait
	popfq
	pop rdx
	out 47h, al
	jmp outwait
	popfq
	pop rdx
	out 48h, al
	jmp outwait
	popfq
	pop rdx
	out 49h, al
	jmp outwait
	popfq
	pop rdx
	out 4ah, al
	jmp outwait
	popfq
	pop rdx
	out 4bh, al
	jmp outwait
	popfq
	pop rdx
	out 4ch, al
	jmp outwait
	popfq
	pop rdx
	out 4dh, al
	jmp outwait
	popfq
	pop rdx
	out 4eh, al
	jmp outwait
	popfq
	pop rdx
	out 4fh, al
	jmp outwait
	popfq
	pop rdx
	out 50h, al
	jmp outwait
	popfq
	pop rdx
	out 51h, al
	jmp outwait
	popfq
	pop rdx
	out 52h, al
	jmp outwait
	popfq
	pop rdx
	out 53h, al
	jmp outwait
	popfq
	pop rdx
	out 54h, al
	jmp outwait
	popfq
	pop rdx
	out 55h, al
	jmp outwait
	popfq
	pop rdx
	out 56h, al
	jmp outwait
	popfq
	pop rdx
	out 57h, al
	jmp outwait
	popfq
	pop rdx
	out 58h, al
	jmp outwait
	popfq
	pop rdx
	out 59h, al
	jmp outwait
	popfq
	pop rdx
	out 5ah, al
	jmp outwait
	popfq
	pop rdx
	out 5bh, al
	jmp outwait
	popfq
	pop rdx
	out 5ch, al
	jmp outwait
	popfq
	pop rdx
	out 5dh, al
	jmp outwait
	popfq
	pop rdx
	out 5eh, al
	jmp outwait
	popfq
	pop rdx
	out 5fh, al
	jmp outwait
	popfq
	pop rdx
	out 60h, al
	jmp outwait
	popfq
	pop rdx
	out 61h, al
	jmp outwait
	popfq
	pop rdx
	out 62h, al
	jmp outwait
	popfq
	pop rdx
	out 63h, al
	jmp outwait
	popfq
	pop rdx
	out 64h, al
	jmp outwait
	popfq
	pop rdx
	out 65h, al
	jmp outwait
	popfq
	pop rdx
	out 66h, al
	jmp outwait
	popfq
	pop rdx
	out 67h, al
	jmp outwait
	popfq
	pop rdx
	out 68h, al
	jmp outwait
	popfq
	pop rdx
	out 69h, al
	jmp outwait
	popfq
	pop rdx
	out 6ah, al
	jmp outwait
	popfq
	pop rdx
	out 6bh, al
	jmp outwait
	popfq
	pop rdx
	out 6ch, al
	jmp outwait
	popfq
	pop rdx
	out 6dh, al
	jmp outwait
	popfq
	pop rdx
	out 6eh, al
	jmp outwait
	popfq
	pop rdx
	out 6fh, al
	jmp outwait
	popfq
	pop rdx
	out 70h, al
	jmp outwait
	popfq
	pop rdx
	out 71h, al
	jmp outwait
	popfq
	pop rdx
	out 72h, al
	jmp outwait
	popfq
	pop rdx
	out 73h, al
	jmp outwait
	popfq
	pop rdx
	out 74h, al
	jmp outwait
	popfq
	pop rdx
	out 75h, al
	jmp outwait
	popfq
	pop rdx
	out 76h, al
	jmp outwait
	popfq
	pop rdx
	out 77h, al
	jmp outwait
	popfq
	pop rdx
	out 78h, al
	jmp outwait
	popfq
	pop rdx
	out 79h, al
	jmp outwait
	popfq
	pop rdx
	out 7ah, al
	jmp outwait
	popfq
	pop rdx
	out 7bh, al
	jmp outwait
	popfq
	pop rdx
	out 7ch, al
	jmp outwait
	popfq
	pop rdx
	out 7dh, al
	jmp outwait
	popfq
	pop rdx
	out 7eh, al
	jmp outwait
	popfq
	pop rdx
	out 7fh, al
	jmp outwait
	popfq
	pop rdx
	out 80h, al
	jmp outwait
	popfq
	pop rdx
	out 81h, al
	jmp outwait
	popfq
	pop rdx
	out 82h, al
	jmp outwait
	popfq
	pop rdx
	out 83h, al
	jmp outwait
	popfq
	pop rdx
	out 84h, al
	jmp outwait
	popfq
	pop rdx
	out 85h, al
	jmp outwait
	popfq
	pop rdx
	out 86h, al
	jmp outwait
	popfq
	pop rdx
	out 87h, al
	jmp outwait
	popfq
	pop rdx
	out 88h, al
	jmp outwait
	popfq
	pop rdx
	out 89h, al
	jmp outwait
	popfq
	pop rdx
	out 8ah, al
	jmp outwait
	popfq
	pop rdx
	out 8bh, al
	jmp outwait
	popfq
	pop rdx
	out 8ch, al
	jmp outwait
	popfq
	pop rdx
	out 8dh, al
	jmp outwait
	popfq
	pop rdx
	out 8eh, al
	jmp outwait
	popfq
	pop rdx
	out 8fh, al
	jmp outwait
	popfq
	pop rdx
	out 90h, al
	jmp outwait
	popfq
	pop rdx
	out 91h, al
	jmp outwait
	popfq
	pop rdx
	out 92h, al
	jmp outwait
	popfq
	pop rdx
	out 93h, al
	jmp outwait
	popfq
	pop rdx
	out 94h, al
	jmp outwait
	popfq
	pop rdx
	out 95h, al
	jmp outwait
	popfq
	pop rdx
	out 96h, al
	jmp outwait
	popfq
	pop rdx
	out 97h, al
	jmp outwait
	popfq
	pop rdx
	out 98h, al
	jmp outwait
	popfq
	pop rdx
	out 99h, al
	jmp outwait
	popfq
	pop rdx
	out 9ah, al
	jmp outwait
	popfq
	pop rdx
	out 9bh, al
	jmp outwait
	popfq
	pop rdx
	out 9ch, al
	jmp outwait
	popfq
	pop rdx
	out 9dh, al
	jmp outwait
	popfq
	pop rdx
	out 9eh, al
	jmp outwait
	popfq
	pop rdx
	out 9fh, al
	jmp outwait
	popfq
	pop rdx
	out 0a0h, al
	jmp outwait
	popfq
	pop rdx
	out 0a1h, al
	jmp outwait
	popfq
	pop rdx
	out 0a2h, al
	jmp outwait
	popfq
	pop rdx
	out 0a3h, al
	jmp outwait
	popfq
	pop rdx
	out 0a4h, al
	jmp outwait
	popfq
	pop rdx
	out 0a5h, al
	jmp outwait
	popfq
	pop rdx
	out 0a6h, al
	jmp outwait
	popfq
	pop rdx
	out 0a7h, al
	jmp outwait
	popfq
	pop rdx
	out 0a8h, al
	jmp outwait
	popfq
	pop rdx
	out 0a9h, al
	jmp outwait
	popfq
	pop rdx
	out 0aah, al
	jmp outwait
	popfq
	pop rdx
	out 0abh, al
	jmp outwait
	popfq
	pop rdx
	out 0ach, al
	jmp outwait
	popfq
	pop rdx
	out 0adh, al
	jmp outwait
	popfq
	pop rdx
	out 0aeh, al
	jmp outwait
	popfq
	pop rdx
	out 0afh, al
	jmp outwait
	popfq
	pop rdx
	out 0b0h, al
	jmp outwait
	popfq
	pop rdx
	out 0b1h, al
	jmp outwait
	popfq
	pop rdx
	out 0b2h, al
	jmp outwait
	popfq
	pop rdx
	out 0b3h, al
	jmp outwait
	popfq
	pop rdx
	out 0b4h, al
	jmp outwait
	popfq
	pop rdx
	out 0b5h, al
	jmp outwait
	popfq
	pop rdx
	out 0b6h, al
	jmp outwait
	popfq
	pop rdx
	out 0b7h, al
	jmp outwait
	popfq
	pop rdx
	out 0b8h, al
	jmp outwait
	popfq
	pop rdx
	out 0b9h, al
	jmp outwait
	popfq
	pop rdx
	out 0bah, al
	jmp outwait
	popfq
	pop rdx
	out 0bbh, al
	jmp outwait
	popfq
	pop rdx
	out 0bch, al
	jmp outwait
	popfq
	pop rdx
	out 0bdh, al
	jmp outwait
	popfq
	pop rdx
	out 0beh, al
	jmp outwait
	popfq
	pop rdx
	out 0bfh, al
	jmp outwait
	popfq
	pop rdx
	out 0c0h, al
	jmp outwait
	popfq
	pop rdx
	out 0c1h, al
	jmp outwait
	popfq
	pop rdx
	out 0c2h, al
	jmp outwait
	popfq
	pop rdx
	out 0c3h, al
	jmp outwait
	popfq
	pop rdx
	out 0c4h, al
	jmp outwait
	popfq
	pop rdx
	out 0c5h, al
	jmp outwait
	popfq
	pop rdx
	out 0c6h, al
	jmp outwait
	popfq
	pop rdx
	out 0c7h, al
	jmp outwait
	popfq
	pop rdx
	out 0c8h, al
	jmp outwait
	popfq
	pop rdx
	out 0c9h, al
	jmp outwait
	popfq
	pop rdx
	out 0cah, al
	jmp outwait
	popfq
	pop rdx
	out 0cbh, al
	jmp outwait
	popfq
	pop rdx
	out 0cch, al
	jmp outwait
	popfq
	pop rdx
	out 0cdh, al
	jmp outwait
	popfq
	pop rdx
	out 0ceh, al
	jmp outwait
	popfq
	pop rdx
	out 0cfh, al
	jmp outwait
	popfq
	pop rdx
	out 0d0h, al
	jmp outwait
	popfq
	pop rdx
	out 0d1h, al
	jmp outwait
	popfq
	pop rdx
	out 0d2h, al
	jmp outwait
	popfq
	pop rdx
	out 0d3h, al
	jmp outwait
	popfq
	pop rdx
	out 0d4h, al
	jmp outwait
	popfq
	pop rdx
	out 0d5h, al
	jmp outwait
	popfq
	pop rdx
	out 0d6h, al
	jmp outwait
	popfq
	pop rdx
	out 0d7h, al
	jmp outwait
	popfq
	pop rdx
	out 0d8h, al
	jmp outwait
	popfq
	pop rdx
	out 0d9h, al
	jmp outwait
	popfq
	pop rdx
	out 0dah, al
	jmp outwait
	popfq
	pop rdx
	out 0dbh, al
	jmp outwait
	popfq
	pop rdx
	out 0dch, al
	jmp outwait
	popfq
	pop rdx
	out 0ddh, al
	jmp outwait
	popfq
	pop rdx
	out 0deh, al
	jmp outwait
	popfq
	pop rdx
	out 0dfh, al
	jmp outwait
	popfq
	pop rdx
	out 0e0h, al
	jmp outwait
	popfq
	pop rdx
	out 0e1h, al
	jmp outwait
	popfq
	pop rdx
	out 0e2h, al
	jmp outwait
	popfq
	pop rdx
	out 0e3h, al
	jmp outwait
	popfq
	pop rdx
	out 0e4h, al
	jmp outwait
	popfq
	pop rdx
	out 0e5h, al
	jmp outwait
	popfq
	pop rdx
	out 0e6h, al
	jmp outwait
	popfq
	pop rdx
	out 0e7h, al
	jmp outwait
	popfq
	pop rdx
	out 0e8h, al
	jmp outwait
	popfq
	pop rdx
	out 0e9h, al
	jmp outwait
	popfq
	pop rdx
	out 0eah, al
	jmp outwait
	popfq
	pop rdx
	out 0ebh, al
	jmp outwait
	popfq
	pop rdx
	out 0ech, al
	jmp outwait
	popfq
	pop rdx
	out 0edh, al
	jmp outwait
	popfq
	pop rdx
	out 0eeh, al
	jmp outwait
	popfq
	pop rdx
	out 0efh, al
	jmp outwait
	popfq
	pop rdx
	out 0f0h, al
	jmp outwait
	popfq
	pop rdx
	out 0f1h, al
	jmp outwait
	popfq
	pop rdx
	out 0f2h, al
	jmp outwait
	popfq
	pop rdx
	out 0f3h, al
	jmp outwait
	popfq
	pop rdx
	out 0f4h, al
	jmp outwait
	popfq
	pop rdx
	out 0f5h, al
	jmp outwait
	popfq
	pop rdx
	out 0f6h, al
	jmp outwait
	popfq
	pop rdx
	out 0f7h, al
	jmp outwait
	popfq
	pop rdx
	out 0f8h, al
	jmp outwait
	popfq
	pop rdx
	out 0f9h, al
	jmp outwait
	popfq
	pop rdx
	out 0fah, al
	jmp outwait
	popfq
	pop rdx
	out 0fbh, al
	jmp outwait
	popfq
	pop rdx
	out 0fch, al
	jmp outwait
	popfq
	pop rdx
	out 0fdh, al
	jmp outwait
	popfq
	pop rdx
	out 0feh, al
	jmp outwait
	popfq
	pop rdx
	out 0ffh, al
	jmp outwait

	; dummy nop. Will never be executed => force all jmps above to 16bit
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	
outDefault:
	popfq
	pop rdx
	out dx, al

	;force second trap to force completion of first
outWait:
	out 0EDh, al

	;save virtual processore register	
	;save rcx and get context pointer
	push rcx
	lea rcx, SaveCallOUTContext
	mov rcx, [rcx]
	pop QWORD PTR [rcx +  4 * 8]

	;swap flags
	push rax
	pushfq
	pop rax
	xchg  rax, [rcx +  1 * 8]
	push rax
	popfq
	pop rax

	;swap registers
	xchg rax, [rcx +  2 * 8]
	xchg rbx, [rcx +  3 * 8]
	xchg rdx, [rcx +  5 * 8]
	xchg rsi, [rcx +  6 * 8]
	xchg rdi, [rcx +  7 * 8]
	xchg rbp, [rcx +  8 * 8]
	xchg r8,  [rcx + 10 * 8]
	xchg r9,  [rcx + 11 * 8]
	xchg r10, [rcx + 12 * 8]
	xchg r11, [rcx + 13 * 8]
	xchg r12, [rcx + 14 * 8]
	xchg r13, [rcx + 15 * 8]
	xchg r14, [rcx + 16 * 8]
	xchg r15, [rcx + 17 * 8]

	;get caller stack
	lea rcx, SaveCallOUTContextSP
	mov rsp, [rcx]

	;release lock
	lea rdx, CallOUTLock
	mov eax, 0
	lock xchg DWORD PTR [rdx], eax

	ret

CallOUT	ENDP


_TEXT	ENDS

_DATA SEGMENT

	CallDxLocation			DQ	0
	SaveCallOUTContext		DQ	0
	SaveCallOUTContextSP	DQ	0
	CallOUTLock				DD	0

_DATA ENDS

END
