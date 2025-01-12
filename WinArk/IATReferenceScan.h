#pragma once

#include "ProcessAccessHelper.h"
#include "ApiReader.h"
#include <PEParser.h>

enum IATReferenceType {
	IAT_REFERENCE_PTR_JMP,
	IAT_REFERENCE_PTR_CALL,
	IAT_REFERENCE_DIRECT_JMP,
	IAT_REFERENCE_DIRECT_CALL,
	IAT_REFERENCE_DIRECT_MOV,
	IAT_REFERENCE_DIRECT_PUSH,
	IAT_REFERENCE_DIRECT_LEA
};

class IATReference {
public:
	DWORD_PTR _addressVA;
	DWORD_PTR _targetPointer;
	DWORD_PTR _targetAddressInIAT;
	BYTE _instSize;
	IATReferenceType _type;
};

class IATReferenceScan {
public:
	bool _scanForDirectImports = false;
	bool _scanForNormalImports = true;
	bool _junkByteAfterInst;
	ApiReader* _pApiReader = nullptr;

	~IATReferenceScan() {
		_iatReferences.clear();
		_iatDirectImports.clear();

		if (_pIATBackup) {
			free(_pIATBackup);
		}
	}

	void StartScan(DWORD_PTR imageBase, DWORD_PTR imageSize, DWORD_PTR iatAddress, DWORD iatSize);
	void PatchNewIAT(DWORD_PTR stdImageBase, DWORD_PTR newIATBaseAddress, PEParser& parser);
	void PatchDirectJumpTable(DWORD_PTR stdImageBase, DWORD directImportsJumpTableRVA, PEParser& parser,
		BYTE* pJmpTableMemory, DWORD newIATBase);
	void PatchDirectImportsMemory(bool junkByteAfterInst);
	int NumberOfFoundDirectImports();
	int NumberOfFoundUniqueDirectImports();
	int NumberOfDirectImportApisNotInIAT();
	int GetSizeInBytesOfJumpTableInSection();
	DWORD AddAdditionalApisToList();


private:
	DWORD_PTR _newIATAddressRVA;
	DWORD_PTR _iatAddressVA = 0;
	DWORD _iatSize = 0;
	DWORD_PTR _imageBase = 0;
	DWORD _imageSize = 0;
	DWORD_PTR* _pIATBackup = nullptr;

	std::vector<IATReference> _iatReferences;
	std::vector<IATReference> _iatDirectImports;

	void ScanMemoryPage(PVOID baseAddress, SIZE_T regionSize);
	void AnalyzeInstruction(_DInst* pInst);
	void FindNormalIATReference(_DInst* pInst);
	void GetIATEntryAddress(IATReference* pRef);
	void FindDirectIATReferenceCallJmp(_DInst* pInst);
	bool IsAddressValidImageMemory(DWORD_PTR address);
	void PatchReferenceInMemory(IATReference* pRef);
	void PatchDirectImportInMemory(IATReference* pRef);
	DWORD_PTR LookupIATForPointer(DWORD_PTR addr);
	void FindDirectIATReferenceMov(_DInst* pInst);
	void FindDirectIATReferencePush(_DInst* pInst);
	void CheckMemoryRangeAndAddToList(IATReference* pRef, _DInst* pInst);
	void FindDirectIATReferenceLea(_DInst* pInst);
	void PatchDirectImportInDump32(int patchPrefixBytes, int instSize, DWORD patchBytes, BYTE* pMemory,
		DWORD memorySize, bool generateReloc, DWORD patchOffset, DWORD sectionRVA);
	void PatchDirectImportInDump64(int patchPrefixBytes, int instSize, DWORD_PTR patchBytes, BYTE* pMemory,
		DWORD memorySize, bool generateReloc, DWORD patchOffset, DWORD sectionRVA);
	void PatchDirectJumpTableEntry(DWORD_PTR targetIATPointer, DWORD_PTR stdImageBase, DWORD directImportsJumpTableRVA,
		PEParser& parser, BYTE* pJmpTableMemory, DWORD newIATBase);
};