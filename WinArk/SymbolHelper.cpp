#include "stdafx.h"
#include "SymbolHelper.h"
#include "Helpers.h"
#include <filesystem>
#include <PEParser.h>
#include "SymbolFileInfo.h"

bool SymbolHelper::GetPdbFile(std::wstring fileName, std::string& pdbDir,
	std::string& pdbName) {
	WCHAR path[MAX_PATH];
	::GetSystemDirectory(path, MAX_PATH);
	wcscat_s(path, L"\\");
	wcscat_s(path, fileName.c_str());
	PEParser parser(path);
	auto dir = parser.GetDataDirectory(IMAGE_DIRECTORY_ENTRY_DEBUG);
	if (dir != nullptr) {
		SymbolFileInfo info;
		auto entry = static_cast<PIMAGE_DEBUG_DIRECTORY>(parser.GetAddress(dir->VirtualAddress));
		ULONG_PTR VA = reinterpret_cast<ULONG_PTR>(parser.GetBaseAddress());
		info.GetPdbSignature(VA, entry);
		::GetCurrentDirectory(MAX_PATH, path);
		wcscat_s(path, L"\\Symbols");
		std::wstring curDir = path;
		std::wstring dir = curDir + L"\\" + info._path.GetString();
		std::wstring name = info._pdbFile.GetString();
		pdbDir = Helpers::WstringToString(dir);
		pdbName = Helpers::WstringToString(name);
		return true;
	}
	return false;
}

void SymbolHelper::Init() {
	void* win32kBase = Helpers::GetWin32kBase();
	DWORD size = Helpers::GetWin32kImageSize();

	std::string pdbPath, pdbName;
	std::wstring fileName =L"win32k.sys";

	GetPdbFile(fileName, pdbPath, pdbName);

	std::string pdbFile = pdbPath + "\\" + pdbName;
	_win32kSize = size;
	_win32kPdb = pdbFile;
	_win32kModule = std::string(pdbName, 0, pdbName.find("."));

#ifdef _WIN64
	_win32kBase = (DWORD64)win32kBase;
#else
	_win32kBase = (DWORD32)win32kBase;
#endif

	void* kernelBase = Helpers::GetKernelBase();
	size = Helpers::GetKernelImageSize();

	std::string kSysName = Helpers::GetNtosFileName();
	fileName = Helpers::StringToWstring(kSysName);
	GetPdbFile(fileName, pdbPath, pdbName);

	pdbFile = pdbPath + "\\" + pdbName;
	_kernelSize = size;
	_kernelPdb = pdbFile;
	_kernelModule = std::string(pdbName, 0, pdbName.find("."));
#ifdef _WIN64
	_kernelBase = (DWORD64)kernelBase;
#else
	_kernelBase = (DWORD)kernelBase;
#endif

	void* flgmgrBase = Helpers::GetKernelModuleBase("fltmgr.sys");
	size = Helpers::GetKernelModuleImageSize("fltmgr.sys");
	
	fileName = L"drivers\\fltmgr.sys";
	GetPdbFile(fileName, pdbPath, pdbName);
	pdbFile = pdbPath + "\\" + pdbName;

	_fltmgrSize = size;
	_fltmgrPdb = pdbFile;
	_fltmgrModule = std::string(pdbName, 0, pdbName.find("."));
#ifdef _WIN64
	_fltmgrBase = (DWORD64)flgmgrBase;
#else
	_fltmgrBase = (DWORD)flgmgrBase;
#endif

	void* ciBase = Helpers::GetKernelModuleBase("ci.dll");
	size = Helpers::GetKernelModuleImageSize("ci.dll");
	
	fileName = L"ci.dll";
	GetPdbFile(fileName, pdbPath, pdbName);
	pdbFile = pdbPath + "\\" + pdbName;

	_ciSize = size;
	_ciPdb = pdbFile;
	_ciModule = std::string(pdbName, 0, pdbName.find("."));
#ifdef _WIN64
	_ciBase = (DWORD64)ciBase;
#else
	_ciBase = (DWORD)ciBase;
#endif

	_win32k.LoadSymbolsForModule(_win32kPdb.c_str(), _win32kBase, _win32kSize);
	_kernel.LoadSymbolsForModule(_kernelPdb.c_str(), _kernelBase, _kernelSize);
	_fltmgr.LoadSymbolsForModule(_fltmgrPdb.c_str(), _fltmgrBase, _fltmgrSize);
	_ci.LoadSymbolsForModule(_ciPdb.c_str(), _ciBase, _ciSize);


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

DWORD SymbolHelper::GetFltmgrStructMemberOffset(std::string name, std::string memberName) {
	return _fltmgr.GetStructMemberOffset(name, memberName);
}

DWORD SymbolHelper::GetKernelStructMemberSize(std::string name, std::string memberName) {
	return _kernel.GetStructMemberSize(name, memberName);
}

DWORD SymbolHelper::GetKernelStructSize(std::string name) {
	return _kernel.GetStructSize(name);
}

DWORD SymbolHelper::GetKernelBitFieldPos(std::string name, std::string fieldName) {
	return _kernel.GetBitFieldPos(name, fieldName);
}

ULONG64 SymbolHelper::GetCiSymbolAddressFromName(PCSTR name) {
	std::string symbolName = _ciModule + "!" + name;
	return _ci.GetSymbolAddressFromName(symbolName.c_str());
}