#include "pch.h"
#include "PdbParser.h"


using namespace std;
SymbolHandler::SymbolHandler(HANDLE hProcess, bool ownHandle)
	:_hProcess(hProcess), _ownHandle(ownHandle) {
	SymSetOptions(SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME);
	SymInitialize(hProcess, nullptr, false);
}

void SymbolHandler::LoadSymbolsForModule(std::string imageName, DWORD64 dllBase, HANDLE hFile) {
	_baseAddress = SymLoadModuleEx(_hProcess, nullptr, imageName.c_str(),nullptr, dllBase, 0, nullptr, 0);
}



void SymbolHandler::EnumSymbols(string mask) {
	SymEnumSymbols(_hProcess, _baseAddress, mask.c_str(), [](PSYMBOL_INFO pSymInfo,ULONG SymbolSize,PVOID UserContext) {
		printf("0x%p %s\n", pSymInfo->Address, pSymInfo->Name);
		return TRUE;
		}, this);
}

void SymbolHandler::EnumTypes(string mask) {
	SymEnumTypesByName(_hProcess, _baseAddress, mask.c_str(), [](PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext) {
		printf("0x%p %s\n", pSymInfo->Address, pSymInfo->Name);
		return TRUE;
		}, this);
}

ULONG64 SymbolHandler::GetSymbolAddressFromName(std::string name) {
	ULONG64 buffer[(sizeof(SYMBOL_INFO) +
		MAX_SYM_NAME * sizeof(TCHAR) +
		sizeof(ULONG64) - 1) /
		sizeof(ULONG64)];
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYM_NAME;
	bool success = SymFromName(_hProcess, name.c_str(), pSymbol);
	return success ? pSymbol->Address : 0;
}

// https://www.cnblogs.com/M-Mr/p/3970003.html
DWORD SymbolHandler::GetStructMemberOffset(std::string name,std::string memberName) {
	DWORD offset = -1;
	ULONG64 buffer[(sizeof(SYMBOL_INFO) +
		MAX_SYM_NAME * sizeof(TCHAR) +
		sizeof(ULONG64) - 1) /
		sizeof(ULONG64)];

	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYM_NAME;
	SymGetTypeFromName(_hProcess, _baseAddress, name.c_str(), pSymbol);

	ULONG childrenCount;
	SymGetTypeInfo(_hProcess, _baseAddress, pSymbol->TypeIndex, TI_GET_CHILDRENCOUNT, &childrenCount);
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
	SymGetTypeInfo(_hProcess, _baseAddress, pSymbol->TypeIndex, TI_FINDCHILDREN, childrenParams);

	for (ULONG i = 0; i < childrenParams->Count; i++) {
		ULONG child = childrenParams->ChildId[i];
		SymFromIndex(_hProcess, _baseAddress, child, pMemberInfo);

		SymGetTypeInfo(_hProcess, _baseAddress, child, TI_GET_OFFSET, &offset);
	}

	free(pMemberInfo);
	free(childrenParams);
	
	return offset;
}

