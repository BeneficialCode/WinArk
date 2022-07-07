#include "stdafx.h"
#include "SymbolHelper.h"
#include "Helpers.h"
#include <filesystem>

SymbolHelper& SymbolHelper::Get() {
	static SymbolHelper helper;
	return helper;
}

SymbolHandler* SymbolHelper::GetKernel() {
	return &_kernelSymbols;
}

SymbolHandler* SymbolHelper::GetWin32k() {
	return &_win32kSymbols;
}

std::unique_ptr<SymbolInfo> SymbolHelper::GetSymbolFromAddress(DWORD64 address, PDWORD64 offset) {
	auto symbol = _kernelSymbols.GetSymbolFromAddress(address, offset);
	if (symbol != nullptr)
		return symbol;
	return _win32kSymbols.GetSymbolFromAddress(address, offset);
}

SymbolHelper::SymbolHelper() {
	void* win32kBase = Helpers::GetWin32kBase();
	DWORD size = Helpers::GetWin32kImageSize();
	char symPath[MAX_PATH];
	::GetCurrentDirectoryA(MAX_PATH, symPath);
	std::string pdbPath = "\\Symbols";
	std::string name;
	pdbPath = symPath + pdbPath;

	for (auto& iter : std::filesystem::directory_iterator(pdbPath)) {
		auto filename = iter.path().filename().string();
		if (filename.find("win32") != std::string::npos) {
			name = filename;
			break;
		}
	}
	std::string pdbFile = pdbPath + "\\" + name;
	_win32kSymbols.LoadSymbolsForModule(pdbFile.c_str(), (DWORD64)win32kBase, size);

	void* kernelBase = Helpers::GetKernelBase();
	size = Helpers::GetKernelImageSize();

	for (auto& iter : std::filesystem::directory_iterator(pdbPath)) {
		auto filename = iter.path().filename().string();
		if (filename.find("ntk") != std::string::npos) {
			name = filename;
			break;
		}
	}

	ATLTRACE("%s", name.c_str());

	pdbFile = pdbPath + "\\" + name;
	_kernelSymbols.LoadSymbolsForModule(pdbFile.c_str(), (DWORD64)kernelBase, size);
}

SymbolHelper::~SymbolHelper() {
}