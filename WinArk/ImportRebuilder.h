#pragma once

#include <map>
#include <PEParser.h>
#include "Thunks.h"
#include "IATReferenceScan.h"
#include <PEParser.h>

class ImportRebuilder: public PEParser{
public:
	ImportRebuilder(const WCHAR* file): PEParser(file) {
	}
	bool RebuildImportTable(const WCHAR* newFilePath, std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap);
	void EnableOFTSupport();
	void EnableNewIATInSection(DWORD_PTR iatAddress, DWORD iatSize);

private:
	PIMAGE_IMPORT_DESCRIPTOR _pImportDescriptor = nullptr;
	PIMAGE_THUNK_DATA _pThunkData = nullptr;
	PIMAGE_IMPORT_BY_NAME _pImportByName = nullptr;

	size_t _numberOfImportDescriptors;
	size_t _sizeOfImportSection;
	size_t _sizeOfApiAndModuleNames;
	size_t _importSectionIndex;

	// OriginalFirstThunk Array in import section
	size_t _sizeOfOFTArray;
	bool _useOFT;
	bool _newIATInSection;
	DWORD_PTR _iatAddress;

	DWORD _iatSize;
	DWORD _sizeOfJumpTable;

	DWORD _directImportsJumpTableRVA;
	BYTE* _pJmpTableMemory;
	DWORD _newIATBaseAddressRVA;

	DWORD FillImportSection(std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap);
	BYTE* GetMemoryPointerFromRVA(DWORD_PTR rva);
	bool CreateNewImportSection(std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap);
	bool BuildNewImportTable(std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap);
	void SetFlagToIATSection(DWORD_PTR iatAddress);
	size_t AddImportToImportTable(ImportThunk* pImportThunk, PIMAGE_THUNK_DATA* pThunkData, PIMAGE_IMPORT_BY_NAME pImportByName,
		DWORD sectionOffset);
	size_t AddImportDescriptor(ImportModuleThunk* pImportThunk, DWORD sectionOffset, DWORD sectionOffsetOFTArray);

	void CalculateImportSize(std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap);

	void AddSepecialImportDescriptor(DWORD_PTR rvaFirstThunk, DWORD sectionOffsetOFTArray);
	void PatchFileForNewIATLocation();
	void ChangeIATBaseAddress(std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap);
	void PatchFileForDirectImportJumpTable();
};