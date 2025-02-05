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

	return true;
}

DWORD ImportRebuilder::FillImportSection(std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap) {
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = nullptr;
	PIMAGE_IMPORT_BY_NAME pImportByName = nullptr;
	PIMAGE_THUNK_DATA pThunkData = nullptr;
	ImportModuleThunk* pModuleThunk = nullptr;
	ImportThunk* pThunk = nullptr;

	size_t len = 0;
	DWORD_PTR lastRVA = 0;
	auto sections = GetSections();
	BYTE* pSectionData = GetFileAddress(sections[_importSectionIndex].PointerToRawData);
	DWORD offset = 0;
	DWORD offsetOFTArray = 0;

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

	pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD_PTR)pSectionData + offset);

	offset += (DWORD)(_numberOfImportDescriptors * sizeof(IMAGE_IMPORT_DESCRIPTOR));

	for (auto& moduleThunk : moduleThunkMap) {
		ImportModuleThunk* pImportModuleThunk = &moduleThunk.second;

		len = AddImportDescriptor(pImportModuleThunk, offset, offsetOFTArray);

		offset += len;

		pImportByName = (PIMAGE_IMPORT_BY_NAME)((DWORD_PTR)pSectionData + offset);

		lastRVA = moduleThunk.second.m_FirstThunk - sizeof(DWORD_PTR);

		for (auto& thunk : pImportModuleThunk->m_ThunkMap) {
			pThunk = &thunk.second;
			if (_useOFT) {
				pThunkData = (PIMAGE_THUNK_DATA)((DWORD_PTR)pSectionData + offsetOFTArray);
				offsetOFTArray += sizeof(DWORD_PTR);
			}
			else {
				pThunkData = (PIMAGE_THUNK_DATA)GetMemoryPointerFromRVA(pThunk->m_RVA);
			}

			if (!pThunkData) {
				return 0;
			}

			if ((lastRVA + sizeof(DWORD_PTR)) != pThunk->m_RVA) {
				AddSepecialImportDescriptor(pThunk->m_RVA, offsetOFTArray);
				if (_useOFT) {
					pThunkData = (PIMAGE_THUNK_DATA)((DWORD_PTR)pSectionData + offsetOFTArray);
					offsetOFTArray += sizeof(DWORD_PTR);
				}
			}
			lastRVA = pThunk->m_RVA;

			len = AddImportToImportTable(pThunk, pThunkData, pImportByName, offset);

			offset += len;
			pImportByName = (PIMAGE_IMPORT_BY_NAME)((DWORD_PTR)pImportByName + len);
		}

		offsetOFTArray += sizeof(DWORD_PTR);
		pImportDesc++;
	}

	return offset;
}

size_t ImportRebuilder::AddImportDescriptor(ImportModuleThunk* pImportThunk, DWORD sectionOffset, DWORD sectionOffsetOFTArray) {
	std::string dllName;

	dllName = Helpers::WstringToString(pImportThunk->m_ModuleName);
	size_t len = dllName.length() + 1;

	auto sections = GetSections();
	BYTE* pSectionData = GetFileAddress(sections[_importSectionIndex].PointerToRawData);
	BYTE* pName = pSectionData + sectionOffset;
	memcpy(pName, dllName.c_str(), len);

	_pImportDescriptor->FirstThunk = pImportThunk->m_FirstThunk;
	_pImportDescriptor->Name = FileOffsetToRva((DWORD_PTR)pName);

	if (_useOFT) {
		DWORD_PTR orginalFirstThunkOffset = (DWORD_PTR)pSectionData + sectionOffsetOFTArray;
		_pImportDescriptor->OriginalFirstThunk = FileOffsetToRva(orginalFirstThunkOffset);
	}

	return len;
}

BYTE* ImportRebuilder::GetMemoryPointerFromRVA(DWORD_PTR rva) {
	int idx = RVAToSectionIndex(rva);

	if (idx == -1)
		return nullptr;

	auto sections = GetSections();
	DWORD rvaPointer = rva - sections[idx].VirtualAddress;
	
	DWORD minSectionSize = rvaPointer + sizeof(DWORD_PTR) * 2;

	if (sections[idx].SizeOfRawData < minSectionSize) {
		sections[idx].SizeOfRawData = minSectionSize;
	}

	BYTE* pSectionData = GetFileAddress(sections[idx].PointerToRawData);

	return pSectionData + rvaPointer;
}

void ImportRebuilder::SetFlagToIATSection(DWORD_PTR iatAddress) {
	auto sections = GetSections();
	for (size_t i = 0; i < GetSectionCount(); i++) {
		if ((sections[i].VirtualAddress <= iatAddress) && ((sections[i].VirtualAddress + sections[i].Misc.VirtualSize) > iatAddress))
		{
			//section must be read and writeable
			sections[i].Characteristics |= IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
		}
	}
}

bool ImportRebuilder::CreateNewImportSection(std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap) {
	char sectionName[IMAGE_SIZEOF_SHORT_NAME + 1] = { 0 };

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

		memcpy_s(pImportByName->Name, sizeof(pImportByName->Name), pImportThunk->m_Name, len);

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

	for (auto& moduleThunk : moduleThunkMap) {
		ImportModuleThunk* pModuleThunk = &moduleThunk.second;

		for (auto& thunk : pModuleThunk->m_ThunkMap) {
			ImportThunk* pThunk = &thunk.second;

			if ((lastRVA + sizeof(DWORD_PTR)) != pThunk->m_RVA) {
				_numberOfImportDescriptors++;
				_sizeOfOFTArray += sizeof(DWORD_PTR);
			}

			if (pThunk->m_Name[0] != '\0') {
				_sizeOfApiAndModuleNames += sizeof(WORD); // Hint from IMAGE_BY_NAME
				_sizeOfApiAndModuleNames += strlen(pThunk->m_Name) + 1;
			}

			_sizeOfOFTArray += sizeof(DWORD_PTR);

			lastRVA = pThunk->m_RVA;
		}

		_sizeOfOFTArray += sizeof(DWORD_PTR);
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