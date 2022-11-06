.686p
.model flat

.code

; void __stdcall UserModeNormalRoutine(PVOID NormalContext, PVOID SystemArgument1, PVOID SystemArgument2)
;
ALIGN 4		; Specify function alignment
;
_UserModeNormalRoutine@12 PROC
	; [esp + 4h]   = BaseAddress of this module
	; [esp + 8h]   = Injected DLL name as const WCHAR* (null-terminated)	<= FAKE32.dll
	; [esp + 0Ch]  = unused

	; INFO: How to get address of function in base-independent way:
	;  https://dennisbabkin.com/blog/?t=how-to-implement-getprocaddress-in-shellcode

	ASSUME FS:NOTHING                           ; needed to use fs: segment register in the code below

	; Save nonvolatile registers
	;
	push        ebp
	push        edi

	; Reserve space for local variables
	;
	sub         esp, 10h


	; Get DllBase for ntdll.dll
	;
	mov         eax, fs:[30h]                   ; eax = ProcessEnvironmentBlock
	mov         eax, [eax+0Ch]                  ; eax = Ldr
	mov         eax, [eax+1Ch]                  ; eax = InInitializationOrderModuleList.Flink 
	mov         ecx, [eax+08h]                  ; ecx = DllBase (ntdll.dll)
	mov         ebp, ecx                        ; ebp = DllBase (ntdll.dll)


	; Get pointer to "LdrLoadDll"
	;
	call        GetStr_LdrLoadDll

	mov         edx, eax

	; Look up address for LdrLoadDll()
	;
	call        GetProcAddressForMod

	test        eax, eax
	jz          @@2                              ; Skip calling that function if failed to get its pointer

	mov         [esp + 0Ch], eax


	; Get length of our DllName
	;
	mov         edi, [esp + 8h + 8h + 10h]       ; edi = Injected DLL name
	xor         ecx, ecx
	xor         eax, eax
	dec         ecx
	mov         edx, edi

	repne       scasw

	sub         edi, edx                         ; edi = length of DllName in bytes + sizeof(WCHAR)


	; struct UNICODE_STRING {
	; 	USHORT Length;                        [+0h] length of string in bytes (not inclusing last null)
	; 	USHORT MaximumLength;                 [+2h] length of string in bytes (including last null)
	; 	USHORT* Buffer;                       [+4h] pointer to the string
	; };

	; Prepare UNICODE_STRING with our injected DLL name on the stack at [esp]
	;
	mov         [esp + 2h], di                    ; MaximumLength
	sub         edi, 2
	mov         [esp], di                         ; Length
	mov         [esp + 4h], edx                   ; Buffer

	xor         eax, eax
	lea         ecx, [esp + 8h]                   ; &ModuleHandle
	lea         edx, [esp]                        ; &ModuleFileName
	push        ecx
	push        edx
	push        eax                               ; Flags = 0
	push        eax                               ; PathToFile = 0

	; __stdcall LdrLoadDll(
	;   IN PWCHAR               PathToFile OPTIONAL,
	;   IN ULONG                Flags OPTIONAL,
	;   IN PUNICODE_STRING      ModuleFileName,
	;   OUT PHANDLE             ModuleHandle
	; );
	;
	call        dword ptr [esp + 0Ch + 4 * 4h]

	; We can't do anything here if LdrLoadDll failed to load Dll


@@2:

	; Get pointer to "NtUnmapViewOfSection"
	;
	call        getStr_NtUnmapViewOfSection

	mov         edx, eax
	mov         ecx, ebp                          ; ecx = base address of Ntdll.dll

	; Look up address of NtUnmapViewOfSection
	;
	call        GetProcAddressForMod


	; Restore stack & nonvolatile registers
	;
	add         esp, 10h

	pop         edi
	pop         ebp


	; Check result of getting the function ptr
	;
	test        eax, eax
	jz          @@fail


	; IMPORTANT: We need to invoke NtUnmapViewOfSection via a JMP because it will be freeing the memory
	;            section that our code runs from, thus we cannot return into it via a CALL instruction!
	;

	; BEFORE:
	;
	; [esp]        = return address from this function
	; [esp + 4h]   = BaseAddress of this module
	; [esp + 8h]   = Injected DLL name as const WCHAR* (null-terminated)	<= FAKE64.dll
	; [esp + 0Ch]  = unused
	;
	; AFTER:
	;
	; [esp]        = << (pop off this one)
	; [esp + 4h]   = [+0] BaseAddress of this module <-- return address from this function
	; [esp + 8h]   = [+4] Injected DLL name as const WCHAR* (null-terminated)	<= FAKE64.dll  <-- ProcessHandle = -1
	; [esp + 0Ch]  = [+8] unused   <-- BaseAddress

	mov         edx, [esp + 4h]
	pop         ecx
	mov         [esp], ecx
	mov         dword ptr [esp + 4], 0ffffffffh      ; ProcessHandle = NtCurrentProcess()
	mov         [esp + 8h], edx                      ; BaseAddress

	; NTSYSAPI NTSTATUS __stdcall NtUnmapViewOfSection(
	;   HANDLE ProcessHandle,
	;   PVOID  BaseAddress
	; );
	;
	jmp         eax


@@fail:
	; Failed to unmap
	;
	IFDEF MASM_DEBUG
	int         3                                     ; debugging break (for debug build only)
	ENDIF

	ret         0Ch                                   ; needed to pop own arguments because of __stdcall

_UserModeNormalRoutine@12 ENDP


ALIGN 4	; Specify function alignment
;
GetStr_LdrLoadDll PROC PRIVATE
	; RETURN:
	;		= Address of "LdrLoadDll" ASCIIZ string

	; string must be aligned on a 2-byte boundary!
	;
	nop
	call @@z

	db 'LdrLoadDll',0

@@z:
	pop eax
	ret

GetStr_LdrLoadDll ENDP

ALIGN 4		; Specify function alignment
;
GetStr_NtUnmapViewOfSection PROC PRIVATE
	; RETURN:
	;		= Address of "NtUnmapViewOfSection" ASCIIZ string

	; string must be aligned on a 2-byte boundary!
	;
	nop

	call        @@z

	db 'NtUnmapViewOfSection',0

@@z:
	pop         eax
	ret

GetStr_NtUnmapViewOfSection ENDP

; FARPROC __fastcall GetProcAddressForMod(HMODULE hMod, const char* pstrFunctioName)
;
ALIGN 4		; Specify function alignment
;
GetProcAddressForMod PROC PRIVATE
	; ECX = base address for module
	; EDX = function name as const char* (null-terminate)
	; RETURN:
	;		 = Pointer to function code
	;		 = 0 if failed

	; Save nonvolatile register
	;
	push        ebp
	push        esi
	push        edi
	push        ebx

	; Reserve space for local variables
	;
	sub         esp, 14h


	mov         ebx, ecx                                    ; rbx = PIMAGE_DOS_HEADER

	; Calculate length of function name in chars + terminating null
	;
	xor         ecx, ecx
	mov         edi, edx
	dec         ecx
	xor         al, al
	repne       scasb
	sub         edi, edx
	mov         ebp, edi                                    ; ebp = function name length in chars

	; "Walk" the PE header
	;
	mov         eax, dword ptr [ebx+3Ch]                    ; e_lfanew
	mov         eax, [ebx+eax+78h]                          ; PIMAGE_DOS_HEADER::OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress
	add         eax, ebx                                    ; eax = PIMAGE_EXPORT_DIRECTORY

	mov         ecx, [eax+18h]                              ; ecx = NumberOfNames
	test        ecx, ecx
	jz          @@bad


	mov         [esp + 4h], ecx                             ; [esp + 4h] = NumberOfNames = B
	xor         ecx, ecx
	mov         [esp], ecx                                  ; [esp] = originally 0 = A


	; Get pointers to PE header structures
	;
	mov         ecx, [eax+1Ch]
	add         ecx, ebx
	mov         [esp + 10h], ecx                            ; [esp + 10h] = AddressOfFunctions

	mov         ecx, [eax+20h]
	add         ecx, ebx
	mov         [esp + 0Ch], ecx                            ; [esp + 0Ch] = AddressOfNames

	mov         ecx, [eax+24h]
	add         ecx, ebx
	mov         [esp + 08h], ecx                            ; [esp + 08h] = AddressOfNameOrdinals


@@1:
	; Since export function names are sorted alphabetically we'll be using binary search algorithm:

	; Ordinal = (A + B) / 2
	;
	mov         eax, [esp]
	add         eax, [esp + 4h]
	shr         eax, 1                                      ; eax = index (or, Ordinal)

	mov         ecx, [esp + 0Ch]
	mov         edi, [ecx+eax*4]                            ; AddressOfNames[Ordinal]
	add         edi, ebx

	mov         ecx, ebp                                    ; ecx = name length
	mov         esi, edx                                    ; esi = function name

	; Compare function name, letter-by-letter until they are not equal
	;
	repe        cmpsb

	jnz         @@2

	; We found a match!
	;
	mov         ecx, [esp + 8h]
	movzx       eax, word ptr [ecx+eax*2]                   ; eax = AddressOfNameOrdinals[Ordinal]
	mov         ecx, [esp + 10h]
	mov         eax, dword ptr [ecx+eax*4]                  ; eax = AddressOfFunctions[eax]
	add         eax, ebx                                    ; eax = needed address of function

	jmp         @@out

@@2:
	; Depending on the result of comparison
	;
	js          @@3

	; A = Ordinal + 1
	;
	inc         eax
	mov         [esp], eax
	jmp         @@4

@@3:
	; B = Ordinal
	;
	mov         [esp + 4h], eax

@@4:
	; Check if we're out of range
	;
	mov         eax, [esp]
	cmp         eax, [esp + 4h]                             ; range: [A = [esp], B = [esp + 4h])
	jb          @@1


@@bad:
	; Nothing was found!
	;
	xor         eax, eax

@@out:
	; Restore stack & nonvolatile registers
	;
	add         esp, 14h

	pop         ebx
	pop         edi
	pop         esi
	pop         ebp

	ret

GetProcAddressForMod ENDP

END