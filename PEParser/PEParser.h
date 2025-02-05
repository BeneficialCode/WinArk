#pragma once
class CLRMetadataParser;

struct ExportedSymbol {
	std::string Name;
	std::string UndecoratedName;
	std::string ForwardName;
	DWORD Address;
	unsigned short Ordinal;
	bool IsForward;
	WORD Hint;
	bool HasName;
};

struct ImportedSymbol {
	std::string Name;
	std::string UndecoratedName;
	unsigned short Hint;
};

struct ImportedLibrary {
	std::string Name;
	DWORD IAT;
	std::vector<ImportedSymbol> Symbols;
};

enum class SubsystemType :short {
	Unknown,
	Native,
	WindowsGUI,
	WindowsCUI,
	PosixCUI = 7,
	WindowsCEGUI = 9,
	EfiApplication = 10,
	EfiBootServiceDriver = 11,
	EfiRuntimeDriver = 12,
	EfiROM = 13,
	XBOX = 14
};

enum class CfgFlags :uint32_t {
	CfInstrumented = 0x100,
	CfwInstrumented = 0x200,
	CfFunctionTablePresent = 0x400,
	SecurityCookieUnused = 0x800,
	ProtectDelayLoadIat = 0x1000,
	DelayLoadIatInItsOwnSection = 0x2000,
	CfExportSuppressionInfoPresent = 0x4000,
	CfEnableExportSuppression = 0x8000,
	CfLongJmpTablePresent = 0x10000,
	CfFunctionTableSizeMask = 0xf0000000,
};
DEFINE_ENUM_FLAG_OPERATORS(CfgFlags);


enum class ImageCharacteristics :unsigned short {
	None = 0,
	RelocsStripped = 1,
	ExecutableImage = 2,
	LineNumsStripped = 4,
	LocalSymbolsStripped = 8,
	AggressiveTrimWorkingSet = 0x10,
	LargeAddressAware = 0x20,
	LittleEndian = 0x80,
	Machine32Bit = 0x100,
	DebugInfoStripped = 0x200,
	RemovableRunFromSwap = 0x400,
	NetRunFromSwap = 0x800,
	SystemFile = 0x1000,
	DllFile = 0x2000,
	SingleCpuOnly = 0x4000,
	BigEndian = 0x8000
};
DEFINE_ENUM_FLAG_OPERATORS(ImageCharacteristics)

enum class ImageDebugType {
	Unknown,
	Coff,
	CodeView,
	Fpo,
	Misc,
	Exception,
	Fixup,
	Borland = 9,
	Repro = 16
};

enum class ControlFlowGuardFlags {
	Instrumented = 0x100,
	WriteInstrumented = 0x200,
	FunctionTablePresent = 0x400,
	SecurityCookieUnused = 0x800,
	ProtectDelayLoadIAT = 0x1000,
	DelayLoadIATOwnSection = 0x2000,
	ExportSuppressInfo = 0x4000,
	EnableExportSuppression = 0x8000,
	LongJumpTablePresent = 0x10000
};

enum class SectionFlags :unsigned {
	None = 0,
	NoPad = 8,
	Code = 0x20,
	InitializedData = 0x40,
	UninitializedData = 0x80,
	Other = 0x100,
	Info = 0x200,
	Remove = 0x800,
	Comdat = 0x1000,
	GPRel = 0x80000,
	Align1Byte = 0x100000,
	Align2Bytes = 0x200000,
	ExtendedReloc = 0x1000000,
	Discardable = 0x2000000,
	NotCached = 0x4000000,
	NotPaged = 0x8000000,
	Shared = 0x10000000,
	Execute = 0x20000000,
	Read = 0x40000000,
	Write = 0x80000000,
};
DEFINE_ENUM_FLAG_OPERATORS(SectionFlags)

enum class OptionalHeaderMagic : short {
	PE32 = 0x10b,
	PE32Plus = 0x20b,
	Rom = 0x107
};

enum class MachineType : unsigned short {
	Unknown = 0,
	X86 = 332,
	Arm = 0x1c0,
	Arm_NT = 0x1c4,
	IA64 = 512,
	Amd64 = 0x8664,
	Arm64 = 0xAA64,
};

struct ResourceInfo {
	std::string Name;
	DWORD Rva;
	DWORD Size;
	void* Address;
	WORD Id;
	bool IsId{ false };
};

class PEFileSection {
public:
	IMAGE_SECTION_HEADER _sectionHeader;
	BYTE* _pData = nullptr;
	DWORD _dataSize = 0;
	DWORD _normalSize = 0;

	PEFileSection() {
		ZeroMemory(&_sectionHeader, sizeof(IMAGE_SECTION_HEADER));
	}
};

struct RelocInfo {
	uint64_t address;
	uint16_t* item;
	uint32_t count;
};

class PEParser {
public:
	explicit PEParser(const wchar_t* path,bool isScylla = false);
	~PEParser();
	PEParser(void* base);

	bool IsValid() const;
	bool IsPe64() const;
	bool IsExecutable() const;
	bool IsManaged() const;
	bool HasExports() const;
	bool HasImports() const;
	bool IsSystemFile() const;

	void FixPEHeader();
	bool DumpProcess(DWORD_PTR modBase, DWORD_PTR entryPoint, const WCHAR* pDumpFilePath);
	void GetPESections();
	void GetPESectionData(DWORD_PTR offset, PEFileSection& peFileSection);
	void SetEntryPointVA(DWORD_PTR entryPoint);
	void SetEntryPointRVA(DWORD rva);
	void SetModuleBase(DWORD_PTR va);
	bool SavePEFileToDisk(const WCHAR* pNewFile);
	bool OpenWriteFileHandle(const WCHAR* pNewFile);
	bool WriteMemoryToFile(HANDLE hFile, LONG offset, DWORD size, LPVOID pData);
	bool WriteZeroMemoryToFile(HANDLE hFile, DWORD fileOffset, DWORD size);
	void GetSectionHeaders();

	int GetSectionCount() const;
	void SetSectionCount(WORD count);
	const IMAGE_SECTION_HEADER* GetSectionHeader(ULONG section) const;
	const IMAGE_DATA_DIRECTORY* GetDataDirectory(int index) const;
	const IMAGE_DOS_HEADER& GetDosHeader() const;
	void* GetBaseAddress() const;
	void AlignAllSectionHeaders();
	DWORD AlignValue(DWORD badValue, DWORD alignTo);

	ULONGLONG GetImageBase() const;

	void RemoveIATDirectory();

	DWORD GetSectionHeaderBasedFileSize();
	DWORD GetSectionHeaderBasedSizeOfImage();

	DWORD GetImageSize() const;
	DWORD GetHeadersSize() const;
	DWORD GetAddressOfEntryPoint() const;

	ULONG GetEAT() const;

	std::string GetSectionName(ULONG section) const;

	void* RVA2FA(unsigned rva) const;


	std::vector<ExportedSymbol> GetExports() const;
	std::vector<ImportedLibrary> GetImports() const;
	const IMAGE_FILE_HEADER& GetFileHeader() const;
	//std::vector<ResourceInfo> EnumResources() const;
	bool GetImportAddressTable() const;
	//bool IsCLRMetadataAvailable() const;

	std::vector<ULONG> GetTlsInfo() const;

	void* GetAddress(unsigned rva) const;

	void* GetMemAddress(unsigned rva) const;

	BYTE* GetFileAddress(DWORD offset);

	SubsystemType GetSubsystemType() const;

	IMAGE_OPTIONAL_HEADER64& GetOptionalHeader64() const {
		return *_opt64;
	}

	IMAGE_OPTIONAL_HEADER32& GetOptionalHeader32() const {
		return *_opt32;
	}

	//IMAGE_COR20_HEADER* GetCLRHeader() const;
	//CLRMetadataParser* GetCLRParser() const;
	//std::vector<std::pair<DWORD, WIN_CERTIFICATE>> EnumCertificates() const;
	//const IMAGE_LOAD_CONFIG_DIRECTORY64* GetLoadConfiguration64() const;
	//const IMAGE_LOAD_CONFIG_DIRECTORY32* GetLoadConfiguration32() const;
	PVOID GetDataDirectoryAddress(UINT index, PULONG size) const;
	void SetDefaultFileAligment();
	DWORD GetSectionAlignment();
	DWORD GetFileAlignment();

	bool IsImportLib() const;
	bool IsObjectFile() const;
	ULONG GetExportByName(PCSTR exportName);
	HANDLE GetFileHandle();
	unsigned RvaToFileOffset(unsigned rva) const;
	DWORD_PTR RVAToRelativeOffset(DWORD_PTR rva) const;
	int RVAToSectionIndex(DWORD_PTR rva) const;
	IMAGE_SECTION_HEADER* GetSections();
	DWORD_PTR FileOffsetToRva(DWORD_PTR offset);

	LARGE_INTEGER GetFileSize() const;

	std::vector<RelocInfo> GetRelocs(void* imageBase);
	static void RelocateImageByDelta(std::vector<RelocInfo>& relocs, const uint64_t delta);

	BYTE* GetDosStub();

	IMAGE_NT_HEADERS64* GetNtHeader();

	std::vector<PEFileSection> _PESections;

protected:
	static const DWORD _fileAlignmentConstant = 0x200;

	bool AddNewLastSection(const char* pSectionName, DWORD sectionSize, BYTE* pSectionData);

private:
	bool IsObjectPe64() const;
	void CheckValidity();


	//CString GetResourceName(void* data) const;

	PBYTE _address{ nullptr };
	DWORD_PTR _moduleBase = 0;
	LARGE_INTEGER _fileSize{ 0 };
	HMODULE _module{ nullptr };
	HANDLE _hMemMap{ nullptr };
	HANDLE _hFile{ INVALID_HANDLE_VALUE };
	IMAGE_DOS_HEADER* _dosHeader = nullptr;
	IMAGE_NT_HEADERS64* _ntHeader = nullptr;
	IMAGE_FILE_HEADER* _fileHeader = nullptr;
	IMAGE_SECTION_HEADER* _sections = nullptr;
	IMAGE_OPTIONAL_HEADER32* _opt32{ nullptr };
	IMAGE_OPTIONAL_HEADER64* _opt64{ nullptr };
	//CComPtr<IMetaDataImport> _spMetadata;
	std::wstring _path;
	mutable HMODULE _resModule{ nullptr };
	//mutable std::unique_ptr<CLRMetaParser> _clrParser;
	bool _valid = false;
	bool _importLib = false, _objectFile = false;
	bool _isFileMap = true;
};