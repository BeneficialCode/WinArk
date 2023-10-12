#pragma once
#include "FastMutex.h"
#include "CriticalRegion.h"
#include "Section.h"

#define SYSMON_PREFIX "SysMon: "
#define SYSMON_TAG 'nmys'
#define PSP_MAX_LOAD_IMAGE_NOTIFY	0x40
#include "..\Anti-Rootkit\AntiRootkit.h"




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


struct _OB_CALLBACK_BLOCK;

typedef struct _OB_CALLBACK_ENTRY {
	LIST_ENTRY CallbackList;
	OB_OPERATION Operations;						// 1 for Creations, 2 for Duplications
	BOOLEAN Enabled;
	struct _OB_CALLBACK_BLOCK* RegistrationHandle;	// Points to the OB_CALLBACK_BLOCK used for ObUnRegisterCallback
	POBJECT_TYPE ObjectType;
	POB_PRE_OPERATION_CALLBACK PreOperation;
	POB_POST_OPERATION_CALLBACK PostOperation;
	EX_RUNDOWN_REF RundownProtect;
}OB_CALLBACK_ENTRY, * POB_CALLBACK_ENTRY;



// x86 0x10	0x24	16	36
// x64 0x20 0x40	32	64
typedef struct _OB_CALLBACK_BLOCK {
	USHORT Version;
	USHORT Count;
	POB_OPERATION_REGISTRATION RegistrationContext;
	UNICODE_STRING Altitude;
	OB_CALLBACK_ENTRY Items[ANYSIZE_ARRAY]; // Callback array
}OB_CALLBACK_BLOCK, * POB_CALLBACK_BLOCK;



typedef struct _OB_POST_CALLBACK_ENTRY {
	LIST_ENTRY Entry;
	POB_CALLBACK_ENTRY CallbackEntry;
	PVOID CallContext;
}OB_POST_CALLBACK_ENTRY,*POB_POST_CALLBACK_ENTRY;

extern SysMonGlobals g_SysMonGlobals;
extern UNICODE_STRING g_BackupDir;

extern Section g_sec;		// native section object

#ifdef _WIN64
extern Section g_secWow;	// Wow64 section object
#endif // _WIN64

//
// Lego Callback
//
typedef
VOID
(NTAPI* PLEGO_NOTIFY_ROUTINE)(
	_In_ PKTHREAD Thread
	);

void PushItem(LIST_ENTRY* entry);

void OnProcessNotify(_Inout_ PEPROCESS Process, _In_ HANDLE ProcessId, _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo);
void OnThreadNotify(_In_ HANDLE ProcessId, _In_ HANDLE ThreadId, _In_ BOOLEAN Create);
void OnImageLoadNotify(_In_opt_ PUNICODE_STRING FullImageName, _In_ HANDLE ProcessId, _In_ PIMAGE_INFO ImageInfo);
NTSTATUS OnRegistryNotify(PVOID context, PVOID arg1, PVOID arg2);

bool EnumSystemNotify(PEX_CALLBACK callback, ULONG count,KernelCallbackInfo* info);
bool EnumObCallbackNotify(ULONG callbackListOffset,ObCallbackInfo* info);
LONG GetObCallbackCount(ULONG callbackListOffset);
bool RemoveObCallbackNotify(POB_CALLBACK_ENTRY pCallbackEntry);

bool EnumRegistryNotify(PLIST_ENTRY pListHead, CmCallbackInfo* info);

NTSTATUS BackupFile(_In_ PUNICODE_STRING FileName);

KSTART_ROUTINE RemoveImageNotify;

extern "C" {
	NTKERNELAPI UCHAR* NTAPI PsGetProcessImageFileName(_In_ PEPROCESS Process);
}

PEX_CALLBACK_ROUTINE_BLOCK ExReferenceCallBackBlock(
	_Inout_ PEX_CALLBACK Callback
);

LOGICAL ExFastRefDereference(
	_Inout_ PEX_FAST_REF_S FastRef,
	_In_ PVOID Object
);

VOID ExDereferenceCallBackBlock(
	_Inout_ PEX_CALLBACK Callback,
	_In_ PEX_CALLBACK_ROUTINE_BLOCK CallbackBlock
);

EX_FAST_REF_S ExFastReference(
	_Inout_ PEX_FAST_REF_S FastRef
);

BOOLEAN
ExFastRefObjectNull(
	_In_ EX_FAST_REF_S FastRef
);

PVOID
ExFastRefGetObject(
	_In_ EX_FAST_REF_S FastRef
);

LOGICAL
ExFastRefIsLastReference(
	_In_ EX_FAST_REF_S FastRef
);

LOGICAL
ExFastRefAddAdditionalReferenceCounts(
	_Inout_ PEX_FAST_REF_S FastRef,
	_In_ PVOID Object,
	_In_ ULONG RefsToAdd
);

NTSTATUS RemoveSystemNotify(_In_ PVOID context);

using PPsSetCreateProcessNotifyRoutineEx2 = NTSTATUS
(NTAPI*)(
	_In_ ULONG NotifyType,
	_In_ PVOID NotifyInformation,
	_In_ BOOLEAN Remove
);

NTSTATUS EnumMiniFilterOperations(MiniFilterData* pData, OperationInfo* pInfo);

NTSTATUS RemoveMiniFilter(MiniFilterData* pData);

KSTART_ROUTINE RemoveFilter;

VOID PsCallImageNotifyRoutines(
	_In_ PUNICODE_STRING ImageName,
	_In_ HANDLE ProcessId,
	_Inout_ PIMAGE_INFO_EX ImageInfoEx,
	_In_ PVOID FileObject
);

bool DisableDriverLoad(CiSymbols* pSym);
bool EnableDriverLoad();

bool StartLogDriverHash(CiSymbols* pSym);
bool StopLogDriverHash();

void DisableObCallbackNotify(POB_CALLBACK_ENTRY pCallbackEntry);
void EnableObCallbackNotify(POB_CALLBACK_ENTRY pCallbackEntry);
