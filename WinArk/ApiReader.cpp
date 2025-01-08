#include "stdafx.h"
#include "ApiReader.h"
#include <PEParser.h>
#include "Helpers.h"

void ApiReader::ReadApisFromModuleList() {
	for (unsigned int i = 0; i < _moduleList.size(); i++) {
		SetModulePriority(&_moduleList[i]);
		if (_moduleList[i]._modBaseAddr + _moduleList[i]._modBaseSize > _maxValidAddress) {
			_maxValidAddress = _moduleList[i]._modBaseAddr + _moduleList[i]._modBaseSize;
		}
		if (!_moduleList[i]._isAlreadyParsed) {
			ParseModule(&_moduleList[i]);
		}
	}
}

void ApiReader::ParseModule(ModuleInfo* pModule) {
	pModule->_parsing = true;
	if (IsWinSxSModule(pModule)) {
		ParseModuleWithMapping(pModule);
	}
	else if (IsModuleLoadedInOwnProcess(pModule)) {
		ParseModuleWithOwnProcess(pModule);
	}
	else {
		if (_readExportTableAlwaysFromDisk) {
			ParseModuleWithMapping(pModule);
		}
		else {
			ParseModuleWithProcess(pModule);
		}
	}
}

void ApiReader::ParseModuleWithMapping(ModuleInfo* pModule) {
	ParseExportTable(pModule, true, true);
}

void ApiReader::ParseExportTable(ModuleInfo* pModule,bool isMapping,bool ownProcess) {
	if (isMapping) {
		PEParser parser(pModule->_fullPath);
		auto exports = parser.GetExports();
		for (ExportedSymbol symbol : exports) {
			if (!symbol.IsForward) {
				if (symbol.HasName)
					AddApi(symbol.Name.c_str(), symbol.Hint, symbol.Ordinal,
						symbol.Address, symbol.Address + pModule->_modBaseAddr, false, pModule);
				else {
					AddApiWithoutName(symbol.Ordinal, symbol.Address, symbol.Address + pModule->_modBaseAddr, false, pModule);
				}
			}
			else {
				HandleForwardedApi(symbol.ForwardName.c_str(), symbol.Name.c_str(),
					symbol.Address, symbol.Ordinal, pModule);
			}
		}
	}
	else if(!ownProcess) {
		BYTE* pPE = nullptr;
		pPE = new BYTE[pModule->_modBaseSize];
		if (!ReadMemoryFromProcess(pModule->_modBaseAddr, pModule->_modBaseSize, pPE)) {
			delete[] pPE;
			return;
		}

		PEParser parser(pPE);
		auto exports = parser.GetExports();
		for (ExportedSymbol symbol : exports) {
			if (!symbol.IsForward) {
				if (symbol.HasName)
					AddApi(symbol.Name.c_str(), symbol.Hint, symbol.Ordinal,
						symbol.Address, symbol.Address + pModule->_modBaseAddr, false, pModule);
				else {
					AddApiWithoutName(symbol.Ordinal, symbol.Address, symbol.Address + pModule->_modBaseAddr, false, pModule);
				}
			}
			else {
				HandleForwardedApi(symbol.ForwardName.c_str(), symbol.Name.c_str(),
					symbol.Address, symbol.Ordinal, pModule);
			}
		}
		delete[] pPE;
	}
	else {
		BYTE* pPE = (BYTE*)GetModuleHandle(pModule->GetFileName());
		PEParser parser(pPE);
		auto exports = parser.GetExports();
		for (ExportedSymbol symbol : exports) {
			if (!symbol.IsForward) {
				if (symbol.HasName)
					AddApi(symbol.Name.c_str(), symbol.Hint, symbol.Ordinal,
						symbol.Address, symbol.Address + pModule->_modBaseAddr, false, pModule);
				else {
					AddApiWithoutName(symbol.Ordinal, symbol.Address, symbol.Address + pModule->_modBaseAddr, false, pModule);
				}
			}
			else {
				HandleForwardedApi(symbol.ForwardName.c_str(), symbol.Name.c_str(),
					symbol.Address, symbol.Ordinal, pModule);
			}
		}
	}
}

void ApiReader::FindApiByModuleAndOrdinal(ModuleInfo* pModule, WORD ordinal, DWORD_PTR* pVA, DWORD_PTR* pRVA)
{
	FindApiByModule(pModule, nullptr, ordinal, pVA, pRVA);
}

void ApiReader::FindApiByModuleAndName(ModuleInfo* pModule, char* searchName, DWORD_PTR* pVA, DWORD_PTR* pRVA)
{
	FindApiByModule(pModule, searchName, 0, pVA, pRVA);
}

void ApiReader::FindApiByModule(ModuleInfo* pModule, char* searchName, WORD ordinal, DWORD_PTR* pVA, DWORD_PTR* pRVA)
{
	if (IsModuleLoadedInOwnProcess(pModule)) {
		HMODULE hModule = GetModuleHandle(pModule->GetFileName());
		if (hModule) {
			if (pVA) {
				if (ordinal) {
					*pVA = (DWORD_PTR)GetProcAddress(hModule, (LPCSTR)ordinal);
				}
				else {
					*pVA = (DWORD_PTR)GetProcAddress(hModule, searchName);
				}
				*pRVA = *pVA - (DWORD_PTR)hModule;
				*pVA = (*pRVA) + pModule->_modBaseAddr;
			}
		}
	}
	else {
		FindApiInProcess(pModule, searchName, ordinal, pVA, pRVA);
	}
}


bool ApiReader::IsModuleLoadedInOwnProcess(ModuleInfo* pModule) {
	for (unsigned int i = 0; i < _ownModuleList.size(); i++) {
		if (!_wcsicmp(pModule->_fullPath, _ownModuleList[i]._fullPath))
			return true;
	}
	return false;
}

bool ApiReader::IsPEAndExportTableValid(PIMAGE_NT_HEADERS pNtHeader) {
	if (pNtHeader->Signature != IMAGE_NT_SIGNATURE)
		return false;
	else if (pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress == 0
		|| pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size == 0)
		return false;
	else
		return true;
}

void ApiReader::ParseModuleWithOwnProcess(ModuleInfo* pModule) {
	HMODULE hModule = GetModuleHandle(pModule->GetFileName());
	if (hModule) {
		ParseExportTable(pModule, false, true);
	}
}

void ApiReader::FindApiInProcess(ModuleInfo* pModule, char* searchName, WORD ordinal, DWORD_PTR* pVA, DWORD_PTR* pRVA) {
	PIMAGE_NT_HEADERS pNtHeader = nullptr;
	PIMAGE_DOS_HEADER pDosHeader = nullptr;
	BYTE* pPE = nullptr;
	PIMAGE_EXPORT_DIRECTORY pExportTable = nullptr;

	pPE = GetHeaderFromProcess(pModule);
	if (pPE == nullptr)
		return;

	pDosHeader = (PIMAGE_DOS_HEADER)pPE;
	pNtHeader = (PIMAGE_NT_HEADERS)((BYTE*)pPE + pDosHeader->e_lfanew);
	if (IsPEAndExportTableValid(pNtHeader)) {
		pExportTable = GetExportTableFromProcess(pModule, pNtHeader);
		if (pExportTable) {
			FindApiInExportTable(pModule, pExportTable,
				(DWORD_PTR)pExportTable - pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,
				searchName, ordinal, pVA, pRVA);
			delete[] pExportTable;
		}
	}

	delete[] pPE;
}

bool ApiReader::FindApiInExportTable(ModuleInfo* pModule, PIMAGE_EXPORT_DIRECTORY pExportDir, 
	DWORD_PTR deltaAddress, char* searchName,WORD ordinal, DWORD_PTR* pVA, DWORD_PTR* pRVA)
{
	DWORD* pAddressOfFunctions = nullptr, * pAddressOfNames = nullptr;
	WORD* pAddressOfNameOrdinals = nullptr;
	char* pFunctionName = nullptr;

	pAddressOfFunctions = (DWORD*)((DWORD_PTR)pExportDir->AddressOfFunctions + deltaAddress);
	pAddressOfNames = (DWORD*)((DWORD_PTR)pExportDir->AddressOfNames + deltaAddress);
	pAddressOfNameOrdinals = (WORD*)((DWORD_PTR)pExportDir->AddressOfNameOrdinals + deltaAddress);

	if (searchName) {
		for (DWORD i = 0; i < pExportDir->NumberOfNames; i++) {
			pFunctionName = (char*)(pAddressOfNames[i] + deltaAddress);
			if (!strcmp(pFunctionName, searchName)) {
				*pRVA = pAddressOfFunctions[pAddressOfNameOrdinals[i]];
				*pVA = *pRVA + pModule->_modBaseAddr;
				return true;
			}
		}
	}
	else {
		for (DWORD i = 0; i < pExportDir->NumberOfFunctions; i++)
		{
			if (ordinal == (i + pExportDir->Base))
			{
				*pRVA = pAddressOfFunctions[i];
				*pVA = *pRVA + pModule->_modBaseAddr;
				return true;
			}
		}
	}

	return false;
}

BYTE* ApiReader::GetHeaderFromProcess(ModuleInfo* pModule)
{
	BYTE* pPE = nullptr;
	DWORD readSize = 0;
	if (pModule->_modBaseSize < PE_HEADER_BYTES_COUNT) {
		readSize = pModule->_modBaseSize;
	}
	else {
		readSize = PE_HEADER_BYTES_COUNT;
	}

	pPE = new BYTE[readSize];
	if (!ReadMemoryFromProcess(pModule->_modBaseAddr, readSize, pPE)) {
		delete[] pPE;
		return nullptr;
	}
	else {
		return pPE;
	}
}

PIMAGE_EXPORT_DIRECTORY ApiReader::GetExportTableFromProcess(ModuleInfo* pModule, PIMAGE_NT_HEADERS pNtHeader) {
	DWORD readSize = 0;
	BYTE* pExportTable = nullptr;

	readSize = pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

	DWORD minSize = sizeof(IMAGE_EXPORT_DIRECTORY);
	if (readSize < minSize + 8) {
		readSize = minSize + 100;
	}
	if (readSize) {
		pExportTable = new BYTE[readSize];
		if (!pExportTable) {
			return nullptr;
		}
		DWORD rva = pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
		if (!ReadMemoryFromProcess(pModule->_modBaseAddr + rva, readSize, pExportTable)) {
			delete[] pExportTable;
			return nullptr;
		}
		return  (PIMAGE_EXPORT_DIRECTORY)pExportTable;
	}
	return nullptr;
}

void ApiReader::SetModulePriority(ModuleInfo* pModule) {
	const WCHAR* pName = pModule->GetFileName();
	if (!_wcsicmp(pName, L"kernelbase.dll")) {
		pModule->_priority = -1;
	}
	else if (!_wcsicmp(pName, L"ntdll.dll")) {
		pModule->_priority = 0;
	}
	else if (!_wcsicmp(pName, L"shlwapi.dll"))
	{
		pModule->_priority = 0;
	}
	else if (!_wcsicmp(pName, L"ShimEng.dll"))
	{
		pModule->_priority = 0;
	}
	else if (!_wcsicmp(pName, L"kernel32.dll"))
	{
		pModule->_priority = 2;
	}
	else if (!_wcsnicmp(pName, L"API-", 4) || !_wcsnicmp(pName, L"EXT-", 4)) //API_SET_PREFIX_NAME, API_SET_EXTENSION
	{
		pModule->_priority = 0;
	}
	else
	{
		pModule->_priority = 1;
	}
}


void ApiReader::SetMinMaxApiAddress(DWORD_PTR address) {
	if (address == 0 || address == (DWORD_PTR)-1)
		return;

	if (address < _minApiAddress) {
		_minApiAddress = address - 1;
	}
	if (address > _maxApiAddress) {
		_maxApiAddress = address + 1;
	}
}

bool ApiReader::AddModuleToModuleList(const WCHAR* pModuleName, DWORD_PTR firstThunk) {
	ImportModuleThunk thunk;
	thunk.m_FirstThunk = firstThunk;
	wcscpy_s(thunk.m_ModuleName, pModuleName);

	_moduleThunkMap->insert(std::pair<DWORD_PTR, ImportModuleThunk>(firstThunk, thunk));

	return true;
}

bool ApiReader::AddFunctionToModuleList(ApiInfo* pApiFound, DWORD_PTR va, DWORD_PTR rva, 
	WORD ordinal, bool valid, bool suspect)
{
	ImportThunk thunk;
	ImportModuleThunk* pModuleThunk = nullptr;
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iter;

	if (_moduleThunkMap->size() > 1) {
		iter = _moduleThunkMap->begin();
		while (iter != _moduleThunkMap->end()) {
			if (rva >= iter->second.m_FirstThunk) {
				iter++;
				if (iter == _moduleThunkMap->end()) {
					iter--;
					pModuleThunk = &iter->second;
					break;
				}
				else if (rva < iter->second.m_FirstThunk) {
					iter--;
					pModuleThunk = &iter->second;
					break;
				}
			}
			else {
				break;
			}
		}
	}
	else {
		iter = _moduleThunkMap->begin();
		pModuleThunk = &iter->second;
	}

	if (!pModuleThunk) {
		return false;
	}

	thunk.m_Suspect = suspect;
	thunk.m_Valid = valid;
	thunk.m_VA = va;
	thunk.m_RVA = rva;
	thunk.m_ApiAddressVA = pApiFound->VA;
	thunk.m_Ordinal = ordinal;
	thunk.m_Hint = pApiFound->Hint;

	wcscpy_s(thunk.m_ModuleName, pApiFound->pModule->GetFileName());
	strcpy_s(thunk.m_Name, pApiFound->Name);

	pModuleThunk->m_ThunkMap.insert(std::pair<DWORD_PTR, ImportThunk>(thunk.m_RVA, thunk));
	return true;
}

bool ApiReader::AddNotFoundApiToModuleList(DWORD_PTR iat, DWORD_PTR apiAddress)
{
	ImportThunk thunk;
	ImportModuleThunk* pModuleThunk = nullptr;
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iter;
	DWORD_PTR rva = iat - _targetImageBase;

	if (_moduleThunkMap->size() > 0) {
		iter = _moduleThunkMap->begin();
		while (iter != _moduleThunkMap->end()) {
			if (rva >= iter->second.m_FirstThunk) {
				iter++;
				if (iter == _moduleThunkMap->end()) {
					iter--;
					if (iter->second.m_ModuleName[0] == L'?') {
						pModuleThunk = &iter->second;
					}
					else {
						AddUnknownModuleToModuleList(rva);
						pModuleThunk = &(_moduleThunkMap->find(rva)->second);
					}
					break;
				}
				else if (rva < iter->second.m_FirstThunk) {
					iter--;
					pModuleThunk = &iter->second;
					break;
				}
			}
			else {
				break;
			}
		}
	}
	else {
		AddUnknownModuleToModuleList(rva);
		pModuleThunk = &(_moduleThunkMap->find(rva)->second);
	}

	if (!pModuleThunk) {
		return false;
	}

	thunk.m_Suspect = true;
	thunk.m_Valid = false;
	thunk.m_VA = iat;
	thunk.m_RVA = rva;
	thunk.m_ApiAddressVA = apiAddress;
	thunk.m_Ordinal = 0;

	wcscpy_s(thunk.m_ModuleName, L"?");
	strcpy_s(thunk.m_Name, "?");

	pModuleThunk->m_ThunkMap.insert(std::pair<DWORD_PTR, ImportThunk>(thunk.m_RVA, thunk));

	return true;
}

bool ApiReader::IsApiBlacklisted(const char* name) {
	return false;
}

bool ApiReader::IsWinSxSModule(ModuleInfo* pModule) {
	if (wcsstr(pModule->_fullPath, L"\\WinSxS\\"))
		return true;
	else if (wcsstr(pModule->_fullPath, L"\\winsxs\\"))
		return true;

	return false;
}

ApiInfo* ApiReader::GetScoredApi(std::unordered_map<DWORD_PTR, ApiInfo*>::iterator iter, size_t countDuplicates,
	bool hasName, bool hasUnicodeAnsiName, bool hasNoUnderlineInName,
	bool hasPrioDll, bool hasPrio0Dll, bool hasPrio1Dll,
	bool hasPrio2Dll, bool firstWin) {

	ApiInfo* pFoundApi = nullptr;
	ApiInfo* pFoundMatchingApi = nullptr;
	int count = 0;
	int scoreNeeded = 0;
	int scoreValue = 0;
	size_t apiNameLength;

	if (hasUnicodeAnsiName || hasNoUnderlineInName) {
		hasName = true;
	}

	if (hasName)
		scoreNeeded++;

	if (hasUnicodeAnsiName)
		scoreNeeded++;

	if (hasNoUnderlineInName)
		scoreNeeded++;

	if (hasPrioDll)
		scoreNeeded++;

	if (hasPrio0Dll)
		scoreNeeded++;

	if (hasPrio1Dll)
		scoreNeeded++;

	if (hasPrio2Dll)
		scoreNeeded++;

	for (size_t i = 0; i < countDuplicates; i++,iter++) {
		pFoundApi = iter->second;
		scoreValue = 0;

		if (hasName) {
			if (pFoundApi->Name[0] != 0x00) {
				scoreValue++;
				if (hasUnicodeAnsiName)
				{
					apiNameLength = strlen(pFoundApi->Name);
					if ((pFoundApi->Name[apiNameLength - 1] == 'W')
						|| (pFoundApi->Name[apiNameLength - 1] == 'A'))
						scoreValue++;
				}

				if (hasNoUnderlineInName) {
					if (!strrchr(pFoundApi->Name, '_')) {
						scoreValue++;
					}
				}
			}
		}

		if (hasPrioDll)
		{
			if (pFoundApi->pModule->_priority >= 1)
			{
				scoreValue++;
			}
		}

		if (hasPrio0Dll)
		{
			if (pFoundApi->pModule->_priority == 0)
			{
				scoreValue++;
			}
		}

		if (hasPrio1Dll)
		{
			if (pFoundApi->pModule->_priority == 1)
			{
				scoreValue++;
			}
		}

		if (hasPrio2Dll)
		{
			if (pFoundApi->pModule->_priority == 2)
			{
				scoreValue++;
			}
		}


		if (scoreValue == scoreNeeded)
		{
			pFoundMatchingApi = pFoundApi;
			count++;

			if (firstWin)
			{
				return pFoundMatchingApi;
			}
		}
	}

	if (count == 1) {
		return pFoundMatchingApi;
	}
	else {
		return nullptr;
	}
}

bool ApiReader::IsApiAddressValid(DWORD_PTR virtualAddress) {
	return _apiMap.count(virtualAddress) > 0;
}

ApiInfo* ApiReader::GetApiByVirtualAddress(DWORD_PTR virtualAddress, bool* pIsSuspect){
	std::unordered_map<DWORD_PTR, ApiInfo*>::iterator iter;
	size_t i = 0;
	size_t countDuplicates = _apiMap.count(virtualAddress);
	int countHighPriority = 0;
	ApiInfo* pApiFound = nullptr;

	if (countDuplicates == 0)
		return nullptr;
	else if (countDuplicates == 1) {
		*pIsSuspect = false;
		iter = _apiMap.find(virtualAddress);
		return iter->second;
	}
	else {
		iter = _apiMap.find(virtualAddress);

		pApiFound = GetScoredApi(iter, countDuplicates, true, false, false, true, false, false, false, false);
		if (pApiFound)
			return pApiFound;

		*pIsSuspect = true;
		//high priority with a name and ansi/unicode name
		pApiFound = GetScoredApi(iter, countDuplicates, true, true, false, true, false, false, false, false);

		if (pApiFound)
			return pApiFound;

		//priority 2 with no underline in name
		pApiFound = GetScoredApi(iter, countDuplicates, true, false, true, false, false, false, true, false);

		if (pApiFound)
			return pApiFound;

		//priority 1 with a name
		pApiFound = GetScoredApi(iter, countDuplicates, true, false, false, false, false, true, false, false);

		if (pApiFound)
			return pApiFound;

		//With a name
		pApiFound = GetScoredApi(iter, countDuplicates, true, false, false, false, false, false, false, false);

		if (pApiFound)
			return pApiFound;

		//any with priority, name, ansi/unicode
		pApiFound = GetScoredApi(iter, countDuplicates, true, true, false, true, false, false, false, true);

		if (pApiFound)
			return pApiFound;

		//any with priority
		pApiFound = GetScoredApi(iter, countDuplicates, false, false, false, true, false, false, false, true);

		if (pApiFound)
			return pApiFound;

		//has prio 0 and name
		pApiFound = GetScoredApi(iter, countDuplicates, false, false, false, false, true, false, false, true);

		if (pApiFound)
			return pApiFound;
	}

	return nullptr;
}


void ApiReader::ReadAndParseIAT(DWORD_PTR iat, DWORD size, std::map<DWORD_PTR, ImportModuleThunk>& moduleListNew)
{
	_moduleThunkMap = &moduleListNew;
	BYTE* pIAT = new BYTE[size];
	if (ReadMemoryFromProcess(iat, size, pIAT)) {
		ParseIAT(iat, pIAT, size);
	}

	delete[] pIAT;
}

void ApiReader::AddFoundApiToModuleList(DWORD_PTR iat, ApiInfo* pApiFound, bool isNewModule, bool isSuspect)
{
	if (isNewModule) {
		AddModuleToModuleList(pApiFound->pModule->GetFileName(), iat - _targetImageBase);
	}
	AddFunctionToModuleList(pApiFound, iat, iat - _targetImageBase, pApiFound->Oridinal, true, isSuspect);
}

void ApiReader::ClearAll() {
	_minApiAddress = (DWORD_PTR)-1;
	_maxApiAddress = 0;

	for (auto iter : _apiMap) {
		delete iter.second;
	}

	_apiMap.clear();
	if (_moduleThunkMap != nullptr) {
		_moduleThunkMap->clear();
	}
}

bool ApiReader::IsInvalidMemoryForIAT(DWORD_PTR address)
{
	if (address == 0 || address == -1)
		return true;
	MEMORY_BASIC_INFORMATION mbi = { 0 };

	if (!::VirtualQueryEx(_hProcess, (LPVOID)address, &mbi, sizeof(MEMORY_BASIC_INFORMATION)))
	{
		return false;
	}

	if(mbi.State == MEM_COMMIT && IsPageAccessable(mbi.Protect))
		return true;

	return false;
}

void ApiReader::ParseIAT(DWORD_PTR iat, BYTE* pIAT, SIZE_T size) {
	ApiInfo* pApiFound = nullptr;
	ModuleInfo* pModule = nullptr;
	bool isSuspect = false;
	int countApiFound = 0, countApiNotFound = 0;
	DWORD_PTR* pIATAddress = (DWORD_PTR*)pIAT;
	SIZE_T iatSize = size / sizeof(DWORD_PTR);

	for (SIZE_T i = 0; i < iatSize; i++) {
		if (!IsInvalidMemoryForIAT(pIATAddress[i])) {
			if (pIATAddress[i] > _minApiAddress && pIATAddress[i] < _maxApiAddress) {
				pApiFound = GetApiByVirtualAddress(pIATAddress[i], &isSuspect);
				if (pApiFound != nullptr) {
					countApiFound++;
					if (pModule != pApiFound->pModule) {
						pModule = pApiFound->pModule;
						AddFoundApiToModuleList(iat + (DWORD_PTR)&pIATAddress[i] - (DWORD_PTR)pIAT, pApiFound, true, isSuspect);
					}
					else {
						AddFoundApiToModuleList(iat + (DWORD_PTR)&pIATAddress[i] - (DWORD_PTR)pIAT, pApiFound, false, isSuspect);
					}
				}
			}
			else {
				countApiNotFound++;
				AddNotFoundApiToModuleList(iat + (DWORD_PTR)&pIATAddress[i] - (DWORD_PTR)pIAT, pIATAddress[i]);
			}
		}
	}
}

void ApiReader::AddApi(const char* pName, WORD hint, WORD ordinal, 
	DWORD_PTR va, DWORD_PTR rva, bool isForward, ModuleInfo* pInfo)
{
	ApiInfo* pApiInfo = new ApiInfo();
	if (pName != nullptr && (strlen(pName) < _countof(pApiInfo->Name))) {
		strcpy_s(pApiInfo->Name, pName);
	}
	else {
		pApiInfo->Name[0] = 0x00;
	}

	pApiInfo->Oridinal = ordinal;
	pApiInfo->IsForwarded = isForward;
	pApiInfo->pModule = pInfo;
	pApiInfo->RVA = rva;
	pApiInfo->VA = va;
	pApiInfo->Hint = hint;

	SetMinMaxApiAddress(va);

	pInfo->_apiList.push_back(pApiInfo);

	_apiMap.insert(API_Pair(va, pApiInfo));
}

void ApiReader::AddApiWithoutName(WORD ordinal, DWORD_PTR va, DWORD_PTR rva, bool isForward, ModuleInfo* pInfo) {
	AddApi(nullptr, 0, ordinal, va, rva, isForward, pInfo);
}

bool ApiReader::IsApiForwarded(DWORD_PTR rva, PIMAGE_NT_HEADERS pNtHeader) {
	DWORD exportRVA = pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	DWORD size = pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
	if((rva > exportRVA) && (rva < exportRVA + size)) {
		return true;
	}
	else {
		return false;
	}
}

void ApiReader::HandleForwardedApi(const char* pForwardName, const char* pFunctionNameParent, DWORD_PTR rvaParent,
	WORD ordinalParent, ModuleInfo* pModuleParent) {
	size_t dllNameLen = 0;
	WORD ordinal = 0;
	ModuleInfo* pModule = nullptr;
	DWORD_PTR vaApi = 0;
	DWORD_PTR rvaApi = 0;
	char dllName[MAX_PATH] = { 0 };
	WCHAR dllNameW[MAX_PATH] = { 0 };
	const char* pSearchFunctionName = strchr(pForwardName, '.');

	if (!pSearchFunctionName)
		return;

	dllNameLen = pSearchFunctionName - pForwardName;

	if (dllNameLen >= 99) {
		return;
	}
	else {
		strncpy_s(dllName, pForwardName, dllNameLen);
	}

	pSearchFunctionName++;

	if (strchr(pSearchFunctionName, '#')) {
		pSearchFunctionName++;
		ordinal = (WORD)atoi(pSearchFunctionName);
	}

	if (!_strnicmp(dllName, "API-", 4) || !_strnicmp(dllName, "EXT-", 4)) {
		FARPROC addr = nullptr;
		HMODULE hModTemp = GetModuleHandleA(dllName);
		if (hModTemp == NULL) {
			hModTemp = LoadLibraryA(dllName);
		}
		if (ordinal) {
			addr = GetProcAddress(hModTemp, (char*)ordinal);
		}
		else {
			addr = GetProcAddress(hModTemp, pSearchFunctionName);
		}

		if (addr != nullptr) {
			AddApi(pFunctionNameParent, 0, ordinalParent, (DWORD_PTR)addr,
				(DWORD_PTR)addr - (DWORD_PTR)hModTemp, true, pModuleParent);
		}

		return;
	}

	strcat_s(dllName, ".dll");

	std::wstring wstr = Helpers::StringToWstring(dllName);
	wcscpy_s(dllNameW, wstr.c_str());

	if (!_wcsicmp(dllNameW, pModuleParent->GetFileName())) {
		pModule = pModuleParent;
	}
	else {
		pModule = FindModuleByName(dllNameW);
	}

	if (pModule != nullptr) {
		if (ordinal) {
			FindApiByModuleAndOrdinal(pModule, ordinal, &vaApi, &rvaApi);
		}
		else {
			FindApiByModuleAndName(pModule, (char*)pSearchFunctionName, &vaApi, &rvaApi);
		}

		if (rvaApi != 0) {
			AddApi(pFunctionNameParent, 0, ordinalParent, vaApi, rvaApi, true, pModuleParent);
		}
	}
}

void ApiReader::ParseModuleWithProcess(ModuleInfo* pModule) {
	ParseExportTable(pModule, false);
}

ModuleInfo* ApiReader::FindModuleByName(WCHAR* name) {
	for (unsigned int i = 0; i < _moduleList.size(); i++) {
		if (!_wcsicmp(_moduleList[i].GetFileName(), name))
			return &_moduleList[i];
	}
	return nullptr;
}

void ApiReader::AddUnknownModuleToModuleList(DWORD_PTR firstThunk) {
	ImportModuleThunk thunk;

	thunk.m_FirstThunk = firstThunk;
	wcscpy_s(thunk.m_ModuleName, L"?");

	_moduleThunkMap->insert(std::pair<DWORD_PTR, ImportModuleThunk>(firstThunk, thunk));
}