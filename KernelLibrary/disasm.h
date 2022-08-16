#pragma once

#define DETOUR_INSTRUCTION_TARGET_NONE		((PVOID)0)
#define DETOUR_INSTRUCTION_TARGET_DYNAMIC	((PVOID)(LONG_PTR)-1)

PVOID
NTAPI
DetourCopyInstruction(
	_In_opt_ PVOID Dst,
	_Inout_opt_ PVOID* pDstPool,
	_In_ PVOID Src,
	_Out_opt_ PVOID* pTarget,
	_Out_opt_ LONG* pExtra
);

struct Disasm{
	Disasm(_Out_opt_ PUCHAR* pTarget,
		_Out_opt_ LONG* Extra);
	PUCHAR CopyInstruction(PUCHAR Dst, PUCHAR Src);
	
	// nFlagBits flags
	enum class FlagBits{
		Dynamic = 0x1u,
		Address = 0x2u,
		NoEnlarge = 0x4u,
		Rax = 0x8u,
	};

	enum class ModRM {
		Sib = 0x10u,
		Rip = 0x20u,
		NotSib = 0x0fu,
	};

	struct CopyEntry;

	struct CopyEntry {
		ULONG FixedSize : 4;
		ULONG FixedSize16 : 4;
		ULONG ModOffset : 4;
		ULONG RelOffset : 4;
		ULONG FlagBits : 4;
	};

	PUCHAR CopyBytes(CopyEntry* pEntry, PUCHAR Dst, PUCHAR Src);


};