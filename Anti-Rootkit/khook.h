#pragma once

UINT_PTR GetSSDT64();
UINT_PTR GetShadowSSDT64();
UINT_PTR GetSSDTFuncCurAddr(ULONG index);
ULONG GetOffsetAddress(UINT_PTR FuncAddr, int ParamCount);
VOID ModifyKeBugCheckEx();
VOID HookSSDT();
VOID UnHookSSDT();
PULONG GetKiServiceTable();

ULONG64 GetShadowSSDTFuncCurAddr(ULONG Index);
VOID ModifyShadowSSDT(ULONG64 Index, ULONG64 Address,ULONG ParamCount);

NTSTATUS __fastcall Fake_NtTerminateProcess(IN HANDLE ProcessHandle, IN NTSTATUS ExitStatus);
UINT_PTR __fastcall ProxyNtUserPostMessage(ULONG hWnd, ULONG Msg, UINT64 wParam, ULONG lParam);
void ModifyNtUserWindowFromPhysicalPoint();
void HookShadowSSDT();
void UnhookShadowSSDT();

extern "C" {
	NTKERNELAPI UCHAR* PsGetProcessImageFileName(IN PEPROCESS Process);
}