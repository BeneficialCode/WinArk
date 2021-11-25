#pragma once
#include "FastMutex.h"

#define SYSMON_PREFIX "SysMon: "
#define SYSMON_TAG 'nmys'
#define PSP_MAX_LOAD_IMAGE_NOTIFY	0x40
#include "..\Anti-Rootkit\AntiRootkit.h"


/*
	PspNotifyEnableMask是一个系统通知回调是否产生的一个标记
	0位——标记是否产生模块回调。
	3位——标记是否产生线程回调。
	其他位有待分析......
*/
template<typename T>
struct FullItem {
	LIST_ENTRY Entry;
	T Data;
};

struct SysMonGlobals {
	LIST_ENTRY ItemHead;
	int ItemCount;
	FastMutex Mutex;
	LARGE_INTEGER RegCookie;
};

typedef struct _EX_CALLBACK_ROUTINE_BLOCK
{
	EX_RUNDOWN_REF        RundownProtect;
	PEX_CALLBACK_FUNCTION Function;
	PVOID                 Context;
} EX_CALLBACK_ROUTINE_BLOCK, * PEX_CALLBACK_ROUTINE_BLOCK;

typedef struct _EX_FAST_REF_S
{
	union
	{
		PVOID Object;
#if defined (_WIN64)
		ULONG_PTR RefCnt : 4;
#else
		ULONG_PTR RefCnt : 3;
#endif
		ULONG_PTR Value;
	};
} EX_FAST_REF_S, * PEX_FAST_REF_S;

typedef struct _EX_CALLBACK
{
	EX_FAST_REF_S RoutineBlock;
} EX_CALLBACK, * PEX_CALLBACK;

#if defined (_WIN64)
#define MAX_FAST_REFS 15
#else
#define MAX_FAST_REFS 7
#endif

typedef struct _CM_CALLBACK_CONTEXT_BLOCKEX
{
	LIST_ENTRY		ListEntry;
	ULONG           PreCallListCount;
	ULONG			Pad;
	LARGE_INTEGER	Cookie;
	PVOID           CallerContext;
	PVOID			Function;
	UNICODE_STRING	Altitude;
	LIST_ENTRY		ObjectContextListHead;
} CM_CALLBACK_CONTEXT_BLOCKEX, * PCM_CALLBACK_CONTEXT_BLOCKEX;

//struct CallbackInfo {
//	ULONG Count;
//	void* Address[ANYSIZE_ARRAY];
//};

extern SysMonGlobals g_SysMonGlobals;
extern ULONG	PspNotifyEnableMask;


void PushItem(LIST_ENTRY* entry);

void OnProcessNotify(_Inout_ PEPROCESS Process, _In_ HANDLE ProcessId, _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo);
void OnThreadNotify(_In_ HANDLE ProcessId, _In_ HANDLE ThreadId, _In_ BOOLEAN Create);
void OnImageLoadNotify(_In_opt_ PUNICODE_STRING FullImageName, _In_ HANDLE ProcessId, _In_ PIMAGE_INFO ImageInfo);
NTSTATUS OnRegistryNotify(PVOID context, PVOID arg1, PVOID arg2);

VOID PsCallImageNotifyRoutines(
	_In_ PUNICODE_STRING ImageName,
	_In_ HANDLE ProcessId,
	_In_ PVOID FileObject,
	_Out_ PIMAGE_INFO_EX ImageInfoEx
);

bool EnumProcessNotify(PEX_CALLBACK callback, ULONG count,KernelCallbackInfo* info);
bool EnumThreadNotify(PEX_CALLBACK callback, ULONG count);
bool EnumImageNotify(PEX_CALLBACK callback, ULONG count);

extern "C" {
	NTKERNELAPI UCHAR* NTAPI PsGetProcessImageFileName(_In_ PEPROCESS Process);
}