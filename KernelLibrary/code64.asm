.code

EXTERN g_DriverObject: QWORD
EXTERN __imp_ObfDereferenceObject: QWORD

EXTERN RundownRoutine_Proc : PROC
EXTERN KernelRoutine_Proc : PROC
EXTERN NormalRoutine_Proc : PROC

; VOID NTAPI KRUNDOWN_ROUTINE(_KAPC* Apc)
; 
RundownRoutine PROC
	; RCX = pointer to _KAPC

	sub rsp,28h
	call RundownRoutine_Proc
	add rsp,28h

	test rax,rax
	jz @@1

	mov rcx,g_DriverObject
	jmp __imp_ObfDereferenceObject

@@1:
	ret
RundownRoutine ENDP

; VOID NTAPI KKERNEL_ROUTINE(_KAPC* Apc, PVOID NormalRoutine, PVOID* NormalContext, PVOID* SystemArgument1, PVOID* SystemArgument2)
;
KernelRoutine PROC
	; RCX         = pointer to _KAPC
	; RDX         = pointer to NormalRoutine
	; R8          = pointer to NormalContext
	; R9          = pointer to SystemArgument1
	; [RSP + 28h] = pointer to SystemArgument2

	; Mov SystemArgument2 for the forwarded call to KernelRoutine_Proc
	mov rax,[rsp+28h]
	mov [rsp+18h],rax

	; Save our return address in the shadow stack
	mov rax,[rsp]
	mov [rsp+20h],rax

	; During call to KernelRoutine:                   During call to KernelRoutine_Proc:
	; 
	;                                                 RSP:
	;                                                 -10h = return address from KernelRoutine_Proc
	; RSP:                                            -08h =  - shadow stack
	; +00h = return address from KernelRoutine        +00h =  - shadow stack
	; +08h =  - shadow stack                          +08h =  - shadow stack
	; +10h =  - shadow stack                          +10h =  - shadow stack
	; +18h =  - shadow stack                          +18h = pointer to SystemArgument2
	; +20h =  - shadow stack                          +20h = (saved return addr)
	; +28h = pointer to SystemArgument2

	; Align stack pointer on the 16-byte boundary for the KernelRoutine_Proc call (and restore it afterwards)
	;	push rax = 8 bytes
	;	call	 = 8 bytes
	push rax
	call KernelRoutine_Proc
	test rax,rax
	pop rax

	; Restore our return address
	mov rax,[rsp+20h]
	mov [rsp],rax

	; Act depending on return value from function
	jz @@1

	; Then invoke ObDereferenceObject(g_DriverObject)
	; 
	; IMPORTANT: We need to invoke ObfDereferenceObject via a JMP because it will be freeing the driver
	;            memory that our code runs from, thus can't return into it via a CALL instruction!
	;
	mov             rcx, g_DriverObject
	jmp             __imp_ObfDereferenceObject

@@1:
	ret
KernelRoutine ENDP

; VOID NTAPI KNORMAL_ROUTINE(PVOID NormalContext, PVOID SystemArgument1, PVOID SystemArgument2)
;
NormalRoutine PROC
	; RCX = NormalContext
	; RDX = SystemArgument1
	; R8  = SystemArgument2

	; First call our NormalRoutine_Proc from C file
	sub rsp,28h
	call NormalRoutine_Proc
	add rsp,28h

	; Act depending on return value from function
	test rax,rax
	jz @@1

	mov              rcx, g_DriverObject
	jmp              __imp_ObfDereferenceObject

@@1:
	ret
NormalRoutine ENDP

END