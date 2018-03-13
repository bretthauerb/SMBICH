_TEXT	SEGMENT

PUBLIC	CallOUT
PUBLIC	CallIN

CallIN	PROC
; new stack location is in rcx
; Context is in rdx
; in parameter is in r8
	push rdx
	push rcx

	;routine is not reentrant, so lock it
	lea rdx, CallINLock	
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
	lea rdx, CallINDxLocation	
	mov [rdx], r8

	lea rdx, SaveCallINContext
	pop rax
	mov [rdx], rax		

	lea rdx, SaveCallINContextSP
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

	;in dx parameter decision
	push rdx
	pushfq

	lea rdx, CallDxLocation
	mov rdx, [rdx]	
	cmp edx, 0FFFFFFFFh
	je inDefault

	;offset calculation based on parameter to jmp to right in
	push rax
	movzx rax, dl
	mov rdx, 9  ; popfq, pop, in and jmp  => 9 Bytes
	mul dx	
	lea rdx, inIm
	add rdx, rax
	pop rax
	jmp rdx

	;SMI trap
inIm:
	popfq
	pop rdx
	in al,0h
	jmp inwait
	popfq
	pop rdx
	in al,1h
	jmp inwait
	popfq
	pop rdx
	in al,2h
	jmp inwait
	popfq
	pop rdx
	in al,3h
	jmp inwait
	popfq
	pop rdx
	in al,4h
	jmp inwait
	popfq
	pop rdx
	in al,5h
	jmp inwait
	popfq
	pop rdx
	in al,6h
	jmp inwait
	popfq
	pop rdx
	in al,7h
	jmp inwait
	popfq
	pop rdx
	in al,8h
	jmp inwait
	popfq
	pop rdx
	in al,9h
	jmp inwait
	popfq
	pop rdx
	in al,0ah
	jmp inwait
	popfq
	pop rdx
	in al,0bh
	jmp inwait
	popfq
	pop rdx
	in al,0ch
	jmp inwait
	popfq
	pop rdx
	in al,0dh
	jmp inwait
	popfq
	pop rdx
	in al,0eh
	jmp inwait
	popfq
	pop rdx
	in al,0fh
	jmp inwait
	popfq
	pop rdx
	in al,10h
	jmp inwait
	popfq
	pop rdx
	in al,11h
	jmp inwait
	popfq
	pop rdx
	in al,12h
	jmp inwait
	popfq
	pop rdx
	in al,13h
	jmp inwait
	popfq
	pop rdx
	in al,14h
	jmp inwait
	popfq
	pop rdx
	in al,15h
	jmp inwait
	popfq
	pop rdx
	in al,16h
	jmp inwait
	popfq
	pop rdx
	in al,17h
	jmp inwait
	popfq
	pop rdx
	in al,18h
	jmp inwait
	popfq
	pop rdx
	in al,19h
	jmp inwait
	popfq
	pop rdx
	in al,1ah
	jmp inwait
	popfq
	pop rdx
	in al,1bh
	jmp inwait
	popfq
	pop rdx
	in al,1ch
	jmp inwait
	popfq
	pop rdx
	in al,1dh
	jmp inwait
	popfq
	pop rdx
	in al,1eh
	jmp inwait
	popfq
	pop rdx
	in al,1fh
	jmp inwait
	popfq
	pop rdx
	in al,20h
	jmp inwait
	popfq
	pop rdx
	in al,21h
	jmp inwait
	popfq
	pop rdx
	in al,22h
	jmp inwait
	popfq
	pop rdx
	in al,23h
	jmp inwait
	popfq
	pop rdx
	in al,24h
	jmp inwait
	popfq
	pop rdx
	in al,25h
	jmp inwait
	popfq
	pop rdx
	in al,26h
	jmp inwait
	popfq
	pop rdx
	in al,27h
	jmp inwait
	popfq
	pop rdx
	in al,28h
	jmp inwait
	popfq
	pop rdx
	in al,29h
	jmp inwait
	popfq
	pop rdx
	in al,2ah
	jmp inwait
	popfq
	pop rdx
	in al,2bh
	jmp inwait
	popfq
	pop rdx
	in al,2ch
	jmp inwait
	popfq
	pop rdx
	in al,2dh
	jmp inwait
	popfq
	pop rdx
	in al,2eh
	jmp inwait
	popfq
	pop rdx
	in al,2fh
	jmp inwait
	popfq
	pop rdx
	in al,30h
	jmp inwait
	popfq
	pop rdx
	in al,31h
	jmp inwait
	popfq
	pop rdx
	in al,32h
	jmp inwait
	popfq
	pop rdx
	in al,33h
	jmp inwait
	popfq
	pop rdx
	in al,34h
	jmp inwait
	popfq
	pop rdx
	in al,35h
	jmp inwait
	popfq
	pop rdx
	in al,36h
	jmp inwait
	popfq
	pop rdx
	in al,37h
	jmp inwait
	popfq
	pop rdx
	in al,38h
	jmp inwait
	popfq
	pop rdx
	in al,39h
	jmp inwait
	popfq
	pop rdx
	in al,3ah
	jmp inwait
	popfq
	pop rdx
	in al,3bh
	jmp inwait
	popfq
	pop rdx
	in al,3ch
	jmp inwait
	popfq
	pop rdx
	in al,3dh
	jmp inwait
	popfq
	pop rdx
	in al,3eh
	jmp inwait
	popfq
	pop rdx
	in al,3fh
	jmp inwait
	popfq
	pop rdx
	in al,40h
	jmp inwait
	popfq
	pop rdx
	in al,41h
	jmp inwait
	popfq
	pop rdx
	in al,42h
	jmp inwait
	popfq
	pop rdx
	in al,43h
	jmp inwait
	popfq
	pop rdx
	in al,44h
	jmp inwait
	popfq
	pop rdx
	in al,45h
	jmp inwait
	popfq
	pop rdx
	in al,46h
	jmp inwait
	popfq
	pop rdx
	in al,47h
	jmp inwait
	popfq
	pop rdx
	in al,48h
	jmp inwait
	popfq
	pop rdx
	in al,49h
	jmp inwait
	popfq
	pop rdx
	in al,4ah
	jmp inwait
	popfq
	pop rdx
	in al,4bh
	jmp inwait
	popfq
	pop rdx
	in al,4ch
	jmp inwait
	popfq
	pop rdx
	in al,4dh
	jmp inwait
	popfq
	pop rdx
	in al,4eh
	jmp inwait
	popfq
	pop rdx
	in al,4fh
	jmp inwait
	popfq
	pop rdx
	in al,50h
	jmp inwait
	popfq
	pop rdx
	in al,51h
	jmp inwait
	popfq
	pop rdx
	in al,52h
	jmp inwait
	popfq
	pop rdx
	in al,53h
	jmp inwait
	popfq
	pop rdx
	in al,54h
	jmp inwait
	popfq
	pop rdx
	in al,55h
	jmp inwait
	popfq
	pop rdx
	in al,56h
	jmp inwait
	popfq
	pop rdx
	in al,57h
	jmp inwait
	popfq
	pop rdx
	in al,58h
	jmp inwait
	popfq
	pop rdx
	in al,59h
	jmp inwait
	popfq
	pop rdx
	in al,5ah
	jmp inwait
	popfq
	pop rdx
	in al,5bh
	jmp inwait
	popfq
	pop rdx
	in al,5ch
	jmp inwait
	popfq
	pop rdx
	in al,5dh
	jmp inwait
	popfq
	pop rdx
	in al,5eh
	jmp inwait
	popfq
	pop rdx
	in al,5fh
	jmp inwait
	popfq
	pop rdx
	in al,60h
	jmp inwait
	popfq
	pop rdx
	in al,61h
	jmp inwait
	popfq
	pop rdx
	in al,62h
	jmp inwait
	popfq
	pop rdx
	in al,63h
	jmp inwait
	popfq
	pop rdx
	in al,64h
	jmp inwait
	popfq
	pop rdx
	in al,65h
	jmp inwait
	popfq
	pop rdx
	in al,66h
	jmp inwait
	popfq
	pop rdx
	in al,67h
	jmp inwait
	popfq
	pop rdx
	in al,68h
	jmp inwait
	popfq
	pop rdx
	in al,69h
	jmp inwait
	popfq
	pop rdx
	in al,6ah
	jmp inwait
	popfq
	pop rdx
	in al,6bh
	jmp inwait
	popfq
	pop rdx
	in al,6ch
	jmp inwait
	popfq
	pop rdx
	in al,6dh
	jmp inwait
	popfq
	pop rdx
	in al,6eh
	jmp inwait
	popfq
	pop rdx
	in al,6fh
	jmp inwait
	popfq
	pop rdx
	in al,70h
	jmp inwait
	popfq
	pop rdx
	in al,71h
	jmp inwait
	popfq
	pop rdx
	in al,72h
	jmp inwait
	popfq
	pop rdx
	in al,73h
	jmp inwait
	popfq
	pop rdx
	in al,74h
	jmp inwait
	popfq
	pop rdx
	in al,75h
	jmp inwait
	popfq
	pop rdx
	in al,76h
	jmp inwait
	popfq
	pop rdx
	in al,77h
	jmp inwait
	popfq
	pop rdx
	in al,78h
	jmp inwait
	popfq
	pop rdx
	in al,79h
	jmp inwait
	popfq
	pop rdx
	in al,7ah
	jmp inwait
	popfq
	pop rdx
	in al,7bh
	jmp inwait
	popfq
	pop rdx
	in al,7ch
	jmp inwait
	popfq
	pop rdx
	in al,7dh
	jmp inwait
	popfq
	pop rdx
	in al,7eh
	jmp inwait
	popfq
	pop rdx
	in al,7fh
	jmp inwait
	popfq
	pop rdx
	in al,80h
	jmp inwait
	popfq
	pop rdx
	in al,81h
	jmp inwait
	popfq
	pop rdx
	in al,82h
	jmp inwait
	popfq
	pop rdx
	in al,83h
	jmp inwait
	popfq
	pop rdx
	in al,84h
	jmp inwait
	popfq
	pop rdx
	in al,85h
	jmp inwait
	popfq
	pop rdx
	in al,86h
	jmp inwait
	popfq
	pop rdx
	in al,87h
	jmp inwait
	popfq
	pop rdx
	in al,88h
	jmp inwait
	popfq
	pop rdx
	in al,89h
	jmp inwait
	popfq
	pop rdx
	in al,8ah
	jmp inwait
	popfq
	pop rdx
	in al,8bh
	jmp inwait
	popfq
	pop rdx
	in al,8ch
	jmp inwait
	popfq
	pop rdx
	in al,8dh
	jmp inwait
	popfq
	pop rdx
	in al,8eh
	jmp inwait
	popfq
	pop rdx
	in al,8fh
	jmp inwait
	popfq
	pop rdx
	in al,90h
	jmp inwait
	popfq
	pop rdx
	in al,91h
	jmp inwait
	popfq
	pop rdx
	in al,92h
	jmp inwait
	popfq
	pop rdx
	in al,93h
	jmp inwait
	popfq
	pop rdx
	in al,94h
	jmp inwait
	popfq
	pop rdx
	in al,95h
	jmp inwait
	popfq
	pop rdx
	in al,96h
	jmp inwait
	popfq
	pop rdx
	in al,97h
	jmp inwait
	popfq
	pop rdx
	in al,98h
	jmp inwait
	popfq
	pop rdx
	in al,99h
	jmp inwait
	popfq
	pop rdx
	in al,9ah
	jmp inwait
	popfq
	pop rdx
	in al,9bh
	jmp inwait
	popfq
	pop rdx
	in al,9ch
	jmp inwait
	popfq
	pop rdx
	in al,9dh
	jmp inwait
	popfq
	pop rdx
	in al,9eh
	jmp inwait
	popfq
	pop rdx
	in al,9fh
	jmp inwait
	popfq
	pop rdx
	in al,0a0h
	jmp inwait
	popfq
	pop rdx
	in al,0a1h
	jmp inwait
	popfq
	pop rdx
	in al,0a2h
	jmp inwait
	popfq
	pop rdx
	in al,0a3h
	jmp inwait
	popfq
	pop rdx
	in al,0a4h
	jmp inwait
	popfq
	pop rdx
	in al,0a5h
	jmp inwait
	popfq
	pop rdx
	in al,0a6h
	jmp inwait
	popfq
	pop rdx
	in al,0a7h
	jmp inwait
	popfq
	pop rdx
	in al,0a8h
	jmp inwait
	popfq
	pop rdx
	in al,0a9h
	jmp inwait
	popfq
	pop rdx
	in al,0aah
	jmp inwait
	popfq
	pop rdx
	in al,0abh
	jmp inwait
	popfq
	pop rdx
	in al,0ach
	jmp inwait
	popfq
	pop rdx
	in al,0adh
	jmp inwait
	popfq
	pop rdx
	in al,0aeh
	jmp inwait
	popfq
	pop rdx
	in al,0afh
	jmp inwait
	popfq
	pop rdx
	in al,0b0h
	jmp inwait
	popfq
	pop rdx
	in al,0b1h
	jmp inwait
	popfq
	pop rdx
	in al,0b2h
	jmp inwait
	popfq
	pop rdx
	in al,0b3h
	jmp inwait
	popfq
	pop rdx
	in al,0b4h
	jmp inwait
	popfq
	pop rdx
	in al,0b5h
	jmp inwait
	popfq
	pop rdx
	in al,0b6h
	jmp inwait
	popfq
	pop rdx
	in al,0b7h
	jmp inwait
	popfq
	pop rdx
	in al,0b8h
	jmp inwait
	popfq
	pop rdx
	in al,0b9h
	jmp inwait
	popfq
	pop rdx
	in al,0bah
	jmp inwait
	popfq
	pop rdx
	in al,0bbh
	jmp inwait
	popfq
	pop rdx
	in al,0bch
	jmp inwait
	popfq
	pop rdx
	in al,0bdh
	jmp inwait
	popfq
	pop rdx
	in al,0beh
	jmp inwait
	popfq
	pop rdx
	in al,0bfh
	jmp inwait
	popfq
	pop rdx
	in al,0c0h
	jmp inwait
	popfq
	pop rdx
	in al,0c1h
	jmp inwait
	popfq
	pop rdx
	in al,0c2h
	jmp inwait
	popfq
	pop rdx
	in al,0c3h
	jmp inwait
	popfq
	pop rdx
	in al,0c4h
	jmp inwait
	popfq
	pop rdx
	in al,0c5h
	jmp inwait
	popfq
	pop rdx
	in al,0c6h
	jmp inwait
	popfq
	pop rdx
	in al,0c7h
	jmp inwait
	popfq
	pop rdx
	in al,0c8h
	jmp inwait
	popfq
	pop rdx
	in al,0c9h
	jmp inwait
	popfq
	pop rdx
	in al,0cah
	jmp inwait
	popfq
	pop rdx
	in al,0cbh
	jmp inwait
	popfq
	pop rdx
	in al,0cch
	jmp inwait
	popfq
	pop rdx
	in al,0cdh
	jmp inwait
	popfq
	pop rdx
	in al,0ceh
	jmp inwait
	popfq
	pop rdx
	in al,0cfh
	jmp inwait
	popfq
	pop rdx
	in al,0d0h
	jmp inwait
	popfq
	pop rdx
	in al,0d1h
	jmp inwait
	popfq
	pop rdx
	in al,0d2h
	jmp inwait
	popfq
	pop rdx
	in al,0d3h
	jmp inwait
	popfq
	pop rdx
	in al,0d4h
	jmp inwait
	popfq
	pop rdx
	in al,0d5h
	jmp inwait
	popfq
	pop rdx
	in al,0d6h
	jmp inwait
	popfq
	pop rdx
	in al,0d7h
	jmp inwait
	popfq
	pop rdx
	in al,0d8h
	jmp inwait
	popfq
	pop rdx
	in al,0d9h
	jmp inwait
	popfq
	pop rdx
	in al,0dah
	jmp inwait
	popfq
	pop rdx
	in al,0dbh
	jmp inwait
	popfq
	pop rdx
	in al,0dch
	jmp inwait
	popfq
	pop rdx
	in al,0ddh
	jmp inwait
	popfq
	pop rdx
	in al,0deh
	jmp inwait
	popfq
	pop rdx
	in al,0dfh
	jmp inwait
	popfq
	pop rdx
	in al,0e0h
	jmp inwait
	popfq
	pop rdx
	in al,0e1h
	jmp inwait
	popfq
	pop rdx
	in al,0e2h
	jmp inwait
	popfq
	pop rdx
	in al,0e3h
	jmp inwait
	popfq
	pop rdx
	in al,0e4h
	jmp inwait
	popfq
	pop rdx
	in al,0e5h
	jmp inwait
	popfq
	pop rdx
	in al,0e6h
	jmp inwait
	popfq
	pop rdx
	in al,0e7h
	jmp inwait
	popfq
	pop rdx
	in al,0e8h
	jmp inwait
	popfq
	pop rdx
	in al,0e9h
	jmp inwait
	popfq
	pop rdx
	in al,0eah
	jmp inwait
	popfq
	pop rdx
	in al,0ebh
	jmp inwait
	popfq
	pop rdx
	in al,0ech
	jmp inwait
	popfq
	pop rdx
	in al,0edh
	jmp inwait
	popfq
	pop rdx
	in al,0eeh
	jmp inwait
	popfq
	pop rdx
	in al,0efh
	jmp inwait
	popfq
	pop rdx
	in al,0f0h
	jmp inwait
	popfq
	pop rdx
	in al,0f1h
	jmp inwait
	popfq
	pop rdx
	in al,0f2h
	jmp inwait
	popfq
	pop rdx
	in al,0f3h
	jmp inwait
	popfq
	pop rdx
	in al,0f4h
	jmp inwait
	popfq
	pop rdx
	in al,0f5h
	jmp inwait
	popfq
	pop rdx
	in al,0f6h
	jmp inwait
	popfq
	pop rdx
	in al,0f7h
	jmp inwait
	popfq
	pop rdx
	in al,0f8h
	jmp inwait
	popfq
	pop rdx
	in al,0f9h
	jmp inwait
	popfq
	pop rdx
	in al,0fah
	jmp inwait
	popfq
	pop rdx
	in al,0fbh
	jmp inwait
	popfq
	pop rdx
	in al,0fch
	jmp inwait
	popfq
	pop rdx
	in al,0fdh
	jmp inwait
	popfq
	pop rdx
	in al,0feh
	jmp inwait
	popfq
	pop rdx
	in al,0ffh
	jmp inwait

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
	
inDefault:
	popfq
	pop rdx
	in al, dx

	;force second trap to force completion of first
inWait:
	out 0EDh, al

	;save virtual processore register	
	;save rcx and get context pointer
	push rcx
	lea rcx, SaveCallINContext
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
	lea rcx, SaveCallINContextSP
	mov rsp, [rcx]

	;release lock
	lea rdx, CallINLock
	mov eax, 0
	lock xchg DWORD PTR [rdx], eax

	ret

CallIN	ENDP

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

	CallINDxLocation		DQ	0
	SaveCallINContext		DQ	0
	SaveCallINContextSP		DQ	0
	CallINLock				DD	0

_DATA ENDS

END
