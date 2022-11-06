.686p
.model flat
.code

EXTERN _g_DriverObject : DWORD

EXTERN _RundownRoutine_Proc@4 : PROC
EXTERN _KernelRoutine_Proc@20 : PROC
EXTERN _NormalRoutine_Proc@12 : PROC

EXTERN __imp_@ObfDereferenceObject@4 : DWORD		; name decoration for the __fastcall calling convention

; VOID NTAPI KRUNDOWN_ROUTINE(_KAPC* Apc)
; 
_RundownRoutine@4 PROC
	; [esp + 4] = pointer to _KAPC

	; Move parameters for the forwarded call to function & push back return address
	mov             eax, [esp]
	xchg            [esp + 4h], eax
	mov             [esp], eax

	; During call to RundownRoutine:                 During call to RundownRoutine_Proc
	;
	; ESP:                                           -04h = return address from RundownRoutine_Proc
	; +00h = return address from RundownRoutine      +00h = pointer to _KAPC
	; +04h = pointer to _KAPC                        +04h = return address from RundownRoutine


	; First call our RundownRoutine_Proc from the C file
	;  BOOL __stdcall RundownRoutine_Proc(PKAPC pApc)
	;
	call            _RundownRoutine_Proc@4

	; Act depending on return value
	test            eax, eax
	jz              @@1

	; Then invoke ObDereferenceObject(g_DriverObject)
	;INFO: ObfDereferenceObject is declared as __fastcall calling convention
	; 
	; IMPORTANT: We need to invoke ObfDereferenceObject via a JMP because it will be freeing the driver
	;            memory that our code runs from, thus can't return into it via a CALL instruction!
	;
	mov             ecx, _g_DriverObject
	jmp             __imp_@ObfDereferenceObject@4

@@1:
	ret
_RundownRoutine@4 ENDP


; VOID __stdcall KKERNEL_ROUTINE(_KAPC* Apc, PVOID NormalRoutine, PVOID* NormalContext, PVOID* SystemArgument1, PVOID* SystemArgument2)
;
_KernelRoutine@20 PROC
	; [ESP + 4h]   = pointer to _KAPC
	; [ESP + 8h]   = pointer to NormalRoutine
	; [ESP + 0Ch]  = pointer to NormalContext
	; [ESP + 10h]  = pointer to SystemArgument1
	; [ESP + 14h]  = pointer to SystemArgument2

	; Move parameters for the forwarded call to function & push back return address
	mov             eax, [esp]
	xchg            [esp + 14h], eax
	xchg            [esp + 10h], eax
	xchg            [esp + 0Ch], eax
	xchg            [esp + 8h], eax
	xchg            [esp + 4h], eax
	mov             [esp], eax

	; During call to KernelRoutine:                  During call to KernelRoutine_Proc
	; 
	; ESP:                                           -04h = return address from KernelRoutine_Proc
	; +00h = return address from KernelRoutine       +00h = pointer to _KAPC
	; +04h = pointer to _KAPC                        +04h = pointer to NormalRoutine
	; +08h = pointer to NormalRoutine                +08h = pointer to NormalContext
	; +0Ch = pointer to NormalContext                +0Ch = pointer to SystemArgument1
	; +10h = pointer to SystemArgument1              +10h = pointer to SystemArgument2
	; +14h = pointer to SystemArgument2              +14h = return address from KernelRoutine


	; First call our KernelRoutine_Proc from the C file
	;  BOOL __stdcall KernelRoutine_Proc(PKAPC pApc, PKNORMAL_ROUTINE* NormalRoutine, PVOID* NormalContext, PVOID* SystemArgument1, PVOID* SystemArgument2)
	;
	call            _KernelRoutine_Proc@20

	; Act depending on return value
	test            eax, eax
	jz              @@1

	; Then invoke ObDereferenceObject(g_DriverObject)
	;INFO: ObfDereferenceObject is declared as __fastcall calling convention
	; 
	; IMPORTANT: We need to invoke ObfDereferenceObject via a JMP because it will be freeing the driver
	;            memory that our code runs from, thus can't return into it via a CALL instruction!
	;
	mov             ecx, _g_DriverObject
	jmp             __imp_@ObfDereferenceObject@4

@@1:
	ret
_KernelRoutine@20 ENDP

; VOID __stdcall KNORMAL_ROUTINE(PVOID NormalContext, PVOID SystemArgument1, PVOID SystemArgument2)
;
_NormalRoutine@12 PROC
	; [esp + 4h]   = NormalContext
	; [esp + 8h]   = SystemArgument1
	; [esp + 0xCh] = SystemArgument2

	; Move parameters for the forwarded call to function & push back return address
	mov             eax, [esp]
	xchg            [esp + 0Ch], eax
	xchg            [esp + 8h], eax
	xchg            [esp + 4h], eax
	mov             [esp], eax

	; First call our NormalRoutine_Proc from the C file
	;  BOOL __stdcall NormalRoutine_Proc(PVOID NormalContext, PVOID SystemArgument1, PVOID SystemArgument2)
	;
	call            _NormalRoutine_Proc@12

	; Act depending on return value
	test            eax, eax
	jz              @@1

	; Then invoke ObDereferenceObject(g_DriverObject)
	;INFO: ObfDereferenceObject is declared as __fastcall calling convention
	; 
	; IMPORTANT: We need to invoke ObfDereferenceObject via a JMP because it will be freeing the driver
	;            memory that our code runs from, thus can't return into it via a CALL instruction!
	;
	mov             ecx, _g_DriverObject
	jmp             __imp_@ObfDereferenceObject@4

@@1:
	ret
_NormalRoutine@12 ENDP


END