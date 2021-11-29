#include "pch.h"
#include "SymbolHandler.h"
#include <memory>
#include <atlstr.h>

enum SYSTEM_INFORMATION_CLASS {
	SystemModuleInformation = 11
};

typedef struct _RTL_PROCESS_MODULE_INFORMATION {
	HANDLE Section;
	PVOID MappedBase;
	PVOID ImageBase;
	ULONG ImageSize;
	ULONG Flags;
	USHORT LoadOrderIndex;
	USHORT InitOrderIndex;
	USHORT LoadCount;
	USHORT OffsetToFileName;
	UCHAR  FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, * PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES {
	ULONG NumberOfModules;
	RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, * PRTL_PROCESS_MODULES;

extern "C" NTSTATUS NTAPI NtQuerySystemInformation(
	_In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
	_Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
	_In_ ULONG SystemInformationLength,
	_Out_opt_ PULONG ReturnLength
);

using namespace std;

SymbolInfo::SymbolInfo() {
	auto size = sizeof(SYMBOL_INFO) + MAX_SYM_NAME;
	m_Symbol = static_cast<SYMBOL_INFO*>(malloc(size));
	::memset(m_Symbol, 0, size);
	m_Symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	m_Symbol->MaxNameLen = MAX_SYM_NAME;
}

SymbolInfo::~SymbolInfo() {
	::free(m_Symbol);
}

SymbolHandler::SymbolHandler(HANDLE hProcess, PCSTR searchPath,DWORD symOptions){
	m_hProcess = hProcess;
	::SymSetOptions(symOptions);
	::SymInitialize(m_hProcess, searchPath, true);
}

SymbolHandler::~SymbolHandler() {
	::SymCleanup(m_hProcess);
	if (m_hProcess != ::GetCurrentProcess())
		::CloseHandle(m_hProcess);
}

ULONG64 SymbolHandler::LoadSymbolsForModule(PCSTR moduleName,DWORD64 baseAddress,DWORD dllSize) {
	_address = SymLoadModuleEx(m_hProcess, nullptr, moduleName, nullptr, baseAddress, dllSize, nullptr, 0);
	return _address;
}

HANDLE SymbolHandler::GetHandle() const {
	return m_hProcess;
}

//void SymbolHandler::EnumSymbols(string mask) {
//	SymEnumSymbols(m_hProcess, _baseAddress, mask.c_str(), [](PSYMBOL_INFO pSymInfo,ULONG SymbolSize,PVOID UserContext) {
//		printf("0x%p %s\n", pSymInfo->Address, pSymInfo->Name);
//		return TRUE;
//		}, this);
//}
//
//void SymbolHandler::EnumTypes(string mask) {
//	SymEnumTypesByName(_hProcess, _baseAddress, mask.c_str(), [](PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext) {
//		printf("0x%p %s\n", pSymInfo->Address, pSymInfo->Name);
//		return TRUE;
//		}, this);
//}

std::unique_ptr<SymbolInfo> SymbolHandler::GetSymbolFromName(PCSTR name) {
	auto symbol = std::make_unique<SymbolInfo>();
	if (::SymFromName(m_hProcess, name, symbol->GetSymbolInfo()))
		return symbol;
	return nullptr;
}

// https://www.cnblogs.com/M-Mr/p/3970003.html
DWORD SymbolHandler::GetStructMemberOffset(std::string name,std::string memberName) {
	DWORD offset = -1;
	
	auto symbol = std::make_unique<SymbolInfo>();
	auto info = symbol->GetSymbolInfo();
	SymGetTypeFromName(m_hProcess, _address, name.c_str(), symbol->GetSymbolInfo());

	ULONG childrenCount;
	SymGetTypeInfo(m_hProcess, _address, info->TypeIndex, TI_GET_CHILDRENCOUNT, &childrenCount);
	TI_FINDCHILDREN_PARAMS* childrenParams = (TI_FINDCHILDREN_PARAMS*)malloc(sizeof(TI_FINDCHILDREN_PARAMS) + sizeof(ULONG) * childrenCount);
	if (childrenParams == nullptr) {
		return offset;
	}
	childrenParams->Count = childrenCount;
	childrenParams->Start = 0;
	PSYMBOL_INFO pMemberInfo = (PSYMBOL_INFO)malloc((sizeof(SYMBOL_INFO) +
		MAX_SYM_NAME * sizeof(TCHAR) +
		sizeof(ULONG64) - 1) /
		sizeof(ULONG64));
	if (pMemberInfo == nullptr) {
		return offset;
	}
	pMemberInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
	pMemberInfo->MaxNameLen = MAX_SYM_NAME;
	SymGetTypeInfo(m_hProcess, _address, info->TypeIndex, TI_FINDCHILDREN, childrenParams);

	for (ULONG i = 0; i < childrenParams->Count; i++) {
		ULONG child = childrenParams->ChildId[i];
		SymFromIndex(m_hProcess, _address, child, pMemberInfo);

		SymGetTypeInfo(m_hProcess, _address, child, TI_GET_OFFSET, &offset);
	}

	free(pMemberInfo);
	free(childrenParams);
	
	return offset;
}

DWORD64 SymbolHandler::LoadKernelModule(DWORD64 address) {
	auto size = 1 << 20;
	auto buffer = std::make_unique<BYTE[]>(size);
	if (0 != ::NtQuerySystemInformation(SystemModuleInformation, buffer.get(), size, nullptr))
		return 0;

	auto p = (RTL_PROCESS_MODULES*)buffer.get();
	auto count = p->NumberOfModules;
	for (ULONG i = 0; i < count; i++) {
		auto& module = p->Modules[i];
		if ((DWORD64)module.ImageBase <= address && (DWORD64)module.ImageBase + module.ImageSize > address) {
			// found the module
			//ATLTRACE(L"Kernel module for address 0x%p found: %s\n", address, CString(module.FullPathName));
			CStringA fullpath(module.FullPathName);
			fullpath.Replace("\\SystemRoot\\", "%SystemRoot%\\");
			if (fullpath.Mid(1, 2) == "??")
				fullpath = fullpath.Mid(4);
			return LoadSymbolsForModule(fullpath, (DWORD64)module.ImageBase);
		}
	}

	return 0;
}

std::unique_ptr<SymbolInfo> SymbolHandler::GetSymbolFromAddress(DWORD64 address, PDWORD64 offset) {
	auto symbol = std::make_unique<SymbolInfo>();
	if (::SymFromAddr(m_hProcess, address, offset, symbol->GetSymbolInfo())) {
		return symbol;
	}
	return nullptr;
}

BOOL SymbolHandler::Callback(ULONG code, ULONG64 data) {
	MSG msg;
	while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		::DispatchMessage(&msg);

	return 0;
}

std::unique_ptr<SymbolHandler> SymbolHandler::CreateForProcess(DWORD pid, PCSTR searchPath) {
	auto hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, pid);
	if (!hProcess)
		return nullptr;
	return std::make_unique<SymbolHandler>(hProcess, searchPath);
}

ULONG SymbolHandler::GetStructSize(std::string name) {
	auto symbol = std::make_unique<SymbolInfo>();
	auto info = symbol->GetSymbolInfo();
	SymGetTypeFromName(m_hProcess, _address, name.c_str(), info);

	return info->Size;
}