#pragma once

#include "FileManager.h"
#include <ntimage.h>

struct ExportSymbol {
	ULONG Address;
	unsigned short Oridinal;
	int Hint;
};

class PEParser final {
public:
	PEParser(const wchar_t* path);
	PEParser(PVOID base);
	~PEParser();

	bool IsValid() const;
	bool IsPe64() const;
	bool HasExports() const;

	int GetSectionCount() const;
	const IMAGE_DATA_DIRECTORY* GetDataDirectory(int index) const;
	const IMAGE_SECTION_HEADER* GetSectionHeader(ULONG section) const;


	bool IsObjectPe64() const;
	void CheckValidity();
	ULONG RvaToFileOffset(ULONG rva) const;
	ULONG GetExportByName(PCSTR exportName);

	void* GetAddress(ULONG rva) const;
	void* GetBaseAddress() const;

	void* GetAddressEntryPoint() const;

	static PIMAGE_SECTION_HEADER RvaToSection(PIMAGE_NT_HEADERS ntHeader, PVOID base, ULONG rva);

private:
	FileManager _file;
	PUCHAR _address = nullptr;	
	IMAGE_DOS_HEADER* _dosHeader;
	IMAGE_NT_HEADERS* _ntHeader;
	IMAGE_FILE_HEADER* _fileHeader;
	IMAGE_SECTION_HEADER* _sections;
	IMAGE_OPTIONAL_HEADER32* _opt32;
	IMAGE_OPTIONAL_HEADER64* _opt64;
	bool _valid = false;
};