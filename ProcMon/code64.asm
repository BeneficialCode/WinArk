.code

; void __stdcall UserModeNormalRoutine(PVOID NormalContext, PVOID SystemArgument1, PVOID SystemArgument2)
;
ALIGN 16		; Specify function alignment
;
UserModeNormalRoutine PROC
	; RCX = BaseAddress of this module
	; RDX = Injected DLL name as const WCHAR* (null-terminated)	<= FAKE64.dll
	; R8  = unused

	; INFO: How to get address of function in base-independent way:
	;  https://dennisbabkin.com/blog/?t=how-to-implement-getprocaddress-in-shellcode

	mov         [rsp + 08h], rcx		; BaseAddr

	; Set the stack frame & save nonvolatile registers
	;
	mov         [rsp + 10h], rdi
	mov         [rsp + 18h], rbp
	sub         rsp, 48h

	mov         rdi, rdx                            ; DllName
	; Get DllBase for ntdll.dll
	;
	mov         rax, gs:[60h]                       ; rax = ProcessEnvironmentBlock
	mov         rax, [rax+18h]                      ; rax = Ldr
	mov         rax, [rax+30h]                      ; rax = InInitializationOrderModuleList.Flink 
	mov         rcx, [rax+10h]                      ; rcx = DllBase (ntdll.dll)
	mov         rbp, rcx                            ; rbp = DllBase (ntdll.dll)

	lea         rdx, [lbl_LdrLoadDll]

	; Look up address for LdrLoadDll()
	;
	call        GetProcAddressForMod

	test        rax, rax
	jz          @@1                                 ; Skip calling that function if failed to get its pointer

	mov         r10, rax

	; Get length of our DllName
	;
	xor         rcx, rcx
	xor         eax, eax
	dec         rcx
	mov         rdx, rdi

	repne       scasw

	sub         rdi, rdx                            ; rdi = length of DllName in bytes + sizeof(WCHAR)


	; struct UNICODE_STRING {
	; 	USHORT Length;                          [+0h] length of string in bytes (not inclusing last null)
	; 	USHORT MaximumLength;                   [+2h] length of string in bytes (including last null)
	; 	USHORT* Buffer;                         [+8h] pointer to the string
	; };

	; Prepare UNICODE_STRING with our injected DLL name on the stack at [rsp + 20h]
	;
	mov         [rsp + 22h], di                     ; MaximumLength
	sub         rdi, 2
	mov         [rsp + 20h], di                     ; Length
	mov         [rsp + 28h], rdx                    ; Buffer

	xor         rcx, rcx                            ; PathToFile = 0
	xor         rdx, rdx                            ; Flags = 0
	lea         r8, [rsp + 20h]                     ; &ModuleFileName
	lea         r9,	[rsp + 30h]                     ; &ModuleHandle

	; LdrLoadDll(
	;   IN PWCHAR               PathToFile OPTIONAL,
	;   IN ULONG                Flags OPTIONAL,
	;   IN PUNICODE_STRING      ModuleFileName,
	;   OUT PHANDLE             ModuleHandle
	; );
	;
	call        r10


	; We can't do anything here if LdrLoadDll failed to load Dll


@@1:

	lea         rdx, [lbl_NtUnmapViewOfSection]
	mov         rcx, rbp                            ; rcx = base address of Ntdll.dll

	; Look up address of NtUnmapViewOfSection
	;
	call        GetProcAddressForMod

	; Restore the stack frame & nonvolatile registers
	;
	add         rsp, 48h
	mov         rdi, [rsp + 10h]
	mov         rbp, [rsp + 18h]


	; Check the result of getting function ptr
	;
	test        rax, rax
	jz          @@fail


	; IMPORTANT: We need to invoke NtUnmapViewOfSection via a JMP because it will be freeing the memory
	;            section that our code runs from, thus we cannot return into it via a CALL instruction!
	;
	xor         ecx, ecx
	mov         rdx, [rsp + 08h]	; rdx = BaseAddr
	dec         rcx                                 ; rcx = NtCurrentProcess()

	; NTSYSAPI NTSTATUS NtUnmapViewOfSection(
	;   HANDLE ProcessHandle,
	;   PVOID  BaseAddress
	; );
	;
	jmp         rax

@@fail:
	; Failed to unmap
	;
	IFDEF MASM_DEBUG
	int         3                                   ; debugging break (for debug build only)
	ENDIF

	ret


lbl_LdrLoadDll:
	db 'LdrLoadDll',0
lbl_NtUnmapViewOfSection:
	db 'NtUnmapViewOfSection',0



UserModeNormalRoutine ENDP

; FARPROC GetProcAddressForMod(HMODULE hMod, const char* pstrFunctioName)
;
ALIGN 16		; Specify function alignment
;
GetProcAddressForMod PROC PRIVATE
	; RCX = base address for module
	; RDX = function name as const char* (null-terminate)
	; RETURN:
	;		 = Pointer to function code
	;		 = 0 if failed

	push        r12
	push        rbp
	push        rsi
	push        rdi
	push        rbx

	mov         rbx, rcx                            ; rbx = PIMAGE_DOS_HEADER

	; Calculate length of function name in chars + terminating null
	;
	xor         ecx, ecx
	mov         rdi, rdx
	dec         rcx
	xor         al, al
	repne       scasb
	sub         rdi, rdx
	mov         rbp, rdi                            ; rbp = function name length in chars

	; "Walk" the PE header
	;
	movsxd      rax, dword ptr [rbx+3Ch]	; e_lfanew
	mov         eax, [rbx+rax+88h]                  ; PIMAGE_DOS_HEADER::OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress
	add         rax, rbx                            ; rax = PIMAGE_EXPORT_DIRECTORY

	mov         r9d, [rax+18h]                      ; r9 = NumberOfNames = B
	test        r9d, r9d
	jz          @@bad

	; Get pointers to PE header structures
	;
	mov         r12d, [rax+1Ch]                     ; r12 = AddressOfFunctions
	mov         r11d, [rax+20h]                     ; r11 = AddressOfNames
	mov         r10d, [rax+24h]                     ; r10 = AddressOfNameOrdinals
	add         r10, rbx
	add         r11, rbx
	add         r12, rbx


	xor         r8d, r8d                            ; A = 0, B = NumberOfNames

@@1:
	; Since export function names are sorted alphabetically we'll be using binary search algorithm:

	; Ordinal = (A + B) / 2
	;
	lea         eax, [r8d+r9d]
	shr         eax, 1                              ; eax = index (or, Ordinal)

	mov         edi, [r11+rax*4]                    ; AddressOfNames[Ordinal]
	add         rdi, rbx

	mov         rcx, rbp                            ; rcx = name length
	mov         rsi, rdx                            ; rsi = function name

	; Compare function name, letter-by-letter until they are not equal
	;
	repe        cmpsb

	jnz         @@2

	; We found a match!
	;
	movzx       eax, word ptr [r10+rax*2]           ; eax = AddressOfNameOrdinals[Ordinal]
	mov         eax, dword ptr [r12+rax*4]          ; eax = AddressOfFunctions[eax]
	add         rax, rbx                            ; rax = needed address of function

	jmp         @@out

@@2:
	; Depending on the result of comparison
	;
	js          @@3

	; A = Ordinal + 1
	;
	lea         r8d, [eax + 1]
	jmp         @@4

@@3:
	; B = Ordinal
	;
	mov         r9d, eax

@@4:
	; Check if we're out of range
	;
	cmp         r8d, r9d                            ; range: [A = r8d, B = r9d)
	jb          @@1


@@bad:
	; Nothing was found
	;
	xor         eax, eax

@@out:
	pop         rbx
	pop         rdi
	pop         rsi
	pop         rbp
	pop         r12

	ret

GetProcAddressForMod ENDP

END