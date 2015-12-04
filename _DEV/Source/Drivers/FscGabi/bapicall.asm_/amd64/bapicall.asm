; void BapiCall(* bapi, PHYSICAL_ADDRESS, PHYSICAL_ADDRESS, PHYSICAL_ADDRESS)

; THIS FILE IS NOT COMPILED UNDER VISUAL STUDIO....
; THE OBJECT IS UNDER CLEAR CASE CONTROL

_TEXT	SEGMENT

PUBLIC	BapiCall
; bapi address is in rcx
; 1st parameter is in rdx
; 2nd parameter is in r8d
; 3rd parameter is in r9d
BapiCall	PROC
	push	r9
	push	r8
	push	rdx
	call	rcx
	add	rsp, 3*8
	ret	0

BapiCall	ENDP
_TEXT	ENDS
END
