#pragma once

#include <string>
#include <vector>
#include <list>
#include <DbgHelp.h>

enum class ProcessAccessMask : unsigned {
	Query = 0x400
};

enum class BasicType {
	NoType = 0,
	Void = 1,
	Char = 2,
	WChar = 3,
	Int = 6,
	UInt = 7,
	Float = 8,
	BCD = 9,
	Bool = 10,
	Long = 13,
	ULong = 14,
	Currency = 25,
	Date = 26,
	Variant = 27,
	Complex = 28,
	Bit = 29,
	BSTR = 30,
	Hresult = 31
};

enum class UdtKind {
	Struct,
	Class,
	Union,
	Interface,
	Unknown = 99
};

enum class SymbolTag{
	Null,
	Exe,
	Compiland,
	CompilandDetails,
	CompilandEnv,
	Function,
	Block,
	Data,
	Annotation,
	Label,
	PublicSymbol,
	UDT,
	Enum,
	FunctionType,
	PointerType,
	ArrayType,
	BaseType,
	Typedef,
	BaseClass,
	Friend,
	FunctionArgType,
	FuncDebugStart,
	FuncDebugEnd,
	UsingNamespace,
	VTableShape,
	VTable,
	Custom,
	Thunk,
	CustomType,
	ManagedType,
	Dimension,
	CallSite,
	InlineSite,
	BaseInterface,
	VectorType,
	MatrixType,
	HLSLType,
	Caller,
	Callee,
	Export,
	HeapAllocationSite,
	CoffGroup,
	Max
};
DEFINE_ENUM_FLAG_OPERATORS(SymbolTag);

enum class SymbolTypeInfo {
	Tag,
	Name,
	Length,
	Type,
	TypeId,
	BaseType,
	ArrayIndexTypeId,
	FindChildren,
	DataKind,
	AddressOffset,
	Offset,
	Value,
	Count,
	ChildrenCount,
	BitPosition,
	VirtualBaseClass,
	VIRTUALTABLESHAPEID,
	VIRTUALBASEPOINTEROFFSET,
	ClassParentId,
	Nested,
	SymIndex,
	LexicalParent,
	Address,
	ThisAdjust,
	UdtKind,
	IsEquivalentTo,
	CallingConvention,
	IsCloseEquivalentTo,
	GTIEX_REQS_VALID,
	VirtualBaseOffset,
	VirtualBaseDispIndex,
	IsReference,
	IndirectVirtualBaseClass
};

enum class SymbolFlags : unsigned {
	None = 0,
	ClrToken = 0x40000,
	Constant = 0x100,
	Export = 0x200,
	Forwarder = 0x400,
	FrameRelative = 0x20,
	Function = 0x800,
	ILRelative = 0x10000,
	Local = 0x80,
	Metadata = 0x20000,
	Parameter = 0x40,
	Register = 0x8,
	RegisterRelative = 0x10,
	Slot = 0x8000,
	Thunk = 0x2000,
	TLSRelative = 0x4000,
	ValuePresent = 1,
	Virtual = 0x1000,
	Null = 0x80000,
	FunctionNoReturn = 0x100000,
	SyntheticZeroBase = 0x200000,
	PublicCode = 0x400000
};

class SymbolInfo {
public:
	SymbolInfo();
	~SymbolInfo();

	operator PSYMBOL_INFO() const {
		return m_Symbol;
	}

	SYMBOL_INFO* GetSymbolInfo() const {
		return m_Symbol;
	}

	IMAGEHLP_MODULE ModuleInfo;

private:
	SYMBOL_INFO* m_Symbol;
};


class ImagehlpSymbol {
public:
	ImagehlpSymbol();
	~ImagehlpSymbol();

	IMAGEHLP_SYMBOL* GetSymbolInfo() const {
		return m_Symbol;
	}
private:
	IMAGEHLP_SYMBOL* m_Symbol;
};

class SymbolHandler final{
public:
	SymbolHandler(HANDLE hProcess = ::GetCurrentProcess(), PCSTR searchPath = nullptr,DWORD symOptions = 
		SYMOPT_UNDNAME | SYMOPT_CASE_INSENSITIVE | 
		SYMOPT_AUTO_PUBLICS | SYMOPT_INCLUDE_32BIT_MODULES | 
		SYMOPT_OMAP_FIND_NEAREST | SYMOPT_DEFERRED_LOADS);
	~SymbolHandler();

	HANDLE GetHandle() const;
	ULONG64 LoadSymbolsForModule(PCSTR moduleName, DWORD64 baseAddress = 0, DWORD dllSize = 0);
	
	ULONG_PTR GetSymbolAddressFromName(PCSTR name);

	std::unique_ptr<SymbolInfo> GetSymbolFromName(PCSTR name);
	std::unique_ptr<SymbolInfo> GetSymbolFromAddress(DWORD64 address, PDWORD64 offset = nullptr);
	
	static std::unique_ptr<SymbolHandler> CreateForProcess(DWORD pid, PCSTR searchPath = nullptr);
	/*void EnumSymbols(std::string mask = "*!*");
	void EnumTypes(std::string mask = "*");*/

	IMAGEHLP_MODULE GetModuleInfo(DWORD64 address);

	DWORD GetStructMemberOffset(std::string name,std::string memberName);
	DWORD GetStructMemberSize(std::string name, std::string memberName);
	DWORD64 LoadKernelModule(DWORD64 address);

	ULONG GetStructSize(std::string name);

	DWORD GetBitFieldPos(std::string name, std::string fieldName);

private:
	BOOL Callback(ULONG code, ULONG64 data);

	HANDLE m_hProcess{ INVALID_HANDLE_VALUE };
	DWORD64 _address{ 0 };
	static inline bool _first = false;
};