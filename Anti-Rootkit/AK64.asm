.data

.code
GetSSDTFunctionAddr PROC
	mov rax,rcx
	lea r10,[rdx]
	mov edi,eax
	shr edi,7
	and edi,20h
	mov r10,qword ptr [r10+rdi]
	movsxd r11,dword ptr [r10+rax*4]
	mov rax,r11
	sar r11,4
	add r10,r11
	mov rax,r10
	ret
GetSSDTFunctionAddr ENDP
	
ForceShutdown PROC
	mov ax,2001h
	mov dx,1004h
	out dx,ax
	ret
ForceShutdown ENDP

ForceReboot PROC
	mov al,0feh
	out 64h,al
	ret
ForceReboot ENDP

END