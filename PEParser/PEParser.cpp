#include "pch.h"
#include <string>
#include <vector>
#include <atlstr.h>
#include "PEParser.h"
#include "CLRMetadataParser.h"
#include <unordered_map>
#include <algorithm>
#pragma comment(lib,"imagehlp")


PEParser::PEParser(const wchar_t* path,bool isScylla) :_path(path) {
	if (isScylla) {
		_hFile = ::CreateFile(path, GENERIC_READ|GENERIC_WRITE,
			FILE_SHARE_READ| FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
		if (_hFile == INVALID_HANDLE_VALUE)
			return;
		::GetFileSizeEx(_hFile, &_fileSize);
		_hMemMap = ::CreateFileMapping(_hFile, nullptr, PAGE_READWRITE, 0, 0, nullptr);
		if (!_hMemMap)
			return;

		_address = (PBYTE)::MapViewOfFile(_hMemMap, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
		if (!_address)
			return;
	}
	else {
		_hFile = ::CreateFile(path, GENERIC_READ,
			FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
		if (_hFile == INVALID_HANDLE_VALUE)
			return;
		::GetFileSizeEx(_hFile, &_fileSize);
		_hMemMap = ::CreateFileMapping(_hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
		if (!_hMemMap)
			return;

		_address = (PBYTE)::MapViewOfFile(_hMemMap, FILE_MAP_READ, 0, 0, 0);
		if (!_address)
			return;
	}

	CheckValidity();
	if (IsValid()) {
		GetSectionHeaders();
	}
}

PEParser::PEParser(void* base) {
	_address = reinterpret_cast<PUCHAR>(base);
	_moduleBase = (DWORD_PTR)base;
	_isFileMap = false;
	CheckValidity();
	if (IsValid()) {
		GetSectionHeaders();
	}
}

void PEParser::GetSectionHeaders() {
	_PESections.clear();
	int count = GetSectionCount();
	_PESections.reserve(count);

	PEFileSection peFileSection;

	for (WORD i = 0; i < count; i++) {
		memcpy_s(&peFileSection._sectionHeader, sizeof(IMAGE_SECTION_HEADER), &_sections[i], sizeof(IMAGE_SECTION_HEADER));
		_PESections.push_back(peFileSection);
	}
}

HANDLE PEParser::GetFileHandle() {
	return _hFile;
}

PEParser::~PEParser() {
	if (_resModule)
		::FreeLibrary(_resModule);
	if (_hMemMap) {
		::UnmapViewOfFile(_address);
		::CloseHandle(_hMemMap);
	}
	if (_hFile != INVALID_HANDLE_VALUE)
		::CloseHandle(_hFile);
}

bool PEParser::IsValid() const {
	return _valid;
}

bool PEParser::IsPe64() const {
	return _opt64 ? _opt64->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC : IsObjectPe64();
}

bool PEParser::IsExecutable() const {
	if (!IsValid())
		return false;

	return (_fileHeader->Characteristics & IMAGE_FILE_DLL) == 0;
}

bool PEParser::IsManaged() const {
	return _opt64 ? GetDataDirectory(IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR)->Size != 0 : false;
}

bool PEParser::HasExports() const {
	return _opt64 ? GetDataDirectory(IMAGE_DIRECTORY_ENTRY_EXPORT)->VirtualAddress != 0 : false;
}

bool PEParser::HasImports() const {
	return _opt64 ? GetDataDirectory(IMAGE_DIRECTORY_ENTRY_IMPORT)->VirtualAddress != 0 : false;
}

int PEParser::GetSectionCount() const {
	if (!IsValid())
		return -1;

	return _fileHeader->NumberOfSections;
}

void PEParser::SetSectionCount(WORD count) {
	_fileHeader->NumberOfSections = count;
}

const IMAGE_SECTION_HEADER* PEParser::GetSectionHeader(ULONG section) const {
	if (!IsValid() || section >= _fileHeader->NumberOfSections)
		return nullptr;

	return _sections + section;
}

const IMAGE_DATA_DIRECTORY* PEParser::GetDataDirectory(int index) const {
	if (_opt64 == nullptr)	// object file
		return nullptr;

	if (!IsValid() || index < 0 || index > 15)
		return nullptr;

	return IsPe64() ? &_opt64->DataDirectory[index] : &_opt32->DataDirectory[index];
}

bool PEParser::IsSystemFile() const {
	return _ntHeader->FileHeader.Characteristics & IMAGE_FILE_SYSTEM;
}

const IMAGE_DOS_HEADER& PEParser::GetDosHeader() const {
	return *_dosHeader;
}

BYTE* PEParser::GetDosStub() {
	BYTE* pDosStub = nullptr;

	if (_dosHeader->e_lfanew > sizeof(IMAGE_DOS_HEADER)) {
		pDosStub = (BYTE*)((DWORD_PTR)_dosHeader + sizeof(IMAGE_DOS_HEADER));
	}
	else if (_dosHeader->e_lfanew < sizeof(IMAGE_DOS_HEADER)) {
		_dosHeader->e_lfanew = sizeof(IMAGE_DOS_HEADER);
	}

	return pDosStub;
}

IMAGE_NT_HEADERS64* PEParser::GetNtHeader() {
	return _ntHeader;
}

void* PEParser::GetBaseAddress() const {
	return _address;
}

std::string PEParser::GetSectionName(ULONG section) const {
	auto header = GetSectionHeader(section);
	if (header == nullptr)
		return "";

	if (header->Name[IMAGE_SIZEOF_SHORT_NAME - 1] == '\0')
		return std::string((PCSTR)header->Name);
	return std::string((PCSTR)header->Name, IMAGE_SIZEOF_SHORT_NAME);
}

std::vector<ExportedSymbol> PEParser::GetExports() const {
	std::vector<ExportedSymbol> exports;
	if (!HasExports())
		return exports;
	auto dir = GetDataDirectory(IMAGE_DIRECTORY_ENTRY_EXPORT);
	if (dir == nullptr || dir->Size == 0)
		return exports;

	if (_isFileMap) {
		auto data = static_cast<IMAGE_EXPORT_DIRECTORY*>(GetAddress(dir->VirtualAddress));
		auto count = data->NumberOfFunctions;
		exports.reserve(count);

		auto names = (PBYTE)(data->AddressOfNames != 0 ? GetAddress(data->AddressOfNames) : nullptr);
		auto ordinals = (PBYTE)(data->AddressOfNameOrdinals != 0 ? GetAddress(data->AddressOfNameOrdinals) : nullptr);
		auto functions = (PDWORD)GetAddress(data->AddressOfFunctions);
		char undecorated[1 << 10];
		auto ordinalBase = data->Base;

		std::unordered_map<uint32_t, std::string> functionNamesMap;
		std::unordered_map<uint32_t, uint32_t> hintsMap;
		for (uint32_t idx = 0; idx < data->NumberOfNames; idx++) {
			uint16_t ordinal;
			ordinal = *(USHORT*)(ordinals + idx * 2) + (USHORT)ordinalBase;
			uint32_t name;
			auto offset = *(ULONG*)(names + idx * 4);
			PCSTR pName = (PCSTR)GetAddress(offset);
			functionNamesMap[ordinal] = pName;
			hintsMap[ordinal] = idx;
		}

		for (DWORD i = 0; i < data->NumberOfFunctions; i++) {
			ExportedSymbol symbol;
			int ordinal = i + (USHORT)ordinalBase;
			symbol.Ordinal = ordinal;

			std::unordered_map<uint32_t, uint32_t>::iterator iter = hintsMap.find(ordinal);
			if (iter != hintsMap.end()) {
				symbol.Hint = iter->second;
			}

			bool hasName = false;
			auto pos = functionNamesMap.find(ordinal);
			if (pos != functionNamesMap.end()) {
				hasName = true;
			}
			if (names && hasName) {
				symbol.Name = pos->second;
				if (::UnDecorateSymbolName(symbol.Name.c_str(), undecorated, sizeof(undecorated), 0))
					symbol.UndecoratedName = undecorated;
			}
			DWORD address = *(functions + symbol.Ordinal - ordinalBase);
			symbol.Address = address;
			symbol.IsForward = false;
			symbol.HasName = false;
			if (hasName) {
				symbol.HasName = true;
				if (address > dir->VirtualAddress && address < dir->VirtualAddress + dir->Size) {
					symbol.ForwardName = (PCSTR)GetAddress(address);
					symbol.IsForward = true;
				}
			}
			exports.push_back(std::move(symbol));
		}
	}
	else {
		auto data = (IMAGE_EXPORT_DIRECTORY*)(_address + dir->VirtualAddress);
		auto count = data->NumberOfFunctions;
		exports.reserve(count);

		auto names = (PBYTE)(data->AddressOfNames != 0 ? _address + data->AddressOfNames: nullptr);
		auto ordinals = (PBYTE)(data->AddressOfNameOrdinals != 0 ? _address + data->AddressOfNameOrdinals : nullptr);
		auto functions = (PDWORD)(_address + data->AddressOfFunctions);
		char undecorated[1 << 10];
		auto ordinalBase = data->Base;

		std::unordered_map<uint32_t, std::string> functionNamesMap;
		std::unordered_map<uint32_t, uint32_t> hintsMap;
		for (uint32_t idx = 0; idx < data->NumberOfNames; idx++) {
			uint16_t ordinal;
			ordinal = *(USHORT*)(ordinals + idx * 2) + (USHORT)ordinalBase;
			uint32_t name;
			auto offset = *(ULONG*)(names + idx * 4);
			PCSTR pName = (PCSTR)_address + offset;
			functionNamesMap[ordinal] = pName;
			hintsMap[ordinal] = idx;
		}

		for (DWORD i = 0; i < data->NumberOfFunctions; i++) {
			ExportedSymbol symbol;
			int ordinal = i + (USHORT)ordinalBase;
			symbol.Ordinal = ordinal;

			std::unordered_map<uint32_t, uint32_t>::iterator iter = hintsMap.find(ordinal);
			if (iter != hintsMap.end()) {
				symbol.Hint = iter->second;
			}

			bool hasName = false;
			auto pos = functionNamesMap.find(ordinal);
			if (pos != functionNamesMap.end()) {
				hasName = true;
			}
			if (names && hasName) {
				symbol.Name = pos->second;
				if (::UnDecorateSymbolName(symbol.Name.c_str(), undecorated, sizeof(undecorated), 0))
					symbol.UndecoratedName = undecorated;
			}
			DWORD address = *(functions + symbol.Ordinal - ordinalBase);
			symbol.Address = address;
			symbol.IsForward = false;
			symbol.HasName = false;
			if (hasName) {
				symbol.HasName = true;
				if (address > dir->VirtualAddress && address < dir->VirtualAddress + dir->Size) {
					symbol.ForwardName = (PCSTR)_address + address;
					symbol.IsForward = true;
				}
			}
			exports.push_back(std::move(symbol));
		}
	}
	

	return exports;
}

std::vector<ImportedLibrary> PEParser::GetImports() const {
	std::vector<ImportedLibrary> libs;

	if (!IsValid())
		return libs;

	auto dir = GetDataDirectory(IMAGE_DIRECTORY_ENTRY_IMPORT);
	if (dir->Size == 0)
		return libs;

	auto imports = static_cast<IMAGE_IMPORT_DESCRIPTOR*>(GetAddress(dir->VirtualAddress));
	auto inc = IsPe64() ? 8 : 4;
	char undecorated[1 << 10];

	for (;;) {
		auto offset = imports->OriginalFirstThunk == 0 ? imports->FirstThunk : imports->OriginalFirstThunk;
		if (offset == 0)
			break;
		auto iat = static_cast<PBYTE>(GetAddress(offset));
		auto libName = static_cast<PBYTE>(GetAddress(imports->Name));

		ImportedLibrary lib;
		lib.Name = (PCSTR)libName;
		lib.IAT = imports->FirstThunk;

		for (;;) {
			int ordinal = -1;
			int nameRva = 0;
			if (inc == 8) {
				auto value = *(ULONGLONG*)iat;
				if (value == 0)
					break;
				auto isOrdinal = (value & (1ULL << 63)) > 0;
				if (isOrdinal)
					ordinal = (unsigned short)(value & 0xffff);
				else
					nameRva = (int)(value & ((1LL << 31) - 1));
			}
			else {
				auto value = *(ULONG*)iat;
				auto isOrdinal = (value & (1UL << 31)) > 0;
				if (isOrdinal)
					ordinal = (unsigned short)(value & 0xffff);
				else
					nameRva = (int)(value & ((1LL << 31) - 1));
			}
			if (nameRva == 0)
				break;
			auto p = static_cast<IMAGE_IMPORT_BY_NAME*>(GetAddress(nameRva));
			ImportedSymbol symbol;
			symbol.Hint = p->Hint;
			symbol.Name = p->Name;
			if (::UnDecorateSymbolName(symbol.Name.c_str(), undecorated, sizeof(undecorated), 0))
				symbol.UndecoratedName = undecorated;
			lib.Symbols.push_back(std::move(symbol));

			iat += inc;
		}
		libs.push_back(std::move(lib));
		imports++;
	}
	return libs;
}

const IMAGE_FILE_HEADER& PEParser::GetFileHeader() const {
	return *_fileHeader;
}

void* PEParser::GetAddress(unsigned rva) const {
	if (!IsValid())
		return nullptr;

	return ::ImageRvaToVa(::ImageNtHeader(_address), _address, rva, nullptr);
}

BYTE* PEParser::GetFileAddress(DWORD offset) {
	return _address + offset;
}

SubsystemType PEParser::GetSubsystemType() const {
	if (IsPe64())
		return static_cast<SubsystemType>(GetOptionalHeader64().Subsystem);
	else
		return static_cast<SubsystemType>(GetOptionalHeader32().Subsystem);
}

void PEParser::CheckValidity() {
	if (_address == nullptr) {
		_valid = false;
		return;
	}
	if (::strncmp((PCSTR)_address, "!<arch>\n", 8) == 0) {
		// LIB import library
		_importLib = true;
		return;
	}

	_dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(_address);
	if (_dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		_valid = false;
		return;
	}
	else {
		auto ntHeader = (PIMAGE_NT_HEADERS64)(_address + _dosHeader->e_lfanew);
		_ntHeader = ntHeader;
		_fileHeader = &ntHeader->FileHeader;
		_opt64 = &ntHeader->OptionalHeader;
		_opt32 = (PIMAGE_OPTIONAL_HEADER32)_opt64;
		_sections = (PIMAGE_SECTION_HEADER)((BYTE*)_opt64 + _fileHeader->SizeOfOptionalHeader);
	}
	_valid = true;
}

unsigned PEParser::RvaToFileOffset(unsigned rva) const {
	auto sections = _sections;
	for (int i = 0; i < GetSectionCount(); ++i) {
		if (rva >= sections[i].VirtualAddress && rva < sections[i].VirtualAddress + _sections[i].Misc.VirtualSize)
			return sections[i].PointerToRawData + rva - sections[i].VirtualAddress;
	}

	return rva;
}

DWORD_PTR PEParser::FileOffsetToRva(DWORD_PTR offset) {
	auto sections = _sections;
	for (int i = 0; i < GetSectionCount(); ++i) {
		if ((sections[i].PointerToRawData <= offset) && 
			((sections[i].PointerToRawData + sections[i].SizeOfRawData) > offset))
		{
			return ((offset - sections[i].PointerToRawData) + sections[i].VirtualAddress);
		}
	}

	return 0;
}

DWORD_PTR PEParser::RVAToRelativeOffset(DWORD_PTR rva) const {
	auto sections = _sections;
	for (int i = 0; i < GetSectionCount(); ++i) {
		if (rva >= sections[i].VirtualAddress && rva < sections[i].VirtualAddress + _sections[i].Misc.VirtualSize)
			return rva - sections[i].VirtualAddress;
	}

	return 0;
}

int PEParser::RVAToSectionIndex(DWORD_PTR rva) const {
	auto sections = _sections;
	for (int i = 0; i < GetSectionCount(); ++i) {
		if (rva >= sections[i].VirtualAddress && rva < sections[i].VirtualAddress + _sections[i].Misc.VirtualSize)
			return i;
	}

	return -1;
}

DWORD PEParser::GetSectionHeaderBasedSizeOfImage() {
	DWORD lastVirtualOffset = 0, lastVirtualSize = 0;

	for (WORD i = 0; i < GetSectionCount(); i++) {
		if ((_sections[i].VirtualAddress + _sections[i].Misc.VirtualSize) > (lastVirtualOffset + lastVirtualSize)) {
			lastVirtualOffset = _sections[i].VirtualAddress;
			lastVirtualSize = _sections[i].Misc.VirtualSize;
		}
	}

	return lastVirtualSize + lastVirtualOffset;
}

bool PEParser::GetImportAddressTable() const {
	auto dir = GetDataDirectory(IMAGE_DIRECTORY_ENTRY_IAT);
	if (dir->Size == 0)
		return false;

	auto table = static_cast<IMAGE_THUNK_DATA64*>(GetAddress(dir->VirtualAddress));

	return true;
}

std::vector<ULONG> PEParser::GetTlsInfo() const {
	std::vector<ULONG> tls;

	auto dir = GetDataDirectory(IMAGE_DIRECTORY_ENTRY_TLS);
	if (dir->Size == 0)
		return tls;

	auto data64 = (IMAGE_TLS_DIRECTORY64*)GetAddress(dir->VirtualAddress);


	return std::vector<ULONG>();
}

bool PEParser::IsImportLib() const {
	return _importLib;
}

bool PEParser::IsObjectFile() const {
	return _opt64 == nullptr;
}

bool PEParser::IsObjectPe64() const {
	auto machine = _fileHeader->Machine;
	return machine == IMAGE_FILE_MACHINE_AMD64 || machine == IMAGE_FILE_MACHINE_ARM64 || machine == IMAGE_FILE_MACHINE_IA64;
}

void* PEParser::GetMemAddress(unsigned rva) const {
	if (!IsValid())
		return nullptr;

	return _address + RvaToFileOffset(rva);
}

ULONG PEParser::GetExportByName(PCSTR exportName) {
	ULONG offset = 0;
	if (!HasExports())
		return 0;

	auto dir = GetDataDirectory(IMAGE_DIRECTORY_ENTRY_EXPORT);
	if (dir == nullptr || dir->Size == 0)
		return 0;

	auto data = static_cast<IMAGE_EXPORT_DIRECTORY*>(GetMemAddress(dir->VirtualAddress));
	auto ordinals = (PBYTE)(data->AddressOfNameOrdinals != 0 ? GetMemAddress(data->AddressOfNameOrdinals) : nullptr);
	auto names = (PUCHAR)(data->AddressOfNames != 0 ? GetMemAddress(data->AddressOfNames) : nullptr);
	auto functions = (PULONG)GetMemAddress(data->AddressOfFunctions);
	auto ordinalBase = data->Base;
	PCSTR Name = nullptr;

	for (ULONG i = 0; i < data->NumberOfNames; ++i) {
		int ordinal = i;
		if (ordinals) {
			ordinal = *(USHORT*)(ordinals + i * 2) + (USHORT)ordinalBase;
		}
		if (names) {
			offset = *(ULONG*)(names + i * 4);
			Name = (PCSTR)GetMemAddress(offset);
		}
		auto address = *(functions + ordinal - ordinalBase);
		if (address > dir->VirtualAddress && address < dir->VirtualAddress + dir->Size) {
			// we ignore forwarded exports
			continue;
		}
		// compare the export name to the requested export
		if (Name) {
			if (!strcmp(Name, exportName)) {
				offset = RvaToFileOffset(address);
				break;
			}
		}
	}

	return offset;
}

IMAGE_SECTION_HEADER* PEParser::GetSections() {
	return _sections;
}

void* PEParser::RVA2FA(unsigned rva) const {
	if (!IsValid())
		return nullptr;

	return _address + RvaToFileOffset(rva);
}

ULONGLONG PEParser::GetImageBase() const {
	return IsPe64() ? GetOptionalHeader64().ImageBase : GetOptionalHeader32().ImageBase;
}

ULONG PEParser::GetEAT() const {
	ULONG offset = 0;
	if (!HasExports())
		return 0;

	auto dir = GetDataDirectory(IMAGE_DIRECTORY_ENTRY_EXPORT);
	if (dir == nullptr || dir->Size == 0)
		return 0;

	auto data = static_cast<IMAGE_EXPORT_DIRECTORY*>(GetMemAddress(dir->VirtualAddress));
	ULONG eat = data->AddressOfFunctions;
	return eat;
}



DWORD PEParser::GetImageSize() const {
	return IsPe64() ? GetOptionalHeader64().SizeOfImage : GetOptionalHeader32().SizeOfImage;
}

DWORD PEParser::GetHeadersSize() const {
	return IsPe64() ? GetOptionalHeader64().SizeOfHeaders : GetOptionalHeader32().SizeOfHeaders;
}

DWORD PEParser::GetAddressOfEntryPoint() const {
	return IsPe64() ? GetOptionalHeader64().AddressOfEntryPoint : GetOptionalHeader32().AddressOfEntryPoint;
}

std::vector<RelocInfo> PEParser::GetRelocs(void* image_base) {
	std::vector<RelocInfo> relocs;
	auto dir = GetDataDirectory(IMAGE_DIRECTORY_ENTRY_BASERELOC);
	DWORD reloc_va = dir->VirtualAddress;
	if (!reloc_va)
		return {};

	auto current_base_relocation = reinterpret_cast<PIMAGE_BASE_RELOCATION>(reinterpret_cast<uint64_t>(image_base) + reloc_va);
	const auto reloc_end = reinterpret_cast<PIMAGE_BASE_RELOCATION>(reinterpret_cast<uint64_t>(current_base_relocation) + dir->Size);
	
	while (current_base_relocation < reloc_end && current_base_relocation->SizeOfBlock) {
		RelocInfo reloc_info;

		reloc_info.address = reinterpret_cast<uint64_t>(image_base) + current_base_relocation->VirtualAddress;
		reloc_info.item = reinterpret_cast<uint16_t*>(reinterpret_cast<uint64_t>(current_base_relocation) + sizeof(IMAGE_BASE_RELOCATION));
		reloc_info.count = (current_base_relocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(uint16_t);
		relocs.push_back(reloc_info);

		current_base_relocation = reinterpret_cast<PIMAGE_BASE_RELOCATION>(reinterpret_cast<uint64_t>(current_base_relocation) + current_base_relocation->SizeOfBlock);
	}

	return relocs;
}

LARGE_INTEGER PEParser::GetFileSize() const {
	return _fileSize;
}

void PEParser::RelocateImageByDelta(std::vector<RelocInfo>& relocs, const uint64_t delta) {
	for (const auto& current_reloc : relocs) {
		for (auto i = 0u; i < current_reloc.count; ++i) {
			const uint16_t type = current_reloc.item[i] >> 12;
			const uint16_t offset = current_reloc.item[i] & 0xFFF;
			if (type == IMAGE_REL_BASED_DIR64)
				*reinterpret_cast<uint64_t*>(current_reloc.address + offset) += delta;
		}
	}
}

PVOID PEParser::GetDataDirectoryAddress(UINT index, PULONG size) const {
	return ::ImageDirectoryEntryToData(_address, FALSE, index, size);
}

void PEParser::SetDefaultFileAligment() {
	if (IsPe64()) {
		GetOptionalHeader64().FileAlignment = _fileAlignmentConstant;
	}
	else {
		GetOptionalHeader32().FileAlignment = _fileAlignmentConstant;
	}
}

DWORD PEParser::GetSectionAlignment() {
	if (IsPe64()) {
		return GetOptionalHeader64().SectionAlignment;
	}
	else {
		return GetOptionalHeader32().SectionAlignment;
	}
}

DWORD PEParser::GetFileAlignment() {
	if (IsPe64()) {
		return GetOptionalHeader64().FileAlignment;
	}
	else {
		return GetOptionalHeader32().FileAlignment;
	}
}

DWORD PEParser::AlignValue(DWORD badValue, DWORD alignTo) {
	return (badValue + alignTo - 1) & ~(alignTo - 1);
}

void PEParser::AlignAllSectionHeaders() {
	auto sections = _sections;
	DWORD sectionAlignment = GetSectionAlignment();
	DWORD fileAlignment = GetFileAlignment();
	DWORD newFileSize = 0;
	

	std::sort(_PESections.begin(), _PESections.end(), [](const PEFileSection& d1, const PEFileSection& d2) {
		return d1._sectionHeader.PointerToRawData < d2._sectionHeader.PointerToRawData;
	});

	newFileSize = _dosHeader->e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) +
		_ntHeader->FileHeader.SizeOfOptionalHeader + GetSectionCount() * sizeof(IMAGE_SECTION_HEADER);

	for (int i = 0; i < GetSectionCount(); ++i) {
		_PESections[i]._sectionHeader.VirtualAddress = AlignValue(sections[i].VirtualAddress, sectionAlignment);
		_PESections[i]._sectionHeader.Misc.VirtualSize = AlignValue(sections[i].Misc.VirtualSize, sectionAlignment);

		_PESections[i]._sectionHeader.PointerToRawData = AlignValue(newFileSize, fileAlignment);
		_PESections[i]._sectionHeader.SizeOfRawData = AlignValue(_PESections[i]._dataSize, fileAlignment);
		
		newFileSize = _PESections[i]._sectionHeader.PointerToRawData + _PESections[i]._sectionHeader.SizeOfRawData;
	}

	std::sort(_PESections.begin(), _PESections.end(), [](const PEFileSection& d1, const PEFileSection& d2) {
		return d1._sectionHeader.VirtualAddress < d2._sectionHeader.VirtualAddress;
	});
}

void PEParser::FixPEHeader() {
	DWORD size = _dosHeader->e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER);

	if (!IsPe64()) {
		_opt32->DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = 0;
		_opt32->DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = 0;

		for (DWORD i = _opt32->NumberOfRvaAndSizes; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i++) {
			_opt32->DataDirectory[i].Size = 0;
			_opt32->DataDirectory[i].VirtualAddress = 0;
		}
		_opt32->NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
		_fileHeader->SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER32);

		_opt32->SizeOfImage = GetSectionHeaderBasedSizeOfImage();

		if (_moduleBase) {
			_opt32->ImageBase = _moduleBase;;
		}

		_opt32->SizeOfHeaders = AlignValue(size + _ntHeader->FileHeader.SizeOfOptionalHeader
			+ GetSectionCount() * sizeof(IMAGE_SECTION_HEADER), _opt32->FileAlignment);
	}
	else {
		_opt64->DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = 0;
		_opt64->DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = 0;

		for (DWORD i = _opt64->NumberOfRvaAndSizes; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i++) {
			_opt64->DataDirectory[i].Size = 0;
			_opt64->DataDirectory[i].VirtualAddress = 0;
		}

		_opt64->NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
		_fileHeader->SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64);

		_opt64->SizeOfImage = GetSectionHeaderBasedSizeOfImage();

		if (_moduleBase) {
			_opt64->ImageBase = _moduleBase;
		}

		_opt64->SizeOfHeaders = AlignValue(size + _fileHeader->SizeOfOptionalHeader +
			+GetSectionCount() * sizeof(IMAGE_SECTION_HEADER), _opt64->FileAlignment);
	}

	RemoveIATDirectory();
}

void PEParser::RemoveIATDirectory() {
	DWORD searchAddress = 0;

	if (!IsPe64()) {
		searchAddress = _opt32->DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress;
		_opt32->DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress = 0;
		_opt32->DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size = 0;
	}
	else {
		searchAddress = _opt64->DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress;
		_opt64->DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress = 0;
		_opt64->DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size = 0;
	}

	if (searchAddress) {
		for (WORD i = 0; i < GetSectionCount(); i++) {
			if (_sections[i].VirtualAddress <= searchAddress &&
				(_sections[i].VirtualAddress + _sections[i].Misc.VirtualSize) > searchAddress) {
				_sections[i].Characteristics |= IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
			}
		}
	}
}

DWORD PEParser::GetSectionHeaderBasedFileSize() {
	DWORD lastRawOffset = 0, lastRawSize = 0;

	for (WORD i = 0; i < GetSectionCount(); i++) {
		auto section = _sections[i];
		if ((section.PointerToRawData + section.SizeOfRawData) > (lastRawOffset + lastRawSize))
		{
			lastRawOffset = section.PointerToRawData;
			lastRawSize = section.SizeOfRawData;
		}
	}

	return lastRawSize + lastRawOffset;
}

bool PEParser::AddNewLastSection(const char* pSectionName, DWORD sectionSize, BYTE* pSectionData) {
	size_t len = strlen(pSectionName);
	DWORD fileAlignment = 0, sectionAlignment = 0;
	PEFileSection peFileSection;

	if (len > IMAGE_SIZEOF_SHORT_NAME) {
		return false;
	}

	if (!IsPe64()) {
		fileAlignment = _opt32->FileAlignment;
		sectionAlignment = _opt32->SectionAlignment;
	}
	else {
		fileAlignment = _opt64->FileAlignment;
		sectionAlignment = _opt64->SectionAlignment;
	}

	memcpy_s(peFileSection._sectionHeader.Name, IMAGE_SIZEOF_SHORT_NAME, pSectionName, len);

	peFileSection._sectionHeader.SizeOfRawData = sectionSize;
	peFileSection._sectionHeader.Misc.VirtualSize = AlignValue(sectionSize, sectionAlignment);

	peFileSection._sectionHeader.PointerToRawData = AlignValue(GetSectionHeaderBasedFileSize(), fileAlignment);
	peFileSection._sectionHeader.VirtualAddress = AlignValue(GetSectionHeaderBasedSizeOfImage(), sectionAlignment);

	peFileSection._sectionHeader.Characteristics = IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ 
		| IMAGE_SCN_MEM_WRITE | IMAGE_SCN_CNT_CODE | IMAGE_SCN_CNT_INITIALIZED_DATA;
	
	peFileSection._normalSize = peFileSection._sectionHeader.SizeOfRawData;
	peFileSection._dataSize = peFileSection._sectionHeader.SizeOfRawData;

	if (pSectionData == nullptr) {
		peFileSection._pData = new BYTE[peFileSection._sectionHeader.SizeOfRawData];
		ZeroMemory(peFileSection._pData, peFileSection._sectionHeader.SizeOfRawData);
	}
	else {
		peFileSection._pData = pSectionData;
	}

	_PESections.push_back(peFileSection);

	SetSectionCount(GetSectionCount() + 1);

	return true;
}

void PEParser::SetEntryPointRVA(DWORD rva) {
	if (IsPe64()) {
		_opt64->AddressOfEntryPoint = rva;
	}
	else {
		_opt32->AddressOfEntryPoint = rva;
	}
}

void PEParser::SetEntryPointVA(DWORD_PTR entryPoint) {
	DWORD rva = (DWORD)(entryPoint - _moduleBase);
	SetEntryPointRVA(rva);
}

bool PEParser::DumpProcess(DWORD_PTR modBase, DWORD_PTR entryPoint, const WCHAR* pDumpFilePath) {
	_moduleBase = modBase;
	GetPESections();
	SetDefaultFileAligment();
	SetEntryPointVA(entryPoint);
	AlignAllSectionHeaders();
	FixPEHeader();
	
	return SavePEFileToDisk(pDumpFilePath);
}

void PEParser::GetPESections() {
	DWORD_PTR offset = 0;

	int count = GetSectionCount();
	_PESections.reserve(count);

	for (WORD i = 0; i < count; i++) {
		_PESections[i]._normalSize = _sections[i].Misc.VirtualSize;
		_PESections[i]._dataSize = _sections[i].Misc.VirtualSize;
		offset = _sections[i].VirtualAddress;
		GetPESectionData(offset, _PESections[i]);
	}
}

void PEParser::GetPESectionData(DWORD_PTR offset, PEFileSection& peFileSection) {
	peFileSection._pData = new BYTE[peFileSection._dataSize];
	memcpy(peFileSection._pData, _address + offset, peFileSection._dataSize);
}

void PEParser::SetModuleBase(DWORD_PTR va) {
	_moduleBase = va;
}

bool PEParser::OpenWriteFileHandle(const WCHAR* pNewFile) {
	if (pNewFile) {
		_hFile = ::CreateFile(pNewFile, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	}

	return _hFile != INVALID_HANDLE_VALUE;
}

bool PEParser::WriteMemoryToFile(HANDLE hFile, LONG offset, DWORD size, LPVOID pData) {
	DWORD bytes;
	DWORD ret;
	DWORD error;

	if (hFile != INVALID_HANDLE_VALUE) {
		ret = SetFilePointer(hFile, offset, nullptr, FILE_BEGIN);
		if (ret == INVALID_SET_FILE_POINTER) {
			error = GetLastError();
			return false;
		}
		return WriteFile(hFile, pData, size, &bytes, nullptr);
	}
	return false;
}

bool PEParser::SavePEFileToDisk(const WCHAR* pNewFile) {
	bool ret = true;

	DWORD fileOffset = 0, writeSize = 0;

	do
	{
		ret = OpenWriteFileHandle(pNewFile);
		if (!ret)
			break;

		writeSize = sizeof(IMAGE_DOS_HEADER);

		if (!WriteMemoryToFile(_hFile, fileOffset, writeSize, GetBaseAddress())) {
			ret = false;
			break;
		}

		fileOffset += writeSize;

		LONG e_lfanew = GetDosHeader().e_lfanew;
		DWORD dosStubSize = 0;
		if (e_lfanew > sizeof(IMAGE_DOS_HEADER)) {
			dosStubSize = e_lfanew - sizeof(IMAGE_DOS_HEADER);

			BYTE* pDosStub = GetDosStub();
			if (pDosStub) {
				writeSize = dosStubSize;
				if (!WriteMemoryToFile(_hFile, fileOffset, writeSize, pDosStub)) {
					ret = false;
					break;
				}
				fileOffset += writeSize;
			}
		}

		if (!IsPe64()) {
			writeSize = sizeof(IMAGE_NT_HEADERS32);
		}
		else {
			writeSize = sizeof(IMAGE_NT_HEADERS64);
		}
		PIMAGE_NT_HEADERS64 pNtHeader = GetNtHeader();
		if (!WriteMemoryToFile(_hFile, fileOffset, writeSize, pNtHeader)) {
			ret = false;
			break;
		}

		fileOffset += writeSize;

		writeSize = sizeof(IMAGE_SECTION_HEADER);
		auto sections = GetSections();
		for (WORD i = 0; i < GetSectionCount(); i++) {
			if (!WriteMemoryToFile(_hFile, fileOffset, writeSize, &_PESections[i]._sectionHeader)) {
				ret = false;
				break;
			}
			fileOffset += writeSize;
		}


		for (WORD i = 0; i < GetSectionCount(); i++) {
			if (!_PESections[i]._sectionHeader.PointerToRawData)
				continue;

			if (_PESections[i]._sectionHeader.PointerToRawData > fileOffset) {
				writeSize = _PESections[i]._sectionHeader.PointerToRawData - fileOffset;

				if (!WriteZeroMemoryToFile(_hFile, fileOffset, writeSize)) {
					ret = false;
					break;
				}
				fileOffset += writeSize;
			}

			writeSize = _PESections[i]._sectionHeader.SizeOfRawData;

			if (writeSize) {
				if (!WriteMemoryToFile(_hFile, _PESections[i]._sectionHeader.PointerToRawData, 
					writeSize, _PESections[i]._pData)) {
					ret = false;
					break;
				}
				fileOffset += writeSize;

				if (_PESections[i]._dataSize < _PESections[i]._sectionHeader.SizeOfRawData) {
					writeSize = _PESections[i]._sectionHeader.SizeOfRawData - _PESections[i]._dataSize;
					if (!WriteZeroMemoryToFile(_hFile, fileOffset, writeSize)) {
						ret = false;
						break;
					}
					fileOffset += writeSize;
				}
			}
		}

		DWORD overlayOffset = GetSectionHeaderBasedFileSize();

		LARGE_INTEGER fileSize = GetFileSize();
		if (fileSize.QuadPart > 0) {
			DWORD overlaySize = fileSize.QuadPart - overlayOffset;

			if (overlaySize) {
				writeSize = overlaySize;
				BYTE* pOverlayData = GetFileAddress(overlayOffset);
				if (!WriteMemoryToFile(_hFile, fileOffset, writeSize, pOverlayData)) {
					ret = false;
					break;
				}
				fileOffset += writeSize;
			}
		}

		SetEndOfFile(_hFile);
	} while (false);


	if (_hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(_hFile);
		_hFile = INVALID_HANDLE_VALUE;
	}

	return ret;
}

bool PEParser::WriteZeroMemoryToFile(HANDLE hFile, DWORD fileOffset, DWORD size) {
	bool ret = false;
	PVOID pZeroMemory = calloc(size, 1);

	if (pZeroMemory) {
		ret = WriteMemoryToFile(hFile, fileOffset, size, pZeroMemory);
		free(pZeroMemory);
	}

	return ret;
}