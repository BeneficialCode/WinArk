#include "pch.h"
#include "BypassAntiKernelDbg.h"

PVOID BypassAntiKernelDbg::GetExportSymbolAddress(PCWSTR symbolName) {
	UNICODE_STRING uname;
	RtlInitUnicodeString(&uname, symbolName);
	return MmGetSystemRoutineAddress(&uname);
}

