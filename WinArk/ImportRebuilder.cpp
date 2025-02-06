#include "stdafx.h"
#include "ImportRebuilder.h"
#include <Helpers.h>

bool ImportRebuilder::RebuildImportTable(const WCHAR* newFilePath, 
	std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap){
	bool ret = false;
	std::map<DWORD_PTR, ImportModuleThunk> copyModule;
	copyModule.insert(moduleThunkMap.begin(), moduleThunkMap.end());

	if (IsValid()) {

		GetPESections();

		SetDefaultFileAligment();

		ret = BuildNewImportTable(copyModule);
		if (ret) {
			AlignAllSectionHeaders();
			FixPEHeader();
			if (_newIATInSection) {
				PatchFileForNewIATLocation();
			}

			if (_buildDirectImportsJumpTable) {
				PatchFileForDirectImportJumpTable();
			}

			ret = SavePEFileToDisk(newFilePath);
		}
	}
	
	return ret;
}

void ImportRebuilder::EnableOFTSupport()
{
	_useOFT = true;
}

void ImportRebuilder::EnableNewIATInSection(DWORD_PTR iatAddress, DWORD iatSize)
{
	_newIATInSection = true;
	_iatAddress = iatAddress;
	_iatSize = iatSize;

	_pIATReferenceScan->_scanForDirectImports = false;
	_pIATReferenceScan->_scanForNormalImports = true;

	_pIATReferenceScan->StartScan(ProcessAccessHelper::_targetImageBase,ProcessAccessHelper::_targetSizeOfImage, 
		_iatAddress, _iatSize);
}

bool ImportRebuilder::BuildNewImportTable(std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap) {
	CreateNewImportSection(moduleThunkMap);

	_importSectionIndex = _PESections.size() - 1;
	if (_buildDirectImportsJumpTable) {
		_directImportsJumpTableRVA = _PESections[_importSectionIndex]._sectionHeader.VirtualAddress;
		_pJmpTableMemory = _PESections[_importSectionIndex]._pData;
	}
	if (_newIATInSection) {
		_newIATBaseAddressRVA = _PESections[_importSectionIndex]._sectionHeader.VirtualAddress;
		if (_buildDirectImportsJumpTable) {
			_newIATBaseAddressRVA += _pIATReferenceScan->GetSizeInBytesOfJumpTableInSection();
		}
		ChangeIATBaseAddress(moduleThunkMap);
	}

	DWORD size = FillImportSection(moduleThunkMap);
	if (size == 0) {
		return false;
	}

	DWORD_PTR iat = (*moduleThunkMap.begin()).second.m_FirstThunk;
	SetFlagToIATSection(iat);

	DWORD va = _PESections[_importSectionIndex]._sectionHeader.VirtualAddress;

	if (_useOFT) {
		va += _sizeOfOFTArray;
	}
	if (_newIATInSection) {
		va += _iatSize;
	}

	SetImportTable(va, _numberOfImportDescriptors * sizeof(IMAGE_IMPORT_DESCRIPTOR));
	

	return true;
}

DWORD ImportRebuilder::FillImportSection(std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap) {
	PIMAGE_IMPORT_BY_NAME pImportByName = nullptr;
	PIMAGE_THUNK_DATA pThunkData = nullptr;
	ImportModuleThunk* pModuleThunk = nullptr;
	ImportThunk* pThunk = nullptr;

	size_t len = 0;
	DWORD_PTR lastRVA = 0;
	BYTE* pSectionData = _PESections[_importSectionIndex]._pData;
	DWORD offset = 0;
	DWORD offsetOFTArray = 0;

	DWORD pointerSize = 4;
	if (IsPe64()) {
		pointerSize = 8;
	}
	/*
	New Scylla section contains:

	1. (optional) direct imports jump table
	2. (optional) new iat
	3. (optional) OFT
	4. Normal IAT entries

	*/
	if (_buildDirectImportsJumpTable) {
		offset += _pIATReferenceScan->GetSizeInBytesOfJumpTableInSection();
		offsetOFTArray = _pIATReferenceScan->GetSizeInBytesOfJumpTableInSection();
	}
	if (_newIATInSection) {
		offset += _iatSize;
		offsetOFTArray += _iatSize;
		memset(pSectionData, 0xFF, offset);
	}
	if (_useOFT) {
		offset += _sizeOfOFTArray;
	}

	_pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD_PTR)pSectionData + offset);

	offset += (DWORD)(_numberOfImportDescriptors * sizeof(IMAGE_IMPORT_DESCRIPTOR));

	for (auto& moduleThunk : moduleThunkMap) {
		ImportModuleThunk* pImportModuleThunk = &moduleThunk.second;

		len = AddImportDescriptor(pImportModuleThunk, offset, offsetOFTArray);

		offset += len;

		pImportByName = (PIMAGE_IMPORT_BY_NAME)((DWORD_PTR)pSectionData + offset);

		lastRVA = moduleThunk.second.m_FirstThunk - pointerSize;

		for (auto& thunk : pImportModuleThunk->m_ThunkMap) {
			pThunk = &thunk.second;
			if (_useOFT) {
				pThunkData = (PIMAGE_THUNK_DATA)((DWORD_PTR)pSectionData + offsetOFTArray);
				offsetOFTArray += pointerSize;
			}
			else {
				pThunkData = (PIMAGE_THUNK_DATA)GetMemoryPointerFromRVA(pThunk->m_RVA);
			}

			if (!pThunkData) {
				return 0;
			}

			if ((lastRVA + pointerSize) != pThunk->m_RVA) {
				AddSepecialImportDescriptor(pThunk->m_RVA, offsetOFTArray);
				if (_useOFT) {
					pThunkData = (PIMAGE_THUNK_DATA)((DWORD_PTR)pSectionData + offsetOFTArray);
					offsetOFTArray += pointerSize;
				}
			}
			lastRVA = pThunk->m_RVA;

			len = AddImportToImportTable(pThunk, pThunkData, pImportByName, offset);

			offset += len;
			pImportByName = (PIMAGE_IMPORT_BY_NAME)((DWORD_PTR)pImportByName + len);
		}

		offsetOFTArray += pointerSize;
		_pImportDescriptor++;
	}

	return offset;
}

size_t ImportRebuilder::AddImportDescriptor(ImportModuleThunk* pImportThunk, DWORD sectionOffset, DWORD sectionOffsetOFTArray) {
	std::string dllName;

	dllName = Helpers::WstringToString(pImportThunk->m_ModuleName);
	size_t len = dllName.length() + 1;

	memcpy(_PESections[_importSectionIndex]._pData + sectionOffset, dllName.c_str(), len);

	_pImportDescriptor->FirstThunk = pImportThunk->m_FirstThunk;
	_pImportDescriptor->Name = FileOffsetToRva((DWORD_PTR)_PESections[_importSectionIndex]._sectionHeader.PointerToRawData + sectionOffset);

	if (_useOFT) {
		DWORD_PTR orginalFirstThunkOffset = (DWORD_PTR)_PESections[_importSectionIndex]._sectionHeader.PointerToRawData + sectionOffsetOFTArray;
		_pImportDescriptor->OriginalFirstThunk = FileOffsetToRva(orginalFirstThunkOffset);
	}

	return len;
}

BYTE* ImportRebuilder::GetMemoryPointerFromRVA(DWORD_PTR rva) {
	int idx = RVAToSectionIndex(rva);

	if (idx == -1)
		return nullptr;

	DWORD pointerSize = 4;
	if (IsPe64()) {
		pointerSize = 8;
	}

	DWORD rvaPointer = rva - _PESections[idx]._sectionHeader.VirtualAddress;
	DWORD minSectionSize = rvaPointer + pointerSize * 2;

	if (_PESections[idx]._pData == nullptr || _PESections[idx]._dataSize == 0) {
		_PESections[idx]._dataSize = minSectionSize;
		_PESections[idx]._normalSize = minSectionSize;
		_PESections[idx]._pData = new BYTE[_PESections[idx]._dataSize];
	}
	else if (_PESections[idx]._dataSize < minSectionSize) {
		BYTE* pTemp = new BYTE[minSectionSize];
		memcpy(pTemp, _PESections[idx]._pData, _PESections[idx]._dataSize);
		delete[] _PESections[idx]._pData;
		_PESections[idx]._pData = pTemp;
		_PESections[idx]._dataSize = minSectionSize;
		_PESections[idx]._normalSize = minSectionSize;

		_PESections[idx]._sectionHeader.SizeOfRawData = _PESections[idx]._dataSize;
	}

	return _PESections[idx]._pData + rvaPointer;
}

void ImportRebuilder::SetFlagToIATSection(DWORD_PTR iatAddress) {
	for (size_t i = 0; i < _PESections.size(); i++) {
		if ((_PESections[i]._sectionHeader.VirtualAddress <= iatAddress) &&
			(_PESections[i]._sectionHeader.VirtualAddress + _PESections[i]._sectionHeader.Misc.VirtualSize > iatAddress)) {
			_PESections[i]._sectionHeader.Characteristics |= IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
		}
	}
}

bool ImportRebuilder::CreateNewImportSection(std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap) {
	char sectionName[IMAGE_SIZEOF_SHORT_NAME + 1] = { 0 };

	CalculateImportSize(moduleThunkMap);

	strcpy_s(sectionName, ".scy");

	if (_newIATInSection) {
		_sizeOfImportSection += _iatSize;
	}
	if (_buildDirectImportsJumpTable) {
		_sizeOfImportSection += _pIATReferenceScan->GetSizeInBytesOfJumpTableInSection();
	}

	return AddNewLastSection(sectionName, _sizeOfImportSection, nullptr);
}

size_t ImportRebuilder::AddImportToImportTable(ImportThunk* pImportThunk, PIMAGE_THUNK_DATA pThunkData, PIMAGE_IMPORT_BY_NAME pImportByName,
	DWORD sectionOffset) {
	size_t len = 0;
	if (pImportThunk->m_Name[0] == '\0') {
		pThunkData->u1.AddressOfData = (IMAGE_ORDINAL(pImportThunk->m_Ordinal) | IMAGE_ORDINAL_FLAG);
	}
	else {
		pImportByName->Hint = pImportThunk->m_Hint;

		len = strlen(pImportThunk->m_Name) + 1;

		memcpy(pImportByName->Name,pImportThunk->m_Name, len);

		DWORD_PTR offset = _PESections[_importSectionIndex]._sectionHeader.PointerToRawData + sectionOffset;
		pThunkData->u1.AddressOfData = FileOffsetToRva(offset);
		
		pThunkData++;
		pThunkData->u1.AddressOfData = 0;

		len += sizeof(WORD);
	}

	return len;
}

void ImportRebuilder::CalculateImportSize(std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap) {
	DWORD_PTR lastRVA = 0;

	_sizeOfImportSection = 0;
	_sizeOfApiAndModuleNames = 0;
	_sizeOfOFTArray = 0;

	_numberOfImportDescriptors = moduleThunkMap.size() + 1;
	DWORD pointerSize = 0;
	if (!IsPe64()) {
		pointerSize = 4;
	}
	else {
		pointerSize = 8;
	}
	for (auto& moduleThunk : moduleThunkMap) {
		ImportModuleThunk* pModuleThunk = &moduleThunk.second;

		lastRVA = pModuleThunk->m_FirstThunk - pointerSize;
		_sizeOfApiAndModuleNames += wcslen(pModuleThunk->m_ModuleName) + 1;

		for (auto& thunk : pModuleThunk->m_ThunkMap) {
			ImportThunk* pThunk = &thunk.second;

			if ((lastRVA + pointerSize) != pThunk->m_RVA) {
				_numberOfImportDescriptors++;
				_sizeOfOFTArray += pointerSize + pointerSize;
			}

			if (pThunk->m_Name[0] != '\0') {
				_sizeOfApiAndModuleNames += sizeof(WORD); // Hint from IMAGE_BY_NAME
				_sizeOfApiAndModuleNames += strlen(pThunk->m_Name) + 1;
			}

			_sizeOfOFTArray += pointerSize;

			lastRVA = pThunk->m_RVA;
		}

		_sizeOfOFTArray += pointerSize;
	}

	_sizeOfImportSection = _sizeOfOFTArray + _sizeOfApiAndModuleNames 
		+ _numberOfImportDescriptors * sizeof(IMAGE_IMPORT_DESCRIPTOR);
}

void ImportRebuilder::AddSepecialImportDescriptor(DWORD_PTR rvaFirstThunk, DWORD sectionOffsetOFTArray) {
	PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor = _pImportDescriptor;
	_pImportDescriptor++;

	_pImportDescriptor->FirstThunk = rvaFirstThunk;
	_pImportDescriptor->Name = pImportDescriptor->Name;

	if (_useOFT) {
		DWORD_PTR offset = _PESections[_importSectionIndex]._sectionHeader.PointerToRawData + sectionOffsetOFTArray;
		_pImportDescriptor->OriginalFirstThunk = FileOffsetToRva(offset);
	}
}

void ImportRebuilder::PatchFileForNewIATLocation() {
	_pIATReferenceScan->PatchNewIAT(GetImageBase(), _newIATBaseAddressRVA, (PEParser*)this);
}

void ImportRebuilder::ChangeIATBaseAddress(std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap) {
	DWORD_PTR oldIATRva = _iatAddress - ProcessAccessHelper::_targetImageBase;

	for (auto& moduleThunk : moduleThunkMap) {
		ImportModuleThunk* pModuleThunk = &moduleThunk.second;
		pModuleThunk->m_FirstThunk = pModuleThunk->m_FirstThunk - oldIATRva + _newIATBaseAddressRVA;
		for (auto& thunk : pModuleThunk->m_ThunkMap) {
			ImportThunk* pThunk = &thunk.second;
			pThunk->m_RVA = pThunk->m_RVA - oldIATRva + _newIATBaseAddressRVA;
		}
	}
}

void ImportRebuilder::PatchFileForDirectImportJumpTable() {
	if (_newIATInSection) {
		_pIATReferenceScan->PatchDirectJumpTable(GetImageBase(), _directImportsJumpTableRVA,
			this, _pJmpTableMemory, _newIATBaseAddressRVA);
	}
	else {
		_pIATReferenceScan->PatchDirectJumpTable(GetImageBase(), _directImportsJumpTableRVA,
			this, _pJmpTableMemory, 0);
	}
}