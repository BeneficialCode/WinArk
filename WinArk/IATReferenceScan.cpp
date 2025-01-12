#include "stdafx.h"
#include "IATReferenceScan.h"
#include <set>

int IATReferenceScan::NumberOfFoundDirectImports()
{
	return _iatDirectImports.size();
}

int IATReferenceScan::NumberOfFoundUniqueDirectImports() {
	std::set<DWORD_PTR> apiPointers;
	for (auto& ref : _iatDirectImports) {
		apiPointers.insert(ref._targetPointer);
	}

	return apiPointers.size();
}

int IATReferenceScan::NumberOfDirectImportApisNotInIAT() {
	std::set<DWORD_PTR> apiPointers;
	for (auto& ref : _iatDirectImports) {
		if (ref._targetPointer == 0) {
			apiPointers.insert(ref._targetAddressInIAT);
		}
	}
	return apiPointers.size();
}

int IATReferenceScan::GetSizeInBytesOfJumpTableInSection() {
	BYTE pattern[] = { 0xFF,0x25,0x00,0x00,0x00,0x00};
	return NumberOfFoundDirectImports() * sizeof(pattern);
}

void IATReferenceScan::StartScan(DWORD_PTR imageBase, DWORD_PTR imageSize, DWORD_PTR iatAddress, DWORD iatSize) {
	MEMORY_BASIC_INFORMATION mbi = { 0 };

	_iatAddressVA = iatAddress;
	_iatSize = iatSize;
	_imageBase = imageBase;
	_imageSize = imageSize;

	if (_scanForNormalImports) {
		_iatReferences.clear();
		_iatReferences.reserve(200);
	}
	if (_scanForDirectImports) {
		_iatDirectImports.clear();
		_iatDirectImports.reserve(50);
	}

	DWORD_PTR section = imageBase;

	do
	{
		if (!VirtualQueryEx(ProcessAccessHelper::_hProcess, (LPCVOID)section, &mbi, sizeof(MEMORY_BASIC_INFORMATION)))
			break;
		else {
			if (ProcessAccessHelper::IsPageExecutable(mbi.Protect)) {
				ScanMemoryPage(mbi.BaseAddress, mbi.RegionSize);
			}
		}

		section = (DWORD_PTR)((SIZE_T)section + mbi.RegionSize);

	} while (section<(imageBase+imageSize));
}

void IATReferenceScan::PatchNewIAT(DWORD_PTR stdImageBase, DWORD_PTR newIATBaseAddress, PEParser& parser) {
	_newIATAddressRVA = newIATBaseAddress;
	DWORD patchBytes = 0;

	for (auto& ref : _iatReferences) {
		DWORD_PTR newIATAddressPointer = ref._targetPointer - _iatAddressVA + _newIATAddressRVA + stdImageBase;

		BOOL isWow64 = FALSE;
		::IsWow64Process(ProcessAccessHelper::_hProcess, &isWow64);
		if (!isWow64) {
			patchBytes = (DWORD)(newIATAddressPointer - (ref._addressVA - _imageBase + stdImageBase) - 6);
		}
		else {
			patchBytes = newIATAddressPointer;
		}

		DWORD_PTR rva = ref._addressVA - _imageBase;
		DWORD_PTR patchOffset = parser.RVAToRelativeOffset(rva);
		int idx = parser.RVAToSectionIndex(rva);
		auto section = parser.GetSectionHeader(idx);
		auto start = section->VirtualAddress ? section->VirtualAddress : section->PointerToRawData;
		auto size = std::min(section->Misc.VirtualSize, section->SizeOfRawData);
		BYTE* pMemory = (BYTE*)parser.GetAddress(start);
		DWORD memorySize = size;

		if (memorySize < (DWORD)(patchOffset + 6)) {
			return;
		}
		else {
			pMemory += patchOffset + 2;
			*(DWORD*)pMemory = patchBytes;
		}
	}
}

void IATReferenceScan::PatchDirectImportsMemory(bool junkByteAfterInst) {
	_junkByteAfterInst = junkByteAfterInst;
	for (auto& ref : _iatDirectImports) {
		PatchDirectImportInMemory(&ref);
	}
}

void IATReferenceScan::PatchDirectJumpTable(DWORD_PTR stdImageBase, DWORD directImportsJumpTableRVA, PEParser& parser,
	BYTE* pJmpTableMemory, DWORD newIATBase) {
	std::set<DWORD_PTR> apiPointers;
	for (auto& ref : _iatDirectImports) {
		apiPointers.insert(ref._targetPointer);
	}

	DWORD patchBytes;

	for (auto pointer : apiPointers) {
		if (newIATBase) {
			pointer = pointer - _iatAddressVA + newIATBase + _imageBase;
		}

		// create jump table in section
		DWORD_PTR newIATAddressPointer = pointer - _imageBase + stdImageBase;

		BOOL isWow64 = FALSE;
		::IsWow64Process(ProcessAccessHelper::_hProcess, &isWow64);
		if (!isWow64) {
			patchBytes = (DWORD)(newIATAddressPointer - (directImportsJumpTableRVA + stdImageBase) - 6);
		}
		else {
			patchBytes = newIATAddressPointer;
			DWORD relocOffset = (directImportsJumpTableRVA + 2);
		}
		pJmpTableMemory[0] = 0xFF;
		pJmpTableMemory[1] = 0x25;
		*(DWORD*)&pJmpTableMemory[2] = patchBytes;

		PatchDirectJumpTableEntry(pointer, stdImageBase, directImportsJumpTableRVA, parser, pJmpTableMemory, newIATBase);

		pJmpTableMemory += 6;
		directImportsJumpTableRVA += 6;
	}
}

DWORD IATReferenceScan::AddAdditionalApisToList() {
	std::set<DWORD_PTR> apiPointers;

	for (auto& ref : _iatDirectImports) {
		if (ref._targetPointer == 0)
			apiPointers.insert(ref._targetAddressInIAT);
	}

	DWORD_PTR targetPointer = _iatAddressVA + _iatSize;
	DWORD newIATSize = _iatSize;

	bool isSuspect = false;
	for (auto& pointer : apiPointers) {
		for (auto& ref : _iatDirectImports) {
			if (ref._targetPointer == 0 && ref._targetAddressInIAT == pointer) {
				ref._targetPointer = targetPointer;
				ApiInfo* pApi = _pApiReader->GetApiByVirtualAddress(ref._targetAddressInIAT, &isSuspect);
				_pApiReader->AddFoundApiToModuleList(targetPointer, pApi, true, isSuspect);
			}
		}

		targetPointer += sizeof(DWORD_PTR);
		newIATSize += sizeof(DWORD_PTR);
	}

	return newIATSize;
}

void IATReferenceScan::ScanMemoryPage(PVOID baseAddress, SIZE_T regionSize)
{
	BYTE* pData = (BYTE*)calloc(regionSize, 1);
	BYTE* pPos = pData;
	int size = regionSize;
	DWORD_PTR pOffset = (DWORD_PTR)baseAddress;
	_DecodeResult result;
	unsigned int instCount = 0, next = 0;

	if (!pData)
		return;

	if (ProcessAccessHelper::ReadMemoryFromProcess((DWORD_PTR)baseAddress, regionSize, pData)) {
		while (true)
		{
			ZeroMemory(&ProcessAccessHelper::_codeInfo, sizeof(_CodeInfo));
			ProcessAccessHelper::_codeInfo.code = pPos;
			ProcessAccessHelper::_codeInfo.codeLen = size;
			BOOL isWow64 = FALSE;
			::IsWow64Process(ProcessAccessHelper::_hProcess, &isWow64);
			if (isWow64) {
				ProcessAccessHelper::_codeInfo.dt = Decode32Bits;
			}
			else {
				ProcessAccessHelper::_codeInfo.dt = Decode64Bits;
			}

			instCount = 0;

			result = distorm_decompose(&ProcessAccessHelper::_codeInfo, ProcessAccessHelper::_insts,
				_countof(ProcessAccessHelper::_insts), &instCount);
			if (result == DECRES_INPUTERR) {
				break;
			}

			for (unsigned int i = 0; i < instCount; i++) {
				if (ProcessAccessHelper::_insts[i].flags != FLAG_NOT_DECODABLE) {
					AnalyzeInstruction(&ProcessAccessHelper::_insts[i]);
				}
			}

			if (result == DECRES_SUCCESS)
				break;
			else if (instCount == 0)
				break;

			next = ProcessAccessHelper::_insts[instCount - 1].addr - ProcessAccessHelper::_insts[0].addr;

			if (ProcessAccessHelper::_insts[instCount - 1].flags != FLAG_NOT_DECODABLE)
				next += ProcessAccessHelper::_insts[instCount - 1].size;

			pPos += next;
			pOffset += next;
			size -= next;
		}
	}

	free(pData);
}

void IATReferenceScan::AnalyzeInstruction(_DInst* pInst) {
	if (_scanForNormalImports) {
		FindNormalIATReference(pInst);
	}

	if (_scanForDirectImports) {
		FindDirectIATReferenceMov(pInst);
		BOOL isWow64 = FALSE;
		::IsWow64Process(ProcessAccessHelper::_hProcess, &isWow64);
		if (!isWow64) {
			FindDirectIATReferenceCallJmp(pInst);
			FindDirectIATReferenceLea(pInst);
			FindDirectIATReferencePush(pInst);
		}
	}
}

void IATReferenceScan::FindNormalIATReference(_DInst* pInst) {
	IATReference ref;

	if (META_GET_FC(pInst->meta) == FC_CALL || META_GET_FC(pInst->meta) == FC_UNC_BRANCH) {
		if (pInst->size >= 5) {
			if (META_GET_FC(pInst->meta) == FC_CALL) {
				ref._type = IAT_REFERENCE_PTR_CALL;
			}
			else {
				ref._type = IAT_REFERENCE_PTR_JMP;
			}
			ref._addressVA = (DWORD_PTR)pInst->addr;
			ref._instSize = pInst->size;

			BOOL isWow64 = FALSE;
			::IsWow64Process(ProcessAccessHelper::_hProcess, &isWow64);
			if (!isWow64) {
				if (pInst->flags & FLAG_RIP_RELATIVE) {
					auto target = INSTRUCTION_GET_RIP_TARGET(pInst);
					if (target >= _iatAddressVA && target < (_iatAddressVA + _iatSize)) {
						ref._targetPointer = target;
						GetIATEntryAddress(&ref);
						_iatReferences.push_back(ref);
					}
				}
			}
			else {
				if (pInst->ops[0].type == O_DISP) {
					// jmp dword ptr || call dword ptr
					if (pInst->disp >= _iatAddressVA && pInst->disp < (_iatAddressVA + _iatSize)) {
						ref._targetPointer = (DWORD_PTR)pInst->disp;
						GetIATEntryAddress(&ref);
						_iatReferences.push_back(ref);
					}
				}
			}
		}
	}
}

void IATReferenceScan::GetIATEntryAddress(IATReference* pRef) {
	if (!ProcessAccessHelper::ReadMemoryFromProcess(pRef->_targetPointer, sizeof(DWORD_PTR), &pRef->_targetAddressInIAT)) {
		pRef->_targetAddressInIAT = 0;
	}
}

void IATReferenceScan::FindDirectIATReferenceCallJmp(_DInst* pInst) {
	IATReference ref;
	if (META_GET_FC(pInst->meta) == FC_CALL || META_GET_FC(pInst->meta) == FC_UNC_BRANCH) {
		if ((pInst->size >= 5) && (pInst->ops[0].type == O_PC)) {
			// call/jmp 0x00000
			if (META_GET_FC(pInst->meta) == FC_CALL) {
				ref._type = IAT_REFERENCE_DIRECT_CALL;
			}
			else {
				ref._type = IAT_REFERENCE_DIRECT_JMP;
			}

			ref._targetAddressInIAT = INSTRUCTION_GET_TARGET(pInst);

			CheckMemoryRangeAndAddToList(&ref, pInst);
		}
	}
}

bool IATReferenceScan::IsAddressValidImageMemory(DWORD_PTR address) {
	MEMORY_BASIC_INFORMATION mbi = { 0 };
	if (!VirtualQueryEx(ProcessAccessHelper::_hProcess, (LPCVOID)address, &mbi, sizeof(MEMORY_BASIC_INFORMATION))) {
		return false;
	}

	return mbi.Type == MEM_IMAGE && ProcessAccessHelper::IsPageExecutable(mbi.Protect);
}

void IATReferenceScan::PatchReferenceInMemory(IATReference* pRef) {
	DWORD_PTR newIATAddressPointer = pRef->_targetPointer - _iatAddressVA + _newIATAddressRVA;
	DWORD patchBytes = 0;

	BOOL isWow64 = FALSE;
	::IsWow64Process(ProcessAccessHelper::_hProcess, &isWow64);
	if (!isWow64) {
		patchBytes = newIATAddressPointer - pRef->_addressVA - 6;
	}
	else {
		patchBytes = newIATAddressPointer;
	}
	ProcessAccessHelper::WriteMemoryToProcess(pRef->_addressVA + 2, sizeof(DWORD), &patchBytes);
}

void IATReferenceScan::PatchDirectImportInMemory(IATReference* pRef) {
	DWORD patchBytes = 0;
	BYTE patchPreBytes[2];

	if (pRef->_targetPointer) {
		patchPreBytes[0] = 0xFF;
		if (pRef->_type == IAT_REFERENCE_DIRECT_CALL) // FF15
		{
			patchPreBytes[1] = 0x15;
		}
		else if (pRef->_type == IAT_REFERENCE_DIRECT_JMP) {
			patchPreBytes[1] = 0x25;
		}
		else {
			return;
		}

		if (!_junkByteAfterInst) {
			pRef->_addressVA -= 1;
		}

		ProcessAccessHelper::WriteMemoryToProcess(pRef->_addressVA, 2, patchPreBytes);

		BOOL isWow64 = FALSE;
		IsWow64Process(ProcessAccessHelper::_hProcess, &isWow64);
		if (!isWow64) {
			patchBytes = (DWORD)(pRef->_targetPointer - pRef->_addressVA - 6);
		}
		else {
			patchBytes = pRef->_targetPointer;
		}
		ProcessAccessHelper::WriteMemoryToProcess(pRef->_addressVA + 2, sizeof(DWORD), &patchBytes);
	}
}

DWORD_PTR IATReferenceScan::LookupIATForPointer(DWORD_PTR addr) {
	if (!_pIATBackup) {
		_pIATBackup = (DWORD_PTR*)calloc(_iatSize + sizeof(DWORD_PTR), 1);
		if (!_pIATBackup)
			return 0;
		if (!ProcessAccessHelper::ReadMemoryFromProcess(_iatAddressVA, _iatSize, _pIATBackup)) {
			free(_pIATBackup);
			_pIATBackup = nullptr;
			return 0;
		}
	}

	for (int i = 0; i < _iatSize / sizeof(DWORD_PTR); i++) {
		if (_pIATBackup[i] == addr) {
			return (DWORD_PTR)&_pIATBackup[i] - (DWORD_PTR)_pIATBackup + _iatAddressVA;
		}
	}

	return 0;
}

void IATReferenceScan::FindDirectIATReferenceMov(_DInst* pInst) {
	IATReference ref;
	ref._type = IAT_REFERENCE_DIRECT_MOV;

	if (pInst->opcode == I_MOV) {
		BOOL isWow64 = FALSE;
		IsWow64Process(ProcessAccessHelper::_hProcess, &isWow64);
		if (!isWow64) {
			if (pInst->size >= 7) {
				if (pInst->ops[0].type == O_REG && pInst->ops[1].type == O_IMM) {
					ref._targetAddressInIAT = (DWORD_PTR)pInst->imm.qword;
					CheckMemoryRangeAndAddToList(&ref, pInst);
				}
			}
		}
		else {
			if (pInst->size >= 5) {
				if (pInst->ops[0].type == O_REG && pInst->ops[1].type == O_IMM) {
					ref._targetAddressInIAT = (DWORD_PTR)pInst->imm.qword;
					CheckMemoryRangeAndAddToList(&ref, pInst);
				}
			}
		}
	}
}

void IATReferenceScan::FindDirectIATReferencePush(_DInst* pInst) {
	IATReference ref;
	ref._type == IAT_REFERENCE_DIRECT_PUSH;

	if(pInst->size >= 5 && pInst->opcode == I_PUSH) {
		ref._targetAddressInIAT = pInst->imm.qword;
		CheckMemoryRangeAndAddToList(&ref, pInst);
	}
}

void IATReferenceScan::CheckMemoryRangeAndAddToList(IATReference* pRef, _DInst* pInst) {
	if (pRef->_targetAddressInIAT > 0x000FFFFF && pRef->_targetAddressInIAT != (DWORD_PTR)-1) {
		if ((pRef->_targetAddressInIAT < _imageBase) || (pRef->_targetAddressInIAT > _imageBase + _imageSize)) {
			bool isSuspect = FALSE;
			if (_pApiReader->GetApiByVirtualAddress(pRef->_targetAddressInIAT, &isSuspect) != 0) {
				pRef->_addressVA = pInst->addr;
				pRef->_instSize = pInst->size;
				pRef->_targetPointer = LookupIATForPointer(pRef->_targetAddressInIAT);

				_iatDirectImports.push_back(*pRef);
			}
		}
	}
}

void IATReferenceScan::FindDirectIATReferenceLea(_DInst* pInst) {
	IATReference ref;
	ref._type = IAT_REFERENCE_DIRECT_LEA;

	if (pInst->size >= 5 && pInst->opcode == I_LEA) {
		if (pInst->ops[0].type == O_REG && pInst->ops[1].type == O_DISP) {
			// lea edx,[0xb58bb8]
			ref._targetAddressInIAT = pInst->disp;
			CheckMemoryRangeAndAddToList(&ref, pInst);
		}
	}
}

void IATReferenceScan::PatchDirectImportInDump32(int patchPrefixBytes, int instSize, DWORD patchBytes, BYTE* pMemory,
	DWORD memorySize, bool generateReloc, DWORD patchOffset, DWORD sectionRVA) {
	if (memorySize < (patchOffset + instSize)) {
		return;
	}
	pMemory += patchOffset + patchPrefixBytes;
	if (generateReloc) {
		DWORD relocOffset = sectionRVA + patchOffset + patchPrefixBytes;
	}
	*(DWORD*)pMemory = patchBytes;
}

void IATReferenceScan::PatchDirectImportInDump64(int patchPrefixBytes, int instSize, DWORD_PTR patchBytes, BYTE* pMemory,
	DWORD memorySize, bool generateReloc, DWORD patchOffset, DWORD sectionRVA) {
	if (memorySize < (patchOffset + instSize)) {
		return;
	}
	pMemory += patchOffset + patchPrefixBytes;
	if (generateReloc) {
		DWORD relocOffset = sectionRVA + patchOffset + patchPrefixBytes;
	}
	*(DWORD_PTR*)pMemory = patchBytes;
}

void IATReferenceScan::PatchDirectJumpTableEntry(DWORD_PTR targetIATPointer, DWORD_PTR stdImageBase, DWORD directImportsJumpTableRVA,
	PEParser& parser, BYTE* pJmpTableMemory, DWORD newIATBase) {
	DWORD patchBytes = 0;
	for (auto& ref : _iatDirectImports) {
		if (ref._targetPointer == targetIATPointer) {
			DWORD patchOffset = 0;
			int index = 0;
			BYTE* pMemory = nullptr;
			DWORD memorySize = 0;
			DWORD sectionRVA = 0;

			DWORD_PTR rva = ref._addressVA - _imageBase;
			patchOffset = parser.RVAToRelativeOffset(rva);
			int idx = parser.RVAToSectionIndex(rva);
			auto section = parser.GetSectionHeader(idx);
			auto start = section->VirtualAddress ? section->VirtualAddress : section->PointerToRawData;
			auto size = std::min(section->Misc.VirtualSize, section->SizeOfRawData);
			pMemory = (BYTE*)parser.GetAddress(start);
			memorySize = size;
			sectionRVA = start;

			BOOL isWow64 = FALSE;
			::IsWow64Process(ProcessAccessHelper::_hProcess, &isWow64);
			if (ref._type == IAT_REFERENCE_DIRECT_CALL || ref._type == IAT_REFERENCE_DIRECT_JMP) {
				if (isWow64) {
					if (ref._instSize == 5) {
						patchBytes = directImportsJumpTableRVA - (ref._addressVA - _imageBase) - 5;
						PatchDirectImportInDump32(1, 5, patchBytes, pMemory, memorySize, false, patchOffset, sectionRVA);
					}
				}
			}
			else if (ref._type == IAT_REFERENCE_DIRECT_PUSH || ref._type == IAT_REFERENCE_DIRECT_MOV) {
				if (isWow64) {
					patchBytes = directImportsJumpTableRVA + stdImageBase;
					PatchDirectImportInDump32(1, 5, patchBytes, pMemory, memorySize, true, patchOffset, sectionRVA);
				}
				else {
					DWORD_PTR patchBytes64 = directImportsJumpTableRVA + stdImageBase;
					PatchDirectImportInDump64(2, 10, patchBytes64, pMemory, memorySize, true, patchOffset, sectionRVA);
				}
			}
			else if (ref._type == IAT_REFERENCE_DIRECT_LEA) {
				if (isWow64) {
					patchBytes = directImportsJumpTableRVA + stdImageBase;
					PatchDirectImportInDump32(2, 6, patchBytes, pMemory, memorySize, true, patchOffset, sectionRVA);
				}
			}
		}
	}
}