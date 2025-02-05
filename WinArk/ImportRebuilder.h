#pragma once

#include <map>
#include <PEParser.h>
#include "Thunks.h"
#include "IATReferenceScan.h"
#include <PEParser.h>

class ImportRebuilder: public PEParser{
public:
	ImportRebuilder(const WCHAR* file): PEParser(file,true) {
	}
	bool RebuildImportTable(const WCHAR* newFilePath, std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap);
	void EnableOFTSupport();
	void EnableNewIATInSection(DWORD_PTR iatAddress, DWORD iatSize);

	IATReferenceScan* _pIATReferenceScan = nullptr;
	bool _buildDirectImportsJumpTable = false;

private:
	HANDLE _hFile = INVALID_HANDLE_VALUE;
	PIMAGE_IMPORT_DESCRIPTOR _pImportDescriptor = nullptr;
	PIMAGE_THUNK_DATA _pThunkData = nullptr;
	PIMAGE_IMPORT_BY_NAME _pImportByName = nullptr;

	size_t _numberOfImportDescriptors = 0;
	size_t _sizeOfImportSection = 0;
	size_t _sizeOfApiAndModuleNames = 0;
	size_t _importSectionIndex = 0;

	// OriginalFirstThunk Array in import section
	size_t _sizeOfOFTArray = 0;
	bool _useOFT = false;
	bool _newIATInSection = false;
	DWORD_PTR _iatAddress = 0;

	DWORD _iatSize = 0;
	DWORD _sizeOfJumpTable = 0;

	DWORD _directImportsJumpTableRVA = 0;
	BYTE* _pJmpTableMemory = nullptr;
	DWORD _newIATBaseAddressRVA = 0;

	DWORD FillImportSection(std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap);
	BYTE* GetMemoryPointerFromRVA(DWORD_PTR rva);
	bool CreateNewImportSection(std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap);
	bool BuildNewImportTable(std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap);
	void SetFlagToIATSection(DWORD_PTR iatAddress);
	size_t AddImportToImportTable(ImportThunk* pImportThunk, PIMAGE_THUNK_DATA pThunkData, PIMAGE_IMPORT_BY_NAME pImportByName,
		DWORD sectionOffset);
	size_t AddImportDescriptor(ImportModuleThunk* pImportThunk, DWORD sectionOffset, DWORD sectionOffsetOFTArray);

	void CalculateImportSize(std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap);

	void AddSepecialImportDescriptor(DWORD_PTR rvaFirstThunk, DWORD sectionOffsetOFTArray);
	void PatchFileForNewIATLocation();
	void ChangeIATBaseAddress(std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap);
	void PatchFileForDirectImportJumpTable();
};