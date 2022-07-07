#pragma once
#include "SymbolHandler.h"


class SymbolHelper {
public:
	static SymbolHelper& Get();
	SymbolHandler* GetKernel();
	SymbolHandler* GetWin32k();
	~SymbolHelper();

	std::unique_ptr<SymbolInfo> GetSymbolFromAddress(DWORD64 address, PDWORD64 offset = nullptr);

private:
	SymbolHelper();

	SymbolHandler _kernelSymbols;
	SymbolHandler _win32kSymbols;
};