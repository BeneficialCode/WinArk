#include "pch.h"
#include "PEParser.h"

PEParser::PEParser(PVOID base) {
	_address = reinterpret_cast<PUCHAR>(base);
	CheckValidity();
}

PEParser::PEParser(const wchar_t* path) {
	UNICODE_STRING fileName;
	RtlInitUnicodeString(&fileName, path);
	NTSTATUS status = _file.Open(&fileName, FileAccessMask::Read);
	IO_STATUS_BLOCK ioStatus;
	if (NT_SUCCESS(status)) {
		LARGE_INTEGER fileSize;
		_file.GetFileSize(&fileSize);
		_address = (PUCHAR)ExAllocatePool(NonPagedPool, fileSize.QuadPart); // 未释放申请的内存
		status = _file.ReadFile(_address, fileSize.LowPart, &ioStatus, nullptr);
		if (!NT_SUCCESS(status)) {
			if (_address)
				ExFreePool(_address);
		}
		_file.Close();
	}
	CheckValidity();
	return;
}

PEParser::~PEParser() {

}

void PEParser::CheckValidity() {
	_dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(_address);
	if (_dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		_valid = false;
		return;
	}
	else {
		auto ntHeader = (PIMAGE_NT_HEADERS64)(_address + _dosHeader->e_lfanew);
		if (ntHeader->Signature != IMAGE_NT_SIGNATURE) {
			_valid = false;
			return;
		}
		_ntHeader = ntHeader;
		_fileHeader = &ntHeader->FileHeader;
		_opt64 = &ntHeader->OptionalHeader;
		_opt32 = (PIMAGE_OPTIONAL_HEADER32)_opt64;
		_sections = (PIMAGE_SECTION_HEADER)((UCHAR*)_opt64 + _fileHeader->SizeOfOptionalHeader);
		_valid = true;
	}
}

int PEParser::GetSectionCount() const {
	if (!IsValid())
		return -1;

	return _fileHeader->NumberOfSections;
}

ULONG PEParser::RvaToFileOffset(ULONG rva) const {
	auto sections = _sections;
	for (int i = 0; i < GetSectionCount(); ++i) {
		if (rva >= sections[i].VirtualAddress && rva < sections[i].VirtualAddress + _sections[i].Misc.VirtualSize)	// 在节区内
			return sections[i].PointerToRawData + rva - sections[i].VirtualAddress;
	}
	return rva;
}

bool PEParser::IsValid() const {
	return _valid;
}

bool PEParser::IsPe64() const {
	return _opt64 ? _opt64->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC : IsObjectPe64();
}

bool PEParser::IsObjectPe64() const {
	auto machine = _fileHeader->Machine;
	return machine == IMAGE_FILE_MACHINE_AMD64 || machine == IMAGE_FILE_MACHINE_ARM64 || machine == IMAGE_FILE_MACHINE_IA64;
}

bool PEParser::HasExports() const {
	return _opt64 ? GetDataDirectory(IMAGE_DIRECTORY_ENTRY_EXPORT)->VirtualAddress != 0 : false;
}


const IMAGE_DATA_DIRECTORY* PEParser::GetDataDirectory(int index) const {
	if (_opt64 == nullptr || _opt32 == nullptr)
		return nullptr;
	if (!IsValid() || index < 0 || index>15)
		return nullptr;

	return IsPe64() ? &_opt64->DataDirectory[index] : &_opt32->DataDirectory[index];
}

void* PEParser::GetAddress(ULONG rva) const {
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

	auto data = static_cast<IMAGE_EXPORT_DIRECTORY*>(GetAddress(dir->VirtualAddress));
	auto ordinals = (PUCHAR)(data->AddressOfNameOrdinals != 0 ? GetAddress(data->AddressOfNameOrdinals) : nullptr);
	auto names = (PUCHAR)(data->AddressOfNames != 0 ? GetAddress(data->AddressOfNames) : nullptr);
	auto functions = (PULONG)GetAddress(data->AddressOfFunctions);
	auto ordinalBase = data->Base;
	PCSTR name = nullptr;

	for (ULONG i = 0; i < data->NumberOfNames; ++i) {
		int ordinal = i;
		if (ordinals) {
			ordinal = *(USHORT*)(ordinals + i * 2) + (USHORT)ordinalBase;
		}
		if (names) {
			offset = *(ULONG*)(names + i * 4);
			name = (PCSTR)GetAddress(offset);
		}
		auto address = *(functions + ordinal - ordinalBase);
		if (address > dir->VirtualAddress && address < dir->VirtualAddress + dir->Size) {
			// we ignore forwarded exports
			continue;
		}
		// compare the export name to the requested export
		if (name) {
			if (!strcmp(name, exportName)) {
				offset = RvaToFileOffset(address);
				break;
			}
		}
	}

	return offset;
}

PIMAGE_SECTION_HEADER PEParser::RvaToSection(PIMAGE_NT_HEADERS ntHeader, PVOID base, ULONG rva) {
	PIMAGE_SECTION_HEADER sections = IMAGE_FIRST_SECTION(ntHeader);
	for (int i = 0; i < ntHeader->FileHeader.NumberOfSections; i++) {
		if (sections[i].VirtualAddress <= rva) {
			if ((sections[i].VirtualAddress + sections[i].Misc.VirtualSize) > rva) {
				return &sections[i];
			}
		}
	}
	return nullptr;
}
void* PEParser::GetBaseAddress() const {
	return _address;
}

void* PEParser::GetAddressEntryPoint() const {
	return _opt64->AddressOfEntryPoint + _address;
}

const IMAGE_SECTION_HEADER* PEParser::GetSectionHeader(ULONG section) const {
	if (!IsValid() || section >= _fileHeader->NumberOfSections)
		return nullptr;

	return _sections + section;
}

IMAGE_SECTION_HEADER* PEParser::GetSectionHeader(ULONG section) {
	if (!IsValid() || section >= _fileHeader->NumberOfSections)
		return nullptr;

	return _sections + section;
}