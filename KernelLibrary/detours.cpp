#include "pch.h"
#include <ntimage.h>
#include "detours.h"
#include "Logging.h"
#include "Memory.h"
#include "disasm.h"
#include "reflector.h"


extern "C" {
	NTSYSAPI NTSTATUS NTAPI ZwFlushInstructionCache(_In_ HANDLE 	ProcessHandle,
		_In_ PVOID 	BaseAddress,
		_In_ ULONG 	NumberOfBytesToFlush
	);
}

static bool s_fIgnoreTooSmall = false;
static bool s_fRetainRegions = false;

static DetourOperation* s_pPendingOperations = nullptr;

bool DetourIsImported(PUCHAR pCode, PUCHAR pAddress) {
	MEMORY_BASIC_INFORMATION mbi;
	SIZE_T len;
	NTSTATUS status = ZwQueryVirtualMemory(ZwCurrentProcess(),
		pCode,
		MemoryBasicInformation,
		&mbi,
		sizeof(mbi),
		&len);
	if (!NT_SUCCESS(status)) {
		return false;
	}
	__try {
		PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)mbi.AllocationBase;
		if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
			return false;
		}

		PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PUCHAR)pDosHeader +
			pDosHeader->e_lfanew);
		if (pNtHeader->Signature != IMAGE_NT_SIGNATURE) {
			return false;
		}
		if (pAddress >= ((PUCHAR)pDosHeader +
			pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress)
			&& pAddress < ((PUCHAR)pDosHeader
				+ pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress
				+ pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size)) {
			return true;
		}
	}
	__except (GetExceptionCode() == STATUS_ACCESS_VIOLATION
		? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
		return false;
	}
	return false;
}

ULONG_PTR Detour2gbBelow(ULONG_PTR address) {
	return (address > (ULONG_PTR)0x7ff80000) ? address - 0x7ff80000 : 0x80000;
}

ULONG_PTR Detour2gbAbove(ULONG_PTR address) {
#if defined(DETOURS_64BIT)
	return (address < (ULONG_PTR)0xffffffff80000000) ? address + 0x7ff80000 : (ULONG_PTR)0xfffffffffff80000;
#else
	return (address < (ULONG_PTR)0x80000000) ? address + 0x7ff80000 : (ULONG_PTR)0xfff80000;
#endif
}

void MapLockedCopyMemory(PUCHAR pDst, PVOID pSrc, ULONG length) {
	UCHAR code[4] = { 0 };

	PHYSICAL_ADDRESS physicalAddress = { 0 };
	PVOID virtualAddress = nullptr;

	if (length > sizeof(ULONG)) {
		RtlCopyMemory(code, pSrc, sizeof(ULONG));
	}
	else {
		RtlCopyMemory(code, pSrc, length);
		RtlCopyMemory(code + length, pDst + length, sizeof(ULONG) - length);
	}

	physicalAddress = MmGetPhysicalAddress(pDst);
	virtualAddress = MmMapIoSpace(physicalAddress, length, MmNonCached);
	if (virtualAddress != nullptr) {
		if (length > sizeof(ULONG)) {
			InterlockedExchange(reinterpret_cast<LONG*>(virtualAddress), 0xfeebUI32);
			RtlCopyMemory(reinterpret_cast<PCHAR>(virtualAddress) + sizeof(ULONG),
				reinterpret_cast<PCHAR>(pSrc) + sizeof(ULONG),
				length - sizeof(ULONG));
		}
		InterlockedExchange(reinterpret_cast<LONG*>(virtualAddress), *reinterpret_cast<PLONG>(code));
		MmUnmapIoSpace(virtualAddress, length);
	}
}

#pragma pack(push,1)
struct JmpImmediate {
	unsigned char jmp = 0xe9;
	INT32 imm32 = 0;
};

struct JmpIndirect {
	unsigned short int jmp = 0x25FF;
	INT32 imm32 = 0;
};

struct Brk {
	unsigned char int3[1];
};

struct JmpCode {
	unsigned char push = 0x68;
	ULONG lowAddr;				// push xxxxxxxx
	unsigned char op1 = 0xc7;
	unsigned char op2 = 0x44;
	unsigned char op3 = 0x24;
	unsigned char op4 = 0x04;
	ULONG highAddr;
	unsigned char ret = 0xc3;
};

#pragma pack(pop)

///////////////////////////////////////////////////////////////////////// X86.
//
#ifdef DETOURS_X86

struct _DETOUR_TRAMPOLINE
{
	UCHAR	rbCode[30];				// target code + jmp to pbRemain
	UCHAR			cbCode;         // size of moved target code.
	UCHAR			cbCodeBreak;    // padding to make debugging easier.
	UCHAR            rbRestore[22]; // original target code.
	UCHAR            cbRestore;     // size of original target code.
	UCHAR            cbRestoreBreak;// padding to make debugging easier.
	_DETOUR_ALIGN   rAlign[8];      // instruction alignment array.
	PUCHAR           pRemain;        // first instruction after moved code. [free list]
	PUCHAR           pDetour;        // first instruction of detour function.
};

C_ASSERT(sizeof(_DETOUR_TRAMPOLINE) == 72);

enum {
	SIZE_OF_JUMP_CODE = 5
};

PUCHAR DetourGenJmpImmediate(PUCHAR pCode, PUCHAR pJmpVal) {
	PUCHAR pJmpSrc = pCode + 5;
	JmpImmediate code;
	code.imm32 = (INT32)(pJmpVal - pJmpSrc);
	MapLockedCopyMemory(pCode, &code, sizeof(code));
	pCode++; // jmp +imm32
	((INT32*&)pCode)++;
	return pCode;
}

PUCHAR DetourGenJmpIndirect(PUCHAR pCode, PUCHAR* ppJmpVal) {
	PUCHAR pJmpSrc = pCode + 6;
	JmpIndirect code;
	code.imm32 = (INT32)((PUCHAR)ppJmpVal - pJmpSrc);
	MapLockedCopyMemory(pCode, &code, sizeof(code));
	pCode++; // jmp [+imm32]
	pCode++;
	((INT32*&)pCode)++;
	return pCode;
}

PUCHAR DetourGenBrk(PUCHAR pCode, PUCHAR pLimit) {
	ULONG count = pLimit - pCode;
	Brk* pBrk = (Brk*)ExAllocatePoolWithTag(NonPagedPool, sizeof(Brk) + count, 'oted');
	if (pBrk != nullptr) {
		MapLockedCopyMemory(pCode, pBrk->int3, count);
		ExFreePool(pBrk);
		while (pCode < pLimit) {
			pCode++;
		}
	}
	return pCode;
}

PUCHAR DetourSkipJmp(PUCHAR pCode, PVOID* ppGlobals) {
	if (pCode == nullptr) {
		return nullptr;
	}
	if (ppGlobals != nullptr) {
		*ppGlobals = nullptr;
	}

	// First, skip over the import vector if there is one
	if (pCode[0] == 0xff && pCode[1] == 0x25) { // jmp [+imm32]
		// Looks like an import alias jump, then get the code it points to.
		PUCHAR pTarget = *(UNALIGNED PUCHAR*) & pCode[2];
		if (DetourIsImported(pCode, pTarget)) {
			PUCHAR pNew = *(UNALIGNED PUCHAR*)pTarget;
			LogDebug("%p->%p: skipped over import table.", pCode, pNew);
			pCode = pNew;
		}
	}

	// Then, skip over a patch jump
	if (pCode[0] == 0xeb) { // jmp +imm8
		PUCHAR pNew = pCode + 2 + *(CHAR*)&pCode[1];
		LogDebug("%p->%p: skipped over short jump.\n", pCode, pNew);
		pCode = pNew;

		// First, skip over the import vector if there is one.
		if (pCode[0] == 0xff && pCode[1] == 0x25) { // jmp [+imm32]
			 // Looks like an import alias jump, then get the code it points to.
			PUCHAR pTarget = *(UNALIGNED PUCHAR*) & pCode[2];
			if (DetourIsImported(pCode, pTarget)) {
				pNew = *(UNALIGNED PUCHAR*)pTarget;
				LogDebug("%p->%p: skipped over import table.\n", pCode, pNew);
				pCode = pNew;
			}
		}
		// Finally, skip over a long jump if it is the target of the patch jump.
		else if (pCode[0] == 0xe9) {
			pNew = pCode + 5 + *(UNALIGNED INT32*) & pCode[1];
			LogDebug("%p->%p: skipped over long jump.\n", pCode, pNew);
			pCode = pNew;
		}
	}
	return pCode;
}

void DetourFindJmpBounds(PUCHAR pCode, PDETOUR_TRAMPOLINE* ppLower,
	PDETOUR_TRAMPOLINE* ppUpper) {
	// We have to place trampolines within +/- 2GB of code
	ULONG_PTR lo = Detour2gbBelow((ULONG_PTR)pCode);
	ULONG_PTR hi = Detour2gbAbove((ULONG_PTR)pCode);
	LogDebug("[%p..%p..%p]\n", (PVOID)lo, pCode, (PVOID)hi);

	// And, within +/- 2GB of relative jmp vectors.
	if (pCode[0] == 0xe9) {   // jmp +imm32
		PUCHAR pNew = pCode + 5 + *(UNALIGNED INT32*) & pCode[1];

		if (pNew < pCode) {
			hi = Detour2gbAbove((ULONG_PTR)pNew);
		}
		else {
			lo = Detour2gbBelow((ULONG_PTR)pNew);
		}
		LogDebug("[%p..%p..%p] +imm32\n", (PVOID)lo, pCode, (PVOID)hi);
	}

	*ppLower = (PDETOUR_TRAMPOLINE)lo;
	*ppUpper = (PDETOUR_TRAMPOLINE)hi;
}

bool DetourDoesCodeEndFunction(PUCHAR pCode) {
	if (pCode[0] == 0xeb ||    // jmp +imm8
		pCode[0] == 0xe9 ||    // jmp +imm32
		pCode[0] == 0xe0 ||    // jmp eax
		pCode[0] == 0xc2 ||    // ret +imm8
		pCode[0] == 0xc3 ||    // ret
		pCode[0] == 0xcc) {    // brk
		return TRUE;
	}
	else if (pCode[0] == 0xf3 && pCode[1] == 0xc3) {  // rep ret
		return TRUE;
	}
	else if (pCode[0] == 0xff && pCode[1] == 0x25) {  // jmp [+imm32]
		return TRUE;
	}
	else if ((pCode[0] == 0x26 ||      // jmp es:
		pCode[0] == 0x2e ||      // jmp cs:
		pCode[0] == 0x36 ||      // jmp ss:
		pCode[0] == 0x3e ||      // jmp ds:
		pCode[0] == 0x64 ||      // jmp fs:
		pCode[0] == 0x65) &&     // jmp gs:
		pCode[1] == 0xff &&       // jmp [+imm32]
		pCode[2] == 0x25) {
		return TRUE;
	}
	return FALSE;
}

ULONG DetourIsCodeFiller(PUCHAR pCode) {
	// 1-byte through 11-byte NOPs.
	if (pCode[0] == 0x90) {
		return 1;
	}
	if (pCode[0] == 0x66 && pCode[1] == 0x90) {
		return 2;
	}
	if (pCode[0] == 0x0F && pCode[1] == 0x1F && pCode[2] == 0x00) {
		return 3;
	}
	if (pCode[0] == 0x0F && pCode[1] == 0x1F && pCode[2] == 0x40 &&
		pCode[3] == 0x00) {
		return 4;
	}
	if (pCode[0] == 0x0F && pCode[1] == 0x1F && pCode[2] == 0x44 &&
		pCode[3] == 0x00 && pCode[4] == 0x00) {
		return 5;
	}
	if (pCode[0] == 0x66 && pCode[1] == 0x0F && pCode[2] == 0x1F &&
		pCode[3] == 0x44 && pCode[4] == 0x00 && pCode[5] == 0x00) {
		return 6;
	}
	if (pCode[0] == 0x0F && pCode[1] == 0x1F && pCode[2] == 0x80 &&
		pCode[3] == 0x00 && pCode[4] == 0x00 && pCode[5] == 0x00 &&
		pCode[6] == 0x00) {
		return 7;
	}
	if (pCode[0] == 0x0F && pCode[1] == 0x1F && pCode[2] == 0x84 &&
		pCode[3] == 0x00 && pCode[4] == 0x00 && pCode[5] == 0x00 &&
		pCode[6] == 0x00 && pCode[7] == 0x00) {
		return 8;
	}
	if (pCode[0] == 0x66 && pCode[1] == 0x0F && pCode[2] == 0x1F &&
		pCode[3] == 0x84 && pCode[4] == 0x00 && pCode[5] == 0x00 &&
		pCode[6] == 0x00 && pCode[7] == 0x00 && pCode[8] == 0x00) {
		return 9;
	}
	if (pCode[0] == 0x66 && pCode[1] == 0x66 && pCode[2] == 0x0F &&
		pCode[3] == 0x1F && pCode[4] == 0x84 && pCode[5] == 0x00 &&
		pCode[6] == 0x00 && pCode[7] == 0x00 && pCode[8] == 0x00 &&
		pCode[9] == 0x00) {
		return 10;
	}
	if (pCode[0] == 0x66 && pCode[1] == 0x66 && pCode[2] == 0x66 &&
		pCode[3] == 0x0F && pCode[4] == 0x1F && pCode[5] == 0x84 &&
		pCode[6] == 0x00 && pCode[7] == 0x00 && pCode[8] == 0x00 &&
		pCode[9] == 0x00 && pCode[10] == 0x00) {
		return 11;
	}

	// int 3.
	if (pCode[0] == 0xcc) {
		return 1;
	}
	return 0;
}

#endif // DETOURS_X86

///////////////////////////////////////////////////////////////////////// X64.
//
#ifdef DETOURS_X64

struct _DETOUR_TRAMPOLINE {
	UCHAR rbCode[0x60];		// target code + jmp to pRemain
	UCHAR cbCode;			// size of moved target code
	UCHAR cbCodeBreak;		// padding to make debugging easier
	UCHAR rbRestore[0x30];	// original target code.
	UCHAR cbRestore;		// size of original code.
	UCHAR cbRestoreBreak;	// padding to make debugging easier
	_DETOUR_ALIGN rAlign[8];// instruction alignment array.
	PUCHAR pRemain;			// first instruction after moved code. [free list]
	PUCHAR pDetour;			// first instruction of detour function.
	UCHAR rbCodeIn[8];		// jmp [pDetour]
};

enum {
	SIZE_OF_JUMP_CODE = 14,
};



PUCHAR DetourGenJmpAddress(PUCHAR pCode, PUCHAR pJmpVal) {
	JmpCode code;
	ULONG_PTR addr = (ULONG_PTR)pJmpVal;
	code.lowAddr = addr & 0xFFFFFFFF;
	code.highAddr = (addr >> 32) & 0xFFFFFFFF;
	MapLockedCopyMemory(pCode, &code, sizeof(code));
	pCode += sizeof(code);
	return pCode;
}

PUCHAR DetourGenJmpIndirect(PUCHAR pCode, PUCHAR* ppJmpVal) {
	PUCHAR pJmpSrc = pCode + 6;
	JmpIndirect code;
	code.imm32 = (INT32)((PUCHAR)ppJmpVal - pJmpSrc);
	MapLockedCopyMemory(pCode, &code, sizeof(code));
	pCode++;
	pCode++;
	((INT32*&)pCode)++;
	return pCode;
}

PUCHAR DetourGenBrk(PUCHAR pCode, PUCHAR pLimit) {
	ULONG count = pLimit - pCode;
	Brk* pBrk = (Brk*)ExAllocatePoolWithTag(NonPagedPool, sizeof(Brk) + count,'oted');
	if (pBrk != nullptr) {
		MapLockedCopyMemory(pCode, pBrk->int3, count);
		ExFreePool(pBrk);
		while (pCode < pLimit) {
			pCode++;
		}
	}
	return pCode;
}

PUCHAR DetourSkipJmp(PUCHAR pCode, PVOID* ppGlobals) {
	if (pCode == nullptr) {
		return nullptr;
	}
	if (ppGlobals != nullptr) {
		*ppGlobals = nullptr;
	}

	// First, skip over the import vector if there is one
	if (pCode[0] == 0xff && pCode[1] == 0x25) { // jmp [+imm32]
		// Looks like an import alias jump, then get the code it points to.
		PUCHAR pTarget = pCode + 6 + *(UNALIGNED INT32*) & pCode[2];
		if (DetourIsImported(pCode, pTarget)) {
			PUCHAR pNew = *(UNALIGNED PUCHAR*)pTarget;
			LogDebug("%p->%p: skipped over import table.", pCode, pNew);
			pCode = pNew;
		}
	}

	// Then, skip over a patch jump
	if (pCode[0] == 0xeb) { // jmp +imm8
		PUCHAR pNew = pCode + 2 + *(CHAR*)&pCode[1];
		LogDebug("%p->%p: skipped over short jump.\n", pCode, pNew);
		pCode = pNew;

		// First, skip over the import vector if there is one.
		if (pCode[0] == 0xff && pCode[1] == 0x25) { // jmp [+imm32]
			 // Looks like an import alias jump, then get the code it points to.
			PUCHAR pTarget = pCode + 6 + *(UNALIGNED INT32*) & pCode[2];
			if (DetourIsImported(pCode, pTarget)) {
				pNew = *(UNALIGNED PUCHAR*)pTarget;
				LogDebug("%p->%p: skipped over import table.\n", pCode, pNew);
				pCode = pNew;
			}
		}
		// Finally, skip over a long jump if it is the target of the patch jump.
		else if (pCode[0] == 0xe9) {
			pNew = pCode + 5 + *(UNALIGNED INT32*) & pCode[1];
			LogDebug("%p->%p: skipped over long jump.\n", pCode, pNew);
			pCode = pNew;
		}
	}
	return pCode;
}

bool DetourDoesCodeEndFunction(PUCHAR pCode) {
	if (pCode[0] == 0xeb ||    // jmp +imm8
		pCode[0] == 0xe9 ||    // jmp +imm32
		pCode[0] == 0xe0 ||    // jmp eax
		pCode[0] == 0xc2 ||    // ret +imm8
		pCode[0] == 0xc3 ||    // ret
		pCode[0] == 0xcc) {    // brk
		return TRUE;
	}
	else if (pCode[0] == 0xf3 && pCode[1] == 0xc3) {  // rep ret
		return TRUE;
	}
	else if (pCode[0] == 0xff && pCode[1] == 0x25) {  // jmp [+imm32]
		return TRUE;
	}
	else if ((pCode[0] == 0x26 ||      // jmp es:
		pCode[0] == 0x2e ||      // jmp cs:
		pCode[0] == 0x36 ||      // jmp ss:
		pCode[0] == 0x3e ||      // jmp ds:
		pCode[0] == 0x64 ||      // jmp fs:
		pCode[0] == 0x65) &&     // jmp gs:
		pCode[1] == 0xff &&      // jmp [+imm32]
		pCode[2] == 0x25) {
		return TRUE;
	}
	return FALSE;
}

ULONG DetourIsCodeFiller(PUCHAR pCode) {
	// 1-byte through 11-byte NOPs.
	if (pCode[0] == 0x90) {
		return 1;
	}
	if (pCode[0] == 0x66 && pCode[1] == 0x90) {
		return 2;
	}
	if (pCode[0] == 0x0F && pCode[1] == 0x1F && pCode[2] == 0x00) {
		return 3;
	}
	if (pCode[0] == 0x0F && pCode[1] == 0x1F && pCode[2] == 0x40 &&
		pCode[3] == 0x00) {
		return 4;
	}
	if (pCode[0] == 0x0F && pCode[1] == 0x1F && pCode[2] == 0x44 &&
		pCode[3] == 0x00 && pCode[4] == 0x00) {
		return 5;
	}
	if (pCode[0] == 0x66 && pCode[1] == 0x0F && pCode[2] == 0x1F &&
		pCode[3] == 0x44 && pCode[4] == 0x00 && pCode[5] == 0x00) {
		return 6;
	}
	if (pCode[0] == 0x0F && pCode[1] == 0x1F && pCode[2] == 0x80 &&
		pCode[3] == 0x00 && pCode[4] == 0x00 && pCode[5] == 0x00 &&
		pCode[6] == 0x00) {
		return 7;
	}
	if (pCode[0] == 0x0F && pCode[1] == 0x1F && pCode[2] == 0x84 &&
		pCode[3] == 0x00 && pCode[4] == 0x00 && pCode[5] == 0x00 &&
		pCode[6] == 0x00 && pCode[7] == 0x00) {
		return 8;
	}
	if (pCode[0] == 0x66 && pCode[1] == 0x0F && pCode[2] == 0x1F &&
		pCode[3] == 0x84 && pCode[4] == 0x00 && pCode[5] == 0x00 &&
		pCode[6] == 0x00 && pCode[7] == 0x00 && pCode[8] == 0x00) {
		return 9;
	}
	if (pCode[0] == 0x66 && pCode[1] == 0x66 && pCode[2] == 0x0F &&
		pCode[3] == 0x1F && pCode[4] == 0x84 && pCode[5] == 0x00 &&
		pCode[6] == 0x00 && pCode[7] == 0x00 && pCode[8] == 0x00 &&
		pCode[9] == 0x00) {
		return 10;
	}
	if (pCode[0] == 0x66 && pCode[1] == 0x66 && pCode[2] == 0x66 &&
		pCode[3] == 0x0F && pCode[4] == 0x1F && pCode[5] == 0x84 &&
		pCode[6] == 0x00 && pCode[7] == 0x00 && pCode[8] == 0x00 &&
		pCode[9] == 0x00 && pCode[10] == 0x00) {
		return 11;
	}

	// int 3.
	if (pCode[0] == 0xcc) {
		return 1;
	}
	return 0;
}

#endif // DETOURS_X64

//////////////////////////////////////////////// Trampoline Memory Management.
//
struct DETOUR_REGION {
	ULONG Signature;
	DETOUR_REGION* pNext;		// Next region in list of regions
	DETOUR_TRAMPOLINE* pFree;	// List of free trampolines in this region
};
typedef DETOUR_REGION* PDETOUR_REGION;

const ULONG DETOUR_REGION_SIGNATURE = 'Rrtd';
const ULONG DETOUR_REGION_SIZE = 0x10000;
const ULONG DETOUR_TRAMPOLINES_PER_REGION = (DETOUR_REGION_SIZE
	/ sizeof(DETOUR_TRAMPOLINE)) - 1;

static PDETOUR_REGION s_pRegions = nullptr;	// List of all regions.
static PDETOUR_REGION s_pRegion = nullptr;  // Default region




PZwProtectVirtualMemory g_pZwProtectVirtualMemory = nullptr;

static PVOID DetourAllocTrampolineAllocateNew() {
	PVOID pTry = nullptr;

	pTry = ExAllocatePoolWithTag(NonPagedPool, DETOUR_REGION_SIZE, 'oted');

	return pTry;
}

static PDETOUR_TRAMPOLINE DetourAllocTrampoline(PUCHAR pTarget) {
	PDETOUR_TRAMPOLINE pTrampoline = nullptr;

	// Insure that there is a default region.
	if (s_pRegion == nullptr && s_pRegions != nullptr) {
		s_pRegion = s_pRegions;
	}

	// First check the default region for an valid free block.
	if (s_pRegion != nullptr && s_pRegion->pFree != nullptr) {

	found_region:
		pTrampoline = s_pRegion->pFree;
		s_pRegion->pFree = (PDETOUR_TRAMPOLINE)pTrampoline->pRemain;
		memset(pTrampoline, 0xcc, sizeof(*pTrampoline));
		return pTrampoline;
	}

	// Then check the existing regions for a valid free block.
	for (s_pRegion = s_pRegions; s_pRegion != nullptr; s_pRegion = s_pRegion->pNext) {
		if (s_pRegion != nullptr && s_pRegion->pFree != nullptr) {
			goto found_region;
		}
	}

	// We need to allocate a new region.

	 // Round pTarget down to 64KB block.
	// /RTCc RuntimeChecks breaks PtrToUlong.
	pTarget = pTarget - (ULONG)((ULONG_PTR)pTarget & 0xffff);

	PVOID pNewlyAllocated = DetourAllocTrampolineAllocateNew();
	if (pNewlyAllocated != nullptr) {
		s_pRegion = (DETOUR_REGION*)pNewlyAllocated;
		s_pRegion->Signature = DETOUR_REGION_SIGNATURE;
		s_pRegion->pFree = NULL;
		s_pRegion->pNext = s_pRegions;
		s_pRegions = s_pRegion;
		LogDebug("  Allocated region %p..%p\n\n",
			s_pRegion, ((PUCHAR)s_pRegion) + DETOUR_REGION_SIZE - 1);

		// Put everything but the first trampoline on the free list.
		PUCHAR pFree = NULL;
		pTrampoline = ((PDETOUR_TRAMPOLINE)s_pRegion) + 1;
		for (int i = DETOUR_TRAMPOLINES_PER_REGION - 1; i > 1; i--) {
			pTrampoline[i].pRemain = pFree;
			pFree = (PUCHAR)&pTrampoline[i];
		}
		s_pRegion->pFree = (PDETOUR_TRAMPOLINE)pFree;
		goto found_region;
	}

	LogError("Couldn't find available memory region!\n");
	return nullptr;
}

static bool DetourIsRegionEmpty(PDETOUR_REGION pRegion) {
	// Stop if the region isn't a region (this would be bad).
	if (pRegion->Signature != DETOUR_REGION_SIGNATURE) {
		return FALSE;
	}

	PUCHAR pRegionBeg = (PUCHAR)pRegion;
	PUCHAR pRegionLim = pRegionBeg + DETOUR_REGION_SIZE;

	// Stop if any of the trampolines aren't free.
	PDETOUR_TRAMPOLINE pTrampoline = ((PDETOUR_TRAMPOLINE)pRegion) + 1;
	for (int i = 0; i < DETOUR_TRAMPOLINES_PER_REGION; i++) {
		if (pTrampoline[i].pRemain != NULL &&
			(pTrampoline[i].pRemain < pRegionBeg ||
				pTrampoline[i].pRemain >= pRegionLim)) {
			return FALSE;
		}
	}

	// OK, the region is empty.
	return TRUE;
}

static void DetourFreeUnusedTrampolineRegions() {
	PDETOUR_REGION* ppRegionBase = &s_pRegions;
	PDETOUR_REGION pRegion = s_pRegions;

	while (pRegion != NULL) {
		if (DetourIsRegionEmpty(pRegion)) {
			*ppRegionBase = pRegion->pNext;
			ExFreePoolWithTag(pRegion, 'oted');
			s_pRegion = NULL;
		}
		else {
			ppRegionBase = &pRegion->pNext;
		}
		pRegion = *ppRegionBase;
	}
}

PVOID NTAPI DetourCodeFromPointer(_In_ PVOID pPointer,
	_Out_opt_ PVOID* ppGlobals) {
	return DetourSkipJmp((PUCHAR)pPointer, ppGlobals);
}

NTSTATUS NTAPI DetourTransactionCommit() {
	return DetourTransactionCommitEx();
}

NTSTATUS NTAPI DetourTransactionCommitEx() {
	// Common variables
	DetourOperation* o = nullptr;
	bool freed = false;

	// Insert or remove each of the detours
	for (o = s_pPendingOperations; o != nullptr; o = o->pNext) {
		if (o->fIsRemove) {
			MapLockedCopyMemory(o->pTarget, o->pTrampoline->rbRestore,
				o->pTrampoline->cbRestore);
#ifdef DETOURS_IA64
			* o->ppbPointer = (PBYTE)o->pTrampoline->ppldTarget;
#endif // DETOURS_IA64

#ifdef DETOURS_X86
			* o->ppPointer = o->pTarget;
#endif // DETOURS_X86

#ifdef DETOURS_X64
			* o->ppPointer = o->pTarget;
#endif // DETOURS_X64

#ifdef DETOURS_ARM
			* o->ppbPointer = DETOURS_PBYTE_TO_PFUNC(o->pbTarget);
#endif // DETOURS_ARM

#ifdef DETOURS_ARM64
			* o->ppbPointer = o->pbTarget;
#endif // DETOURS_ARM

		}
		else {
			LogDebug("detours: pbTramp =%p, pbRemain=%p, pbDetour=%p, cbRestore=%u\n",
				o->pTrampoline,
				o->pTrampoline->pRemain,
				o->pTrampoline->pDetour,
				o->pTrampoline->cbRestore);

			LogDebug("detours: pbTarget=%p: "
				"%02x %02x %02x %02x "
				"%02x %02x %02x %02x "
				"%02x %02x %02x %02x [before]\n",
				o->pTarget,
				o->pTarget[0], o->pTarget[1], o->pTarget[2], o->pTarget[3],
				o->pTarget[4], o->pTarget[5], o->pTarget[6], o->pTarget[7],
				o->pTarget[8], o->pTarget[9], o->pTarget[10], o->pTarget[11]);

#ifdef DETOURS_IA64
			((DETOUR_IA64_BUNDLE*)o->pbTarget)
				->SetBrl((UINT64)&o->pTrampoline->bAllocFrame);
			*o->ppbPointer = (PBYTE)&o->pTrampoline->pldTrampoline;
#endif // DETOURS_IA64

#ifdef DETOURS_X64
			DetourGenJmpIndirect(o->pTrampoline->rbCodeIn, &o->pTrampoline->pDetour);
			PUCHAR pCode = DetourGenJmpAddress(o->pTarget, o->pTrampoline->rbCodeIn);
			pCode = DetourGenBrk(pCode, o->pTrampoline->pRemain);
			*o->ppPointer = o->pTrampoline->rbCode;
			UNREFERENCED_PARAMETER(pCode);
#endif // DETOURS_X64

#ifdef DETOURS_X86
			PUCHAR pCode = DetourGenJmpImmediate(o->pTarget, o->pTrampoline->pDetour);
			pCode = DetourGenBrk(pCode, o->pTrampoline->pRemain);
			*o->ppPointer = o->pTrampoline->rbCode;
			UNREFERENCED_PARAMETER(pCode);
#endif // DETOURS_X86

#ifdef DETOURS_ARM
			
#endif // DETOURS_ARM

#ifdef DETOURS_ARM64

#endif // DETOURS_ARM64

			LogDebug("detours: pbTarget=%p: "
				"%02x %02x %02x %02x "
				"%02x %02x %02x %02x "
				"%02x %02x %02x %02x [after]\n",
				o->pTarget,
				o->pTarget[0], o->pTarget[1], o->pTarget[2], o->pTarget[3],
				o->pTarget[4], o->pTarget[5], o->pTarget[6], o->pTarget[7],
				o->pTarget[8], o->pTarget[9], o->pTarget[10], o->pTarget[11]);

			LogDebug("detours: pbTramp =%p: "
				"%02x %02x %02x %02x "
				"%02x %02x %02x %02x "
				"%02x %02x %02x %02x "
				"%02x %02x %02x %02x "
				"%02x %02x %02x %02x "
				"%02x %02x %02x %02x "
				"%02x %02x %02x %02x "
				"%02x %02x %02x %02x "
				"%02x %02x %02x %02x "
				"%02x %02x %02x %02x "
				"%02x %02x %02x %02x "
				"%02x %02x %02x %02x "
				"%02x %02x %02x %02x "
				"%02x %02x %02x %02x\n",
				o->pTrampoline,
				o->pTrampoline->rbCode[0], o->pTrampoline->rbCode[1],
				o->pTrampoline->rbCode[2], o->pTrampoline->rbCode[3],
				o->pTrampoline->rbCode[4], o->pTrampoline->rbCode[5],
				o->pTrampoline->rbCode[6], o->pTrampoline->rbCode[7],
				o->pTrampoline->rbCode[8], o->pTrampoline->rbCode[9],
				o->pTrampoline->rbCode[10],o->pTrampoline->rbCode[11],
				o->pTrampoline->rbCode[12],o->pTrampoline->rbCode[13],
				o->pTrampoline->rbCode[14], o->pTrampoline->rbCode[15],
				o->pTrampoline->rbCode[16], o->pTrampoline->rbCode[17], 
				o->pTrampoline->rbCode[18], o->pTrampoline->rbCode[19], 
				o->pTrampoline->rbCode[20], o->pTrampoline->rbCode[21], 
				o->pTrampoline->rbCode[22], o->pTrampoline->rbCode[23], 
				o->pTrampoline->rbCode[24], o->pTrampoline->rbCode[25], 
				o->pTrampoline->rbCode[26], o->pTrampoline->rbCode[27]);

#ifdef DETOURS_IA64

#endif // DETOURS_IA64
		}
	}

	// Restore all of the page permissions and flush the icache.
	for (DetourOperation* pOper = s_pPendingOperations; pOper != nullptr;) {
		// We don't care if this fails, because the code is still accessible
		if (pOper->fIsRemove && pOper->pTrampoline) {
			pOper->pTrampoline = nullptr;
			freed = true;
		}

		DetourOperation* n = pOper->pNext;
		delete pOper;
		pOper = n;
	}
	s_pPendingOperations = nullptr;

	// Free any trampoline regions that are now unused.
	if (freed && !s_fRetainRegions) {
		DetourFreeUnusedTrampolineRegions();
	}

	return STATUS_SUCCESS;
}



NTSTATUS NTAPI DetourAttach(_Inout_ PVOID* ppPointer,
	_In_ PVOID pDetour) {
	return DetourAttachEx(ppPointer, pDetour, nullptr, nullptr, nullptr);
}

NTSTATUS NTAPI DetourAttachEx(_Inout_ PVOID* ppPointer,
	_In_ PVOID pDetour,
	_Out_opt_ PDETOUR_TRAMPOLINE* ppRealTrampoline,
	_Out_opt_ PVOID* ppRealTarget,
	_Out_opt_ PVOID* ppRealDetour) {
	NTSTATUS status = STATUS_SUCCESS;

	if (ppRealTrampoline != nullptr) {
		*ppRealTrampoline = nullptr;
	}
	if (ppRealTarget != nullptr) {
		*ppRealTarget = nullptr;
	}
	if (ppRealDetour != nullptr) {
		*ppRealDetour = nullptr;
	}
	if (pDetour == nullptr) {
		LogDebug("empty detour\n");
		return STATUS_INVALID_PARAMETER;
	}


	if (ppPointer == nullptr) {
		LogDebug("ppPointer is null\n");
		return STATUS_INVALID_HANDLE;
	}

	if (*ppPointer == nullptr) {
		status = STATUS_INVALID_HANDLE;
		LogDebug("*ppPointer is null (ppPointer=%p)\n", ppPointer);
		DbgBreakPoint();
		return status;
	}

	PUCHAR pTarget = (PUCHAR)*ppPointer;
	PDETOUR_TRAMPOLINE pTrampoline = nullptr;
	DetourOperation* o = nullptr;

#ifdef DETOURS_IA64

#else // DETOURS_IA64
	pTarget = (PUCHAR)DetourCodeFromPointer(pTarget, NULL);
	pDetour = DetourCodeFromPointer(pDetour, NULL);
#endif // !DETOURS_IA64

	// Don't follow a jump if its destination is the target function.
	// This happens when the detour does nothing other than call the target.
	if (pDetour == (PVOID)pTarget) {
		if (s_fIgnoreTooSmall) {
			goto stop;
		}
		else {
			DbgBreakPoint();
			goto fail;
		}
	}

	if (ppRealTarget != nullptr) {
		*ppRealTarget = pTarget;
	}
	if (ppRealDetour != nullptr) {
		*ppRealDetour = pDetour;
	}

	o = new (NonPagedPool) DetourOperation;
	if (o == nullptr) {
		status = STATUS_NO_MEMORY;
	fail:
		DbgBreakPoint();
	stop:
		if (pTrampoline != nullptr) {
			pTrampoline = nullptr;
			if (ppRealTrampoline != nullptr) {
				*ppRealTrampoline = nullptr;
			}
		}
		if (o != nullptr) {
			delete o;
			o = nullptr;
		}
		if (ppRealDetour != nullptr) {
			*ppRealDetour = nullptr;
		}
		if (ppRealTarget != nullptr) {
			*ppRealTarget = nullptr;
		}
		return status;
	}

	pTrampoline = DetourAllocTrampoline(pTarget);
	if (pTrampoline == nullptr) {
		status = STATUS_NO_MEMORY;
		DbgBreakPoint();
		goto fail;
	}

	if (ppRealTrampoline != nullptr) {
		*ppRealTrampoline = pTrampoline;
	}

	LogDebug("detours: pTramp=%p, pDetour=%p\n", pTrampoline, pDetour);

	memset(pTrampoline->rAlign, 0, sizeof(pTrampoline->rAlign));

	// Detour the number of movable target instructions.
	PUCHAR pSrc = pTarget;
	PUCHAR prbCode = pTrampoline->rbCode;

#ifdef DETOURS_IA64

#else
	PUCHAR pPool = prbCode + sizeof(pTrampoline->rbCode);
#endif

	ULONG target = 0;
	ULONG jump = SIZE_OF_JUMP_CODE;
	ULONG align = 0;

#ifdef DETOURS_ARM

#endif

	while (target < jump) {
		PUCHAR pOp = pSrc;
		LONG extra = 0;

		LogDebug("DetourCopyInstruction(%p,%p)\n",
			prbCode, pSrc);
		pSrc = (PUCHAR)DetourCopyInstruction(prbCode, (PVOID*)&pPool, pSrc, nullptr, &extra);
		LogDebug("DetourCopyInstruction() = %p (%d bytes)\n", pSrc, (int)(pSrc - pOp));
		prbCode += (pSrc - pOp) + extra;
		target = (LONG)(pSrc - pTarget);
		pTrampoline->rAlign[align].obTarget = target;
		pTrampoline->rAlign[align].obTrampoline = prbCode - pTrampoline->rbCode;
		align++;

		if (align >= ARRAYSIZE(pTrampoline->rAlign)) {
			break;
		}
		if (DetourDoesCodeEndFunction(pOp)) {
			break;
		}
	}

	// Consume, but don't duplicate padding if it is needed and available
	while (target < jump) {
		LONG filler = DetourIsCodeFiller(pSrc);
		if (filler == 0)
			break;

		pSrc += filler;
		target = (LONG)(pSrc - pTarget);
	}

#if DETOUR_DEBUG
	{
		DETOUR_TRACE((" detours: rAlign ["));
		LONG n = 0;
		for (n = 0; n < ARRAYSIZE(pTrampoline->rAlign); n++) {
			if (pTrampoline->rAlign[n].obTarget == 0 &&
				pTrampoline->rAlign[n].obTrampoline == 0) {
				break;
			}
			DETOUR_TRACE((" %u/%u",
				pTrampoline->rAlign[n].obTarget,
				pTrampoline->rAlign[n].obTrampoline
				));

		}
		DETOUR_TRACE((" ]\n"));
	}
#endif

	if (target<jump || align>ARRAYSIZE(pTrampoline->rAlign)) {
		// Too few instruction

		status = STATUS_INVALID_BLOCK_LENGTH;
		if (s_fIgnoreTooSmall) {
			goto stop;
		}
		else {
			DbgBreakPoint();
			goto fail;
		}
	}

	if (prbCode > pPool) {
		DbgBreakPoint();
	}

	pTrampoline->cbCode = (UCHAR)(prbCode - pTrampoline->rbCode);
	pTrampoline->cbRestore = (UCHAR)target;
	RtlCopyMemory(pTrampoline->rbRestore, pTarget, target);

#if !defined(DETOURS_IA64)
	if (target > sizeof(pTrampoline->rbCode) - jump) {
		// Too many instructions.
		status = STATUS_INVALID_HANDLE;
		DbgBreakPoint();
		goto fail;
	}
#endif // !DETOURS_IA64

	pTrampoline->pRemain = pTarget + target;
	pTrampoline->pDetour = (PUCHAR)pDetour;

#ifdef DETOURS_IA64

#endif // DETOURS_IA64

	prbCode = pTrampoline->rbCode + pTrampoline->cbCode;
#ifdef DETOURS_X64
	// fixup the trampoline
	prbCode = ReflectCode(pTarget, pTrampoline->cbCode, pTrampoline->rbCode, sizeof(pTrampoline->rbCode));
	if (prbCode == pTrampoline->rbCode) {
		status = STATUS_UNSUCCESSFUL;
		goto fail;
	}
	prbCode = DetourGenBrk(prbCode, pPool);
#endif

#ifdef DETOURS_X86
	prbCode = DetourGenJmpImmediate(prbCode, pTrampoline->pRemain);
	prbCode = DetourGenBrk(prbCode, pPool);
#endif // DETOURS_X86

#ifdef DETOURS_ARM

#endif // DETOURS_ARM

#ifdef DETOURS_ARM64

#endif // DETOURS_ARM64


	LogDebug("detours: pbTarget=%p: "
		"%02x %02x %02x %02x "
		"%02x %02x %02x %02x "
		"%02x %02x %02x %02x\n",
		pTarget,
		pTarget[0], pTarget[1], pTarget[2], pTarget[3],
		pTarget[4], pTarget[5], pTarget[6], pTarget[7],
		pTarget[8], pTarget[9], pTarget[10], pTarget[11]);
	LogDebug("detours: pbTramp =%p: "
		"%02x %02x %02x %02x "
		"%02x %02x %02x %02x "
		"%02x %02x %02x %02x\n",
		pTrampoline,
		pTrampoline->rbCode[0], pTrampoline->rbCode[1],
		pTrampoline->rbCode[2], pTrampoline->rbCode[3],
		pTrampoline->rbCode[4], pTrampoline->rbCode[5],
		pTrampoline->rbCode[6], pTrampoline->rbCode[7],
		pTrampoline->rbCode[8], pTrampoline->rbCode[9],
		pTrampoline->rbCode[10], pTrampoline->rbCode[11]);

	o->fIsRemove = FALSE;
	o->ppPointer = (PUCHAR*)ppPointer;
	o->pTrampoline = pTrampoline;
	o->pTarget = pTarget;
	o->pNext = s_pPendingOperations;
	s_pPendingOperations = o;

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI DetourDetach(_Inout_ PVOID* ppPointer,
	_In_ PVOID pDetour) {
	NTSTATUS error = STATUS_SUCCESS;

	if (pDetour == nullptr) {
		return STATUS_INVALID_HANDLE;
	}
	if (ppPointer == NULL) {
		return STATUS_INVALID_HANDLE;
	}
	if (*ppPointer == NULL) {
		error = STATUS_INVALID_HANDLE;
		DbgBreakPoint();
		return error;
	}

	DetourOperation* o = new (NonPagedPool) DetourOperation;
	if (o == NULL) {
		error = STATUS_NO_MEMORY;
	fail:
		DbgBreakPoint();
	stop:
		if (o != NULL) {
			delete o;
			o = NULL;
		}
		return error;
	}

#ifdef DETOURS_IA64

#else // !DETOURS_IA64
	PDETOUR_TRAMPOLINE pTrampoline =
		(PDETOUR_TRAMPOLINE)DetourCodeFromPointer(*ppPointer, NULL);
	pDetour = DetourCodeFromPointer(pDetour, NULL);
#endif // !DETOURS_IA64

	////////////////////////////////////// Verify that Trampoline is in place.
   //
	LONG target = pTrampoline->cbRestore;
	PUCHAR pTarget = pTrampoline->pRemain - target;
	if (target == 0 || target > sizeof(pTrampoline->rbCode)) {
		error = STATUS_INVALID_BLOCK_LENGTH;
		if (s_fIgnoreTooSmall) {
			goto stop;
		}
		else {
			DbgBreakPoint();
			goto fail;
		}
	}

	if (pTrampoline->pDetour != pDetour) {
		error = STATUS_INVALID_BLOCK_LENGTH;
		if (s_fIgnoreTooSmall) {
			goto stop;
		}
		else {
			DbgBreakPoint();
			goto fail;
		}
	}

	o->fIsRemove = TRUE;
	o->ppPointer = (PUCHAR*)ppPointer;
	o->pTrampoline = pTrampoline;
	o->pTarget = pTarget;
	o->pNext = s_pPendingOperations;
	s_pPendingOperations = o;

	return STATUS_SUCCESS;
}