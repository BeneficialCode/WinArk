#include "pch.h"
#include <string>
#include <vector>
#include <atlstr.h>
#include "PEParser.h"
#include "CLRMetadataParser.h"
#include <unordered_map>
#pragma comment(lib,"imagehlp")


PEParser::PEParser(const wchar_t* path) :_path(path) {
	_hFile = ::CreateFile(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (_hFile == INVALID_HANDLE_VALUE)
		return;
	::GetFileSizeEx(_hFile, &_fileSize);
	_hMemMap = ::CreateFileMapping(_hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
	if (!_hMemMap)
		return;

	_address = (PBYTE)::MapViewOfFile(_hMemMap, FILE_MAP_READ, 0, 0, 0);
	if (!_address)
		return;

	CheckValidity();
	if (IsValid() && IsManaged()) {

	}
}

PEParser::PEParser(void* base) {
	_address = reinterpret_cast<PUCHAR>(base);
	CheckValidity();
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

	auto data = static_cast<IMAGE_EXPORT_DIRECTORY*>(GetAddress(dir->VirtualAddress));
	auto count = data->NumberOfFunctions;
	exports.reserve(count);

	auto names = (PBYTE)(data->AddressOfNames != 0 ? GetAddress(data->AddressOfNames) : nullptr);
	auto ordinals = (PBYTE)(data->AddressOfNameOrdinals != 0 ? GetAddress(data->AddressOfNameOrdinals) : nullptr);
	auto functions = (PDWORD)GetAddress(data->AddressOfFunctions);
	char undecorated[1 << 10];
	auto ordinalBase = data->Base;

	std::unordered_map<uint32_t, std::string> functionNamesMap;
	for (uint32_t idx = 0; idx < data->NumberOfNames; idx++) {
		uint16_t ordinal;
		ordinal = *(USHORT*)(ordinals + idx * 2) + (USHORT)ordinalBase;
		uint32_t name;
		auto offset = *(ULONG*)(names + idx * 4);
		functionNamesMap[ordinal] = (PCSTR)GetAddress(offset);
	}

	for (DWORD i = 0; i < data->NumberOfFunctions; i++) {
		ExportedSymbol symbol;
		int ordinal = i + (USHORT)ordinalBase;
		symbol.Ordinal = ordinal;
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
		//auto offset = RvaToFileOffset(address);
		if (hasName) {
			if (address > dir->VirtualAddress && address < dir->VirtualAddress + dir->Size) {
				symbol.ForwardName = (PCSTR)GetAddress(address);
			}
		}
		exports.push_back(std::move(symbol));
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
		// perhaps object file / static library
		_fileHeader = (IMAGE_FILE_HEADER*)_address;
		_sections = (PIMAGE_SECTION_HEADER)(_fileHeader + 1);
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