#pragma once

struct BypassAntiKernelDbg {
	static PVOID GetExportSymbolAddress(PCWSTR symbolName);
};