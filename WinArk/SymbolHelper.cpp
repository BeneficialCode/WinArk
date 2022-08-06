#include "stdafx.h"
#include "SymbolHelper.h"
#include "Helpers.h"
#include <filesystem>

SymbolHelper& SymbolHelper::Get() {
	static SymbolHelper helper;
	return helper;
}

std::unique_ptr<SymbolInfo> SymbolHelper::GetSymbolFromAddress(DWORD64 address, PDWORD64 offset) {
	SymbolHandler kernel;
	kernel.LoadSymbolsForModule(_kernelPdb.c_str(), _kernelBase, _kernelSize);
	auto symbol = kernel.GetSymbolFromAddress(address, offset);
	if (symbol != nullptr)
		return symbol;
	SymbolHandler win32k;

	win32k.LoadSymbolsForModule(_win32kPdb.c_str(), _win32kBase, _win32kSize);
	return win32k.GetSymbolFromAddress(address, offset);
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
	_win32kSize = size;
	_win32kPdb = pdbFile;
	_win32kModule = std::string(name, 0, name.find("."));

#ifdef _WIN64
	_win32kBase = (DWORD64)win32kBase;
#else
	_win32kBase = (DWORD32)win32kBase;
#endif

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
	_kernelSize = size;
	_kernelPdb = pdbFile;
	_kernelModule = std::string(name, 0, name.find("."));
#ifdef _WIN64
	_kernelBase = (DWORD64)kernelBase;
#else
	_kernelBase = (DWORD)kernelBase;
#endif
	
}

SymbolHelper::~SymbolHelper() {
}

// https://blog.csdn.net/xiaoxinjiang/article/details/7013488
ULONG64 SymbolHelper::GetKernelSymbolAddressFromName(PCSTR name) {
	SymbolHandler kernel;
	kernel.LoadSymbolsForModule(_kernelPdb.c_str(), _kernelBase, _kernelSize);
	std::string symbolName = _kernelModule + "!" + name;
	return kernel.GetSymbolAddressFromName(symbolName.c_str());
}

ULONG64 SymbolHelper::GetWin32kSymbolAddressFromName(PCSTR name) {
	SymbolHandler win32k;
	win32k.LoadSymbolsForModule(_win32kPdb.c_str(), _win32kBase, _win32kSize);
	std::string symbolName = _win32kModule + "!" + name;
	return win32k.GetSymbolAddressFromName(symbolName.c_str());
}
