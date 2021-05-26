#include<ntddk.h>
#include<intrin.h>
#include"util.h"
#include"khook.h"


typedef struct _SYSTEM_SERVICE_TABLE {
	PVOID ServiceTableBase;
	PVOID ServiceCounterTableBase;
	ULONGLONG NumberOfServices;
	PVOID ParamTableBase;
}SYSTEM_SERVICE_TABLE, *PSYSTEM_SERVICE_TABLE;

typedef struct _SERVICE_DESCRIPTOR_TABLE {
	SYSTEM_SERVICE_TABLE ntoskrnl;	// ntoskrnl.exe
	SYSTEM_SERVICE_TABLE win32k;	// win32k.sys
	SYSTEM_SERVICE_TABLE Table2;
	SYSTEM_SERVICE_TABLE Table4;
}SYSTEM_DESCRIPTOR_TABLE, *PSYSTEM_DESCRIPTOR_TABLE;

typedef ULONG64(__fastcall *NTUSERQUERYWINDOW)
(
	IN ULONG WindowHandle,
	IN UINT_PTR TypeInformation
	);

typedef ULONG64(__fastcall *NTUSERPOSTMESSAGE)
(
	ULONG hWnd,
	ULONG Msg,
	INT64 wParam,
	ULONG lParam
	);

typedef NTSTATUS(__fastcall *NTTERMINATEPROCESS)
(
	IN HANDLE ProcessHandle,
	IN NTSTATUS ExitStatus
	);

PSYSTEM_SERVICE_TABLE KeSystemServiceTable;
PSYSTEM_SERVICE_TABLE KeServiceDescriptorTableShadow;


NTTERMINATEPROCESS NtTerminateProcess = NULL;
ULONG OldTpVal;
UCHAR OrignalCode[256] = { 0 };
UCHAR OrignalCode2[24] = { 0 };

ULONG64 ul64W32pServiceTable = 0;
ULONG64 IndexOfNtUserPostMessage = 0x100f;
ULONG64 IndexOfNtUserQueryWindow = 0x1010;
NTUSERQUERYWINDOW NtUserQueryWindow = NULL;
NTUSERPOSTMESSAGE NtUserPostMessage = NULL;
ULONG64 MyProcessId = 0;

ULONG64 IndexOfNtUserWindowFromPhysicalPoint = 0x1337;
UINT_PTR AddressNtUserWindowFromPhysicalPoint = 0;

#define SETBIT(x,y) x|=(1<<y) //将X的第Y位置1
#define CLRBIT(x,y) x&=~(1<<y) //将X的第Y位清0
#define GETBIT(x,y) (x & (1 << y)) //取X的第Y位，返回0或非0

UINT_PTR GetSSDT64() {
	UCHAR FeatureCode[] = { 0x4c,0x8d,0x15 };
	UINT_PTR Offset = 0;
	
	UINT_PTR Addr = SearchFeature(__readmsr(0xc0000082), FeatureCode, sizeof(FeatureCode));
	UINT_PTR OffsetAddr = Addr + sizeof(FeatureCode);
	memcpy(&Offset, (PVOID)OffsetAddr, 3);
	return Addr + Offset + 7;
}

UINT_PTR GetShadowSSDT64() {
	UCHAR FeatureCode[] = { 0x4c,0x8d,0x1d };
	UINT_PTR Offset = 0;
	UINT_PTR Addr = SearchFeature(__readmsr(0xc0000082), FeatureCode, sizeof(FeatureCode));
	UINT_PTR OffsetAddr = Addr + sizeof(FeatureCode);
	memcpy(&Offset, (PVOID)OffsetAddr, 3);
	return Addr + Offset + 7;
}

void ShowStuff0(LONG int32num)
{
	CHAR b[4] = { 0 };
	memcpy(&b[0], (PUCHAR)(&int32num) + 0, 1); KdPrint(("b[0] & 0xF=%ld\n", b[0] & 0xF));	//这个数参数的个数-4
	memcpy(&b[1], (PUCHAR)(&int32num) + 1, 1); KdPrint(("b[1] & 0xF=%ld\n", b[1] & 0xF));
	memcpy(&b[2], (PUCHAR)(&int32num) + 2, 1); KdPrint(("b[2] & 0xF=%ld\n", b[2] & 0xF));
	memcpy(&b[3], (PUCHAR)(&int32num) + 3, 1); KdPrint(("b[3] & 0xF=%ld\n", b[3] & 0xF));
}

UINT_PTR GetSSDTFuncCurAddr(ULONG index) {
	LONG dwtmp = 0;
	PULONG ServiceTableBase = NULL;
	ServiceTableBase = (PULONG)KeSystemServiceTable->ServiceTableBase;
	dwtmp = ServiceTableBase[index];
	ShowStuff0(dwtmp);
	dwtmp = dwtmp >> 4;
	return (UINT_PTR)dwtmp + (UINT_PTR)ServiceTableBase;
}

ULONG64 GetShadowSSDTFuncCurAddr(ULONG Index) {
	ULONG_PTR W32pServiceTable = 0, qwTemp = 0;
	LONG dwTemp = 0;
	PSYSTEM_SERVICE_TABLE pWin32k;
	pWin32k = (PSYSTEM_SERVICE_TABLE)((ULONG64)KeServiceDescriptorTableShadow + sizeof(SYSTEM_SERVICE_TABLE));
	W32pServiceTable = (UINT_PTR)(pWin32k->ServiceTableBase);
	ul64W32pServiceTable = W32pServiceTable;
	qwTemp = W32pServiceTable + 4 * (Index - 0x1000);
	dwTemp = *(PULONG)qwTemp;
	dwTemp = dwTemp >> 4;
	qwTemp = W32pServiceTable + (LONG64)dwTemp;
	return qwTemp;
}


VOID ModifyShadowSSDT(ULONG64 Index, ULONG64 Address,ULONG ParamCount) {
	UINT_PTR W32pServiceTable = 0, qwTemp = 0;
	LONG dwTemp = 0;
	char b = 0, bits[4] = { 0 };
	LONG i;
	PSYSTEM_SERVICE_TABLE pWin32k;
	pWin32k = (PSYSTEM_SERVICE_TABLE)((ULONG64)KeServiceDescriptorTableShadow + sizeof(SYSTEM_SERVICE_TABLE));
	W32pServiceTable = (ULONGLONG)(pWin32k->ServiceTableBase);
	qwTemp = W32pServiceTable + 4 * (Index - 0x1000);
	dwTemp = (LONG)(Address - W32pServiceTable);
	dwTemp = dwTemp << 4;

	// 处理参数
	if (ParamCount > 4)
		ParamCount = ParamCount - 4;
	else
		ParamCount = 0;
	memcpy(&b, &dwTemp, 1);
	// 处理低四位，填写参数个数
	for (i = 0; i < 4; i++) {
		bits[i] = GETBIT(ParamCount, i);
		if (bits[i])
			SETBIT(b, i);
		else
			CLRBIT(b, i);
	}
	memcpy(&dwTemp, &b, 1);

	WriteProtectOff();
	*(PLONG)qwTemp = dwTemp;
	WriteProtectOn();
}

UINT_PTR ProxyNtUserPostMessage(ULONG hWnd, ULONG Msg, UINT64 wParam, ULONG lParam) {
	if (NtUserQueryWindow(hWnd, 0) == MyProcessId && PsGetCurrentProcessId() != (HANDLE)MyProcessId) {
		KdPrint(("I accept a message %d from process %d\n",Msg, PsGetCurrentProcessId()));
		return 0;
	}
	else {
		KdPrint(("Orignal NtUserPostMessage called!\n"));
		return NtUserPostMessage(hWnd, Msg, wParam, lParam);
	}
}


/*[23 bytes]
lkd> u win32k!NtUserWindowFromPhysicalPoint
win32k!NtUserWindowFromPhysicalPoint:
fffff960`0014cef0 48894c2408      mov     qword ptr [rsp+8],rcx
fffff960`0014cef5 53              push    rbx
fffff960`0014cef6 4883ec60        sub     rsp,60h
fffff960`0014cefa 488b0df7072000  mov     rcx,qword ptr [win32k!gpresUser (fffff960`0034d6f8)]
fffff960`0014cf01 ff1569351c00    call    qword ptr [win32k!_imp_ExEnterPriorityRegionAndAcquireResourceExclusive (fffff960`00310470)]
*/
void ModifyNtUserWindowFromPhysicalPoint() {
	/*48:33C0 | xor rax, rax |
		 | C3 | ret 
	*/
	UCHAR code[] = "\x48\x33\xC0\xC3\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90";
	memcpy(OrignalCode2, (PVOID)AddressNtUserWindowFromPhysicalPoint, 23);
	WriteProtectOff();
	memcpy((PVOID)AddressNtUserWindowFromPhysicalPoint, code, 23);	// 保持指令完整性
	WriteProtectOn();
}

void HookShadowSSDT() {
	KeServiceDescriptorTableShadow = (PSYSTEM_SERVICE_TABLE)GetShadowSSDT64();
	MyProcessId = (ULONG64)PsGetCurrentProcessId();
	KdPrint(("MyProcessId: %ld", MyProcessId));
	NtUserQueryWindow = (NTUSERQUERYWINDOW)GetShadowSSDTFuncCurAddr(IndexOfNtUserQueryWindow);
	KdPrint(("NtUserQueryWindow: %p", NtUserQueryWindow));
	NtUserPostMessage = (NTUSERPOSTMESSAGE)GetShadowSSDTFuncCurAddr(IndexOfNtUserPostMessage);
	KdPrint(("NtUserPostMessage: %p", NtUserPostMessage));
	AddressNtUserWindowFromPhysicalPoint = GetShadowSSDTFuncCurAddr(IndexOfNtUserWindowFromPhysicalPoint);
	KdPrint(("AddressNtUserWindowFromPhysicalPoint: %p", AddressNtUserWindowFromPhysicalPoint));
	UINT_PTR myfunc;
	UCHAR jmp_code[] = "\xFF\x25\x00\x00\x00\x00\x90\x90\x90\x90\x90\x90\x90\x90";
	myfunc = (UINT_PTR)ProxyNtUserPostMessage;
	memcpy(&jmp_code[6], &myfunc, 8);
	ModifyNtUserWindowFromPhysicalPoint();
	WriteProtectOff();
	memcpy((PVOID)(AddressNtUserWindowFromPhysicalPoint + 4), jmp_code, 14);
	WriteProtectOn();
	ModifyShadowSSDT(IndexOfNtUserPostMessage, AddressNtUserWindowFromPhysicalPoint + 4,1);
	KdPrint(("Hook Shadow SSDT Ok!\n"));
}

void UnhookShadowSSDT() {
	ModifyShadowSSDT(IndexOfNtUserPostMessage, (ULONG64)NtUserPostMessage,4);
	WriteProtectOff();
	memcpy((PVOID)AddressNtUserWindowFromPhysicalPoint, OrignalCode2, 23);
	WriteProtectOn();
	KdPrint(("Unhook ShadowSSDT OK!"));
}



ULONG GetOffsetAddress(UINT_PTR FuncAddr,int ParamCount) {
	ULONG dwtmp = 0, i;
	PULONG ServiceTableBase = NULL;
	CHAR b = 0, bits[4] = { 0 };
	ServiceTableBase = (PULONG)KeSystemServiceTable->ServiceTableBase;
	dwtmp = (ULONG)(FuncAddr - (UINT_PTR)ServiceTableBase);
	dwtmp = dwtmp >> 4;
	//处理参数
	if (ParamCount > 4)
		ParamCount = ParamCount - 4;
	else
		ParamCount = 0;
	//获得dwtmp的第一个字节
	memcpy(&b, &dwtmp, 1);
	//处理低四位，填写参数个数
	for (i = 0; i < 4; i++)
	{
		bits[i] = GETBIT(ParamCount, i);
		if (bits[i])
			SETBIT(b, i);
		else
			CLRBIT(b, i);
	}
	//把数据复制回去
	memcpy(&dwtmp, &b, 1);
	return dwtmp;
}

VOID ModifyKeBugCheckEx() {
	UCHAR jmp_code[] = "\x48\xB8\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\xFF\xE0";
	UINT_PTR FuncAddr = (UINT_PTR)Fake_NtTerminateProcess;
	RtlCopyMemory(&jmp_code[2],&FuncAddr , 8);	// 注意！ 这里拷贝的是函数地址
	WriteProtectOff();
	memcpy(OrignalCode, KeBugCheckEx, 15);
	memset(KeBugCheckEx, 0x90, 15);
	memcpy(KeBugCheckEx, jmp_code, 12);
	WriteProtectOn();
}

NTSTATUS _fastcall Fake_NtTerminateProcess(IN HANDLE ProcessHandle, IN NTSTATUS ExitStatus) {
	PEPROCESS Process;
	NTSTATUS status = ObReferenceObjectByHandle(ProcessHandle, 0, *PsProcessType, KernelMode, (PVOID*)&Process, NULL);
	KdPrint(("Fake_NtTerminateProcess called!\n"));
	if (NT_SUCCESS(status)) {
		if (!_stricmp((char*)PsGetProcessImageFileName(Process), "calc.exe"))
			return STATUS_ACCESS_DENIED;
		else
			return NtTerminateProcess(ProcessHandle, ExitStatus);
	}
	else
		return NtTerminateProcess(ProcessHandle, ExitStatus);
}

/*
填写KeBugCheckEx的地址
在KeBugCheckEx填写jmp,跳到Fake_NtTerminateProcess
*/
VOID HookSSDT() {
	KeSystemServiceTable = (PSYSTEM_SERVICE_TABLE)GetSSDT64();
	PULONG ServiceTableBase = NULL;
	// get old address
	NtTerminateProcess = (NTTERMINATEPROCESS)GetSSDTFuncCurAddr(41);
	KdPrint(("Old_NtTerminateProcess: %p", NtTerminateProcess));
	// set KeBugCheckEx()
	ModifyKeBugCheckEx();
	// show new address
	ServiceTableBase = (PULONG)KeSystemServiceTable->ServiceTableBase;
	OldTpVal = ServiceTableBase[41];
	WriteProtectOff();
	ServiceTableBase[41] = GetOffsetAddress((UINT_PTR)KeBugCheckEx,11);	// 篡改为KeBugCheckEx
	WriteProtectOn();
	KdPrint(("KeBugCheckEx: %p", (UINT_PTR)KeBugCheckEx));
	KdPrint(("New_NtTerminateProcess: %p", GetSSDTFuncCurAddr(41)));
}

VOID UnHookSSDT()
{
	PULONG ServiceTableBase = NULL;
	ServiceTableBase = (PULONG)KeSystemServiceTable->ServiceTableBase;
	WriteProtectOff();
	ServiceTableBase[41] = GetOffsetAddress((UINT_PTR)NtTerminateProcess,2);	// 恢复为原来的index
	memcpy(KeBugCheckEx, OrignalCode, 15);
	WriteProtectOn();
	KdPrint(("NtTerminateProcess: %p", GetSSDTFuncCurAddr(41)));
}

PULONG GetKiServiceTable() {
	KeSystemServiceTable = (PSYSTEM_SERVICE_TABLE)GetSSDT64();
	return (PULONG)KeSystemServiceTable->ServiceTableBase;
}

