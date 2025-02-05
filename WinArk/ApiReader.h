#pragma once

#include "Thunks.h"
#include "ProcessAccessHelper.h"

typedef std::pair<DWORD_PTR, ApiInfo*> API_Pair;

class ApiReader : public ProcessAccessHelper
{
public:
	// api look up table
	static inline std::unordered_map<DWORD_PTR, ApiInfo*> _apiMap;

	// store found apis
	static inline std::map<DWORD_PTR, ImportModuleThunk>* _moduleThunkMap;

	static inline DWORD_PTR _minApiAddress = -1;
	static inline DWORD_PTR _maxApiAddress = 0;

	/*
	* Read all APIs from target process
	*/
	void ReadApisFromModuleList();

	bool IsApiAddressValid(DWORD_PTR virtualAddress);
	ApiInfo* GetApiByVirtualAddress(DWORD_PTR virtualAddress, bool* pIsSuspect);
	void ReadAndParseIAT(DWORD_PTR iat, DWORD size, std::map<DWORD_PTR, ImportModuleThunk>& moduleListNew);
	void AddFoundApiToModuleList(DWORD_PTR iat, ApiInfo* pApiFound, bool isNewModule, bool isSuspect);
	void ClearAll();
	bool IsInvalidMemoryForIAT(DWORD_PTR address);
private:
	bool _readExportTableAlwaysFromDisk = false;
	void ParseIAT(DWORD_PTR iat, BYTE* pIAT, SIZE_T size);

	void AddApi(const char* pName, WORD hint, WORD ordinal, DWORD_PTR va, DWORD_PTR rva, bool isForward, ModuleInfo* pInfo);
	void AddApiWithoutName(WORD ordinal, DWORD_PTR va, DWORD_PTR rva, bool isForward, ModuleInfo* pInfo);
	inline bool IsApiForwarded(DWORD_PTR rva, PIMAGE_NT_HEADERS pNtHeader);
	void HandleForwardedApi(const char* pForwardName, const char* pFunctionNameParent, DWORD_PTR rvaParent, WORD ordinalParent, ModuleInfo* pModuleParent);
	void ParseModule(ModuleInfo* pModule);
	void ParseModuleWithProcess(ModuleInfo* pModule);
	void ParseExportTable(ModuleInfo* pModule, bool isMapping, bool ownProcess = false);
	
	ModuleInfo* FindModuleByName(WCHAR* name);

	void FindApiByModuleAndOrdinal(ModuleInfo* pModule, WORD ordinal, DWORD_PTR* pVA, DWORD_PTR* pRVA);
	void FindApiByModuleAndName(ModuleInfo* pModule, char* searchName, DWORD_PTR* pVA, DWORD_PTR* pRVA);
	void FindApiByModule(ModuleInfo* pModule, char* searchName, WORD ordinal, DWORD_PTR* pVA, DWORD_PTR* pRVA);

	bool IsModuleLoadedInOwnProcess(ModuleInfo* pModule);
	void ParseModuleWithOwnProcess(ModuleInfo* pModule);
	bool IsPEAndExportTableValid(PIMAGE_NT_HEADERS pNtHeader);
	void FindApiInProcess(ModuleInfo* pModule, char* searchName, WORD ordinal, DWORD_PTR* pVA, DWORD_PTR* pRVA);

	BYTE* GetHeaderFromProcess(ModuleInfo* pModule);
	PIMAGE_EXPORT_DIRECTORY GetExportTableFromProcess(ModuleInfo* pModule, PIMAGE_NT_HEADERS pNtHeader);

	void SetModulePriority(ModuleInfo* pModule);
	void SetMinMaxApiAddress(DWORD_PTR address);

	void ParseModuleWithMapping(ModuleInfo* pModule);

	bool AddModuleToModuleList(const WCHAR* pModuleName, DWORD_PTR firstThunk);
	bool AddFunctionToModuleList(ApiInfo* pApiFound, DWORD_PTR va, DWORD_PTR rva, WORD ordinal, bool valid, bool suspect);
	bool AddNotFoundApiToModuleList(DWORD_PTR iat, DWORD_PTR apiAddress);

	void AddUnknownModuleToModuleList(DWORD_PTR firstThunk);
	bool IsApiBlacklisted(const char* name);
	bool IsWinSxSModule(ModuleInfo* pModule);

	ApiInfo* GetScoredApi(std::unordered_map<DWORD_PTR, ApiInfo*>::iterator iter, size_t countDuplicates,
		bool hasName, bool hasUnicodeAnsiName, bool hasNoUnderlineInName,
		bool hasPrioDll, bool hasPrio0Dll, bool hasPrio1Dll,
		bool hasPrio2Dll, bool firstWin);
};