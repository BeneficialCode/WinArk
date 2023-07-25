#pragma once


#define DETOUR_INSTRUCTION_TARGET_NONE		((PVOID)0)
#define DETOUR_INSTRUCTION_TARGET_DYNAMIC	((PVOID)(LONG_PTR)-1)

PVOID
NTAPI
DetourCopyInstruction(
	_In_opt_ PVOID pDst,
	_Inout_opt_ PVOID* ppDstPool,
	_In_ PVOID pSrc,
	_Out_opt_ PVOID* ppTarget,
	_Out_opt_ LONG* pExtra
);

// nFlagBits flags
enum class FlagBits {
	None = 0x0,
	Dynamic = 0x1,
	Address = 0x2,
	NoEnlarge = 0x4,
	Rax = 0x8,
};
DEFINE_ENUM_FLAG_OPERATORS(FlagBits);

// ModR/M flags
enum class ModRm : UCHAR {
	Sib = 0x10,
	Rip = 0x20,
	NotSib = 0x0f,
};
DEFINE_ENUM_FLAG_OPERATORS(ModRm);

struct Disasm {
	Disasm(PUCHAR* ppTarget, LONG* pExtra);
	PUCHAR CopyInstruction(PUCHAR pDst, PUCHAR pSrc);

	struct CopyEntry;
	typedef const CopyEntry* RefCopyEntry;

	typedef PUCHAR(Disasm::* CopyFunc)(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);

	struct CopyEntry {
		ULONG FixedSize : 4;
		ULONG FixedSize16 : 4;
		ULONG ModOffset : 4;
		ULONG RelOffset : 4;
		FlagBits FlagBits : 4;
		CopyFunc pfCopy;
	};

	PUCHAR CopyBytes(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);
	PUCHAR CopyBytesPrefix(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);
	PUCHAR CopyBytesSegment(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);
	PUCHAR CopyBytesRax(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);
	PUCHAR CopyBytesJump(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);

	PUCHAR Invalid(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);

	PUCHAR AdjustTarget(PUCHAR pDst, PUCHAR pSrc, ULONG op, ULONG targetOffset,
		ULONG targetSize);

protected:
	PUCHAR Copy0F(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);
	// x86 only sldt/0 str/1 lldt/2 ltr/3 err/4 verw/5 jmpe/6/dynamic invalid/7
	PUCHAR Copy0F00(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);
	// vmread, 66/extrq/ib/ib, F2/insertq/ib/ib
	PUCHAR Copy0F78(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);
	// jmpe or F3/popcnt
	PUCHAR Copy0FB8(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);
	PUCHAR Copy66(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);
	PUCHAR Copy67(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);
	PUCHAR CopyF2(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);
	PUCHAR CopyF3(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);
	PUCHAR CopyF6(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);
	PUCHAR CopyF7(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);
	PUCHAR CopyFF(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);
	PUCHAR CopyVex2(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);
	PUCHAR CopyVex3(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);
	PUCHAR CopyVexCommon(UCHAR m, PUCHAR pDst, PUCHAR pSrc);
	PUCHAR CopyVexEvexCommon(UCHAR m, PUCHAR pDst, PUCHAR pSrc, UCHAR p, UCHAR fp16 = 0);
	PUCHAR CopyEvex(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);
	PUCHAR CopyXop(RefCopyEntry pEntry, PUCHAR pDst, PUCHAR pSrc);

protected:
	static const CopyEntry	s_CopyTable[];
	static const CopyEntry	s_CopyTable0F[];
	static const UCHAR		s_ModRm[256];
	static inline PUCHAR	s_pModuleBeg{ nullptr };
	static inline PUCHAR	s_pModuleEnd{ (PUCHAR)~(ULONG_PTR)0 };
	static inline bool		s_fLimitReferencesToModule{ false };

protected:
	bool m_OperandOverride{ false };
	bool m_AddressOverride{ false };
	bool m_RaxOverride{ false };		// AMD64 only
	bool m_Vex{ false };
	bool m_Evex{ false };
	bool m_F2{ false };
	bool m_F3{ false };				// x86 only
	UCHAR m_SegmentOverride;
	PUCHAR* m_ppTarget;
	LONG* m_pExtra;
	LONG m_ScratchExtra;
	PUCHAR m_pScratchTarget;
	UCHAR m_ScratchDst[64];	// matches or exceeds rbCode
};