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

struct SymbolInfo : SYMBOL_INFO {
	// member name cannot be larger than this (docs says 2000, but seems wasteful in practice)
	//const int MaxSymbolLen = 500;

};

class SymbolHandler final{
public:
	SymbolHandler(HANDLE hProcess, bool ownHandle);

	void LoadSymbolsForModule(std::string imageName,DWORD64 dllBase = 0,HANDLE hFile = nullptr);
	void EnumSymbols(std::string mask = "*!*");

	void EnumTypes(std::string mask = "*");
	ULONG64 GetSymbolAddressFromName(std::string name);
	DWORD GetStructMemberOffset(std::string name,std::string memberName);

private:
	HANDLE _hProcess;
	bool _ownHandle;
	DWORD64 _baseAddress = 0;
};