#include "stdafx.h"
#include "SymbolHelper.h"
#include "Helpers.h"
#include <filesystem>

void SymbolHelper::Init() {
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
	_win32k.LoadSymbolsForModule(_win32kPdb.c_str(), _win32kBase, _win32kSize);
	_kernel.LoadSymbolsForModule(_kernelPdb.c_str(), _kernelBase, _kernelSize);
}

std::unique_ptr<SymbolInfo> SymbolHelper::GetSymbolFromAddress(DWORD64 address, PDWORD64 offset) {
	auto symbol = _kernel.GetSymbolFromAddress(address, offset);
	if (symbol != nullptr)
		return symbol;

	return _win32k.GetSymbolFromAddress(address, offset);
}

// https://blog.csdn.net/xiaoxinjiang/article/details/7013488
ULONG64 SymbolHelper::GetKernelSymbolAddressFromName(PCSTR name) {
	std::string symbolName = _kernelModule + "!" + name;
	return _kernel.GetSymbolAddressFromName(symbolName.c_str());
}

ULONG64 SymbolHelper::GetWin32kSymbolAddressFromName(PCSTR name) {
	// https://stackoverflow.com/questions/4867159/how-do-you-use-symloadmoduleex-to-load-a-pdb-file
	std::string symbolName = _win32kModule + "!" + name;
	return _win32k.GetSymbolAddressFromName(symbolName.c_str());
}

DWORD SymbolHelper::GetKernelStructMemberOffset(std::string name, std::string memberName) {
	return _kernel.GetStructMemberOffset(name, memberName);
}
