#include<ntddk.h>
#include"khook.h"
#include"util.h"
#include"RecoverHook.h"

typedef struct _SYSTEM_SERVICE_TABLE {
	PVOID ServiceTableBase;
	PVOID ServiceCounterTableBase;
	ULONGLONG NumberOfServices;
	PVOID ParamTableBase;
}SYSTEM_SERVICE_TABLE, *PSYSTEM_SERVICE_TABLE;

VOID RecoverSSDT(ULONG index, UINT_PTR FuncAddr,ULONG ParamCount) {
	ULONG dwtmp;
	PSYSTEM_SERVICE_TABLE SystemServiceTable;
	PULONG ServiceTableBase = NULL;
	dwtmp = GetOffsetAddress(FuncAddr,ParamCount);
	SystemServiceTable = (PSYSTEM_SERVICE_TABLE)GetSSDT64();
	ServiceTableBase = (PULONG)SystemServiceTable->ServiceTableBase;
	WriteProtectOff();
	ServiceTableBase[index] = dwtmp;
	WriteProtectOn();
}