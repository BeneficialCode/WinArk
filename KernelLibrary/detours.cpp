#include "pch.h"
#include <ntimage.h>
#include "detours.h"
#include "Logging.h"
#include "Memory.h"


static bool s_fIgnoreTooSmall = false;

static LONG s_PendingThreadId = 0;
static NTSTATUS s_PendingError = STATUS_SUCCESS;
static PVOID* s_ppPendingError = nullptr;
static DetourThread* s_pPendingThreads = nullptr;
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
	SIZE_OF_JMP = 5
};

PUCHAR DetourGenJmpImmedate(PUCHAR pCode, PUCHAR pJmpVal) {
	PUCHAR pJmpSrc = pCode + 5;
	*pCode++ = 0xE9; // jmp +imm32
	*((INT32*&)pCode)++ = (INT32)(pJmpVal - pJmpSrc);
	return pCode;
}

PUCHAR DetourGenJmpIndirect(PUCHAR pCode, PUCHAR* ppJmpVal) {
	PUCHAR pJmpSrc = pCode + 6;
	*pCode++ = 0xff; // jmp [+imm32]
	*pCode++ = 0x25;
	*((INT32*&)pCode)++ = (INT32)((PUCHAR)ppJmpVal - pJmpSrc);
	return pCode;
}

PUCHAR DetourGenBrk(PUCHAR pCode, PUCHAR pLimie) {
	while (pCode < pLimie) {
		*pCode++ = 0xcc;
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
		PUCHAR pTarget = pCode + 6 + *(UNALIGNED PUCHAR*) & pCode[2];
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
			PUCHAR pTarget = pCode + 6 + *(UNALIGNED PUCHAR*) & pCode[2];
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
	// An X64 instuction can be 15 bytes long.
	// In practice 11 seems to be the limit.
	UCHAR rbCode[30];		// target code + jmp to pRemain
	UCHAR cbCode;			// size of moved target code
	UCHAR cbCodeBreak;		// padding to make debugging easier
	UCHAR cbRestore[30];	// original target code.
	UCHAR cbRestoreBreak;	// padding to make debugging easier
	_DETOUR_ALIGN rAlign[8];// instruction alignment array.
	PUCHAR pRemain;			// first instruction after moved code. [free list]
	PUCHAR pDetour;			// first instruction of detour function.
	UCHAR rbCodeIn[8];		// jmp [pDetour]
};

C_ASSERT(sizeof(_DETOUR_TRAMPOLINE) == 96);

enum {
	SIZE_OF_JMP = 5
};

PUCHAR DetourGenJmpImmediate(PUCHAR pCode, PUCHAR pJmpVal) {
	PUCHAR pJmpSrc = pCode + 5;
	*pCode++ = 0xe9; // jmp + imm32
	*((INT32*&)pCode)++ = (INT32)(pJmpVal - pJmpSrc);
	return pCode;
}

PUCHAR DetourGenJmpIndirect(PUCHAR pCode, PUCHAR* ppJmpVal) {
	PUCHAR pJmpSrc = pCode + 6;
	*pCode++ = 0xff; // jmp [+imm32]
	*pCode++ = 0x25;
	*((INT32*&)pCode)++ = (INT32)((PUCHAR)ppJmpVal - pJmpSrc);
	return pCode;
}

PUCHAR DetourGenBrk(PUCHAR pCode, PUCHAR pLimie) {
	while (pCode < pLimie) {
		*pCode++ = 0xcc;
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

void DetourFindJmpBounds(PUCHAR pCode, PDETOUR_TRAMPOLINE* ppLower,
	PDETOUR_TRAMPOLINE* ppUpper) {
	// We have to place trampolines within +/- 2GB of code
	ULONG_PTR lo = Detour2gbBelow((ULONG_PTR)pCode);
	ULONG_PTR hi = Detour2gbAbove((ULONG_PTR)pCode);
	LogDebug("[%p..%p..%p]\n", (PVOID)lo, pCode, (PVOID)hi);

	// And, within +/- 2GB of relative jmp vectors.
	if (pCode[0] == 0xff && pCode[1] == 0x25) {
		PUCHAR pNew = pCode + 6 + *(UNALIGNED INT32*) & pCode[2];
		if (pNew < pCode) {
			hi = Detour2gbAbove((ULONG_PTR)pNew);
		}
		else {
			lo = Detour2gbBelow((ULONG_PTR)pNew);
		}
		LogDebug("[%p..%p..%p] [+imm32]\n", (PVOID)lo, pCode, (PVOID)hi);
	}
	// And, within +/- 2GB of relative jmp targets.
	else if (pCode[0] == 0xe9) {   // jmp +imm32
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

const ULONG DETOUR_REGION_SIZE = 0x10000;


static PDETOUR_REGION s_pRegions = nullptr;	// List of all regions.
static PDETOUR_REGION s_pRegion = nullptr;  // Default region

using PZwProtectVirtualMemory = NTSTATUS(NTAPI*)(HANDLE, PVOID*, PULONG, ULONG, PULONG);

PZwProtectVirtualMemory pZwProtectVirtualMemory = nullptr;


NTSTATUS DetourWritableTrampolineRegions() {
	if (pZwProtectVirtualMemory == nullptr) {
		return STATUS_UNSUCCESSFUL;
	}
	ULONG size = DETOUR_REGION_SIZE;
	// Mark all of the regions as writable.
	for (PDETOUR_REGION pRegion = s_pRegions; pRegion != nullptr; pRegion = pRegion->pNext) {
		NTSTATUS status;
		ULONG dummy;
		status = pZwProtectVirtualMemory(ZwCurrentProcess(), (PVOID*)&pRegion,
			&size, PAGE_EXECUTE_READWRITE, &dummy);
		if (!NT_SUCCESS(status)) {
			return status;
		}
	}
	return STATUS_SUCCESS;
}

static PUCHAR DetourAllocRoundDownToRegion(PUCHAR pTry) {
	// WinXP64 returns free areas that aren't REGION aligned to 32-bit applications.
	ULONG_PTR extra = ((ULONG_PTR)pTry) & (DETOUR_REGION_SIZE - 1);
	if (extra != 0) {
		pTry -= extra;
	}
	return pTry;
}

static PUCHAR DetourAllocRoundUpToRegion(PUCHAR pTry) {
	// WinXP64 returns free areas that aren't REGION aligned to 32-bit applications.
	ULONG_PTR extra = ((ULONG_PTR)pTry) & (DETOUR_REGION_SIZE - 1);
	if (extra != 0) {
		ULONG_PTR adjust = DETOUR_REGION_SIZE - extra;
		pTry += adjust;
	}
	return pTry;
}

// Starting at pLo, try to allocate a memory region, continue until pHi.
static PVOID DetourAllocRegionFromLo(PUCHAR pLo, PUCHAR pHi) {
	PUCHAR pTry = DetourAllocRoundUpToRegion(pLo);
	LogDebug("Looking for free region in %p..%p from %p:\n", pLo, pHi, pTry);

	for (; pTry < pHi;) {
		MEMORY_BASIC_INFORMATION mbi;

		RtlZeroMemory(&mbi, sizeof(mbi));
		SIZE_T len;
		NTSTATUS status = ZwQueryVirtualMemory(ZwCurrentProcess(),
			pTry,
			MemoryBasicInformation,
			&mbi,
			sizeof(mbi),
			&len);
		if (!NT_SUCCESS(status)) {
			break;
		}

		LogDebug("Try %p => %p..%p %61x\n", pTry,
			mbi.BaseAddress, (PUCHAR)mbi.BaseAddress + mbi.RegionSize - 1,
			mbi.State);
		if (mbi.State == MEM_FREE && mbi.RegionSize >= DETOUR_REGION_SIZE) {
			PVOID pv = nullptr;
			SIZE_T size = DETOUR_REGION_SIZE;
			status = ZwAllocateVirtualMemory(ZwCurrentProcess(), &pv, 0, &size,
				MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			if (pv != nullptr) {
				return pv;
			}
			else if (status == STATUS_DYNAMIC_CODE_BLOCKED) {
				return nullptr;
			}
			pTry += DETOUR_REGION_SIZE;
		}
		else {
			pTry = DetourAllocRoundUpToRegion((PUCHAR)mbi.BaseAddress + mbi.RegionSize);
		}
	}
	return nullptr;
}

// Starting at pHi, try to allocate a memory region, continue until pLo.
static PVOID DetourAllocRegionFromHi(PUCHAR pLo, PUCHAR pHi) {
	PUCHAR pTry = DetourAllocRoundUpToRegion(pHi - DETOUR_REGION_SIZE);

	LogDebug("Looking for free region in %p..%p from %p:\n", pLo, pHi, pTry);

	for (; pTry > pLo;) {
		MEMORY_BASIC_INFORMATION mbi;

		LogDebug("Try %p\n", pTry);

		RtlZeroMemory(&mbi, sizeof(mbi));
		SIZE_T len;
		NTSTATUS status = ZwQueryVirtualMemory(ZwCurrentProcess(),
			pTry,
			MemoryBasicInformation,
			&mbi,
			sizeof(mbi),
			&len);
		if (!NT_SUCCESS(status)) {
			break;
		}

		LogDebug("Try %p => %p..%p %61x\n",
			pTry, mbi.BaseAddress, (PUCHAR)mbi.BaseAddress + mbi.RegionSize - 1,
			mbi.State);
		if (mbi.State == MEM_FREE && mbi.RegionSize >= DETOUR_REGION_SIZE) {
			PVOID pv = nullptr;
			SIZE_T size = DETOUR_REGION_SIZE;
			status = ZwAllocateVirtualMemory(ZwCurrentProcess(), &pv, 0, &size,
				MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			if (pv != nullptr) {
				return pv;
			}
			else if (status == STATUS_DYNAMIC_CODE_BLOCKED) {
				return nullptr;
			}
			pTry -= DETOUR_REGION_SIZE;
		}
		else {
			pTry = DetourAllocRoundDownToRegion((PUCHAR)mbi.AllocationBase - DETOUR_REGION_SIZE);
		}
	}
	return nullptr;
}

static PVOID DetourAllocTrampolineAllocateNew(PUCHAR pTarget,
	PDETOUR_TRAMPOLINE pLo,
	PDETOUR_TRAMPOLINE pHi) {
	PVOID pTry = nullptr;
	// NB: We must always also start the search at an offset from pTarget
	//     in order to maintain ASLR entropy.

#if defined(DETOURS_64BIT)
// Try looking 1GB below or lower.
	if (pTry == NULL && pTarget > (PUCHAR)0x40000000) {
		pTry = DetourAllocRegionFromHi((PUCHAR)pLo, pTarget - 0x40000000);
	}
	// Try looking 1GB above or higher.
	if (pTry == NULL && pTarget < (PUCHAR)0xffffffff40000000) {
		pTry = DetourAllocRegionFromLo(pTarget + 0x40000000, (PUCHAR)pHi);
	}
	// Try looking 1GB below or higher.
	if (pTry == NULL && pTarget > (PUCHAR)0x40000000) {
		pTry = DetourAllocRegionFromLo(pTarget - 0x40000000, pTarget);
	}
	// Try looking 1GB above or lower.
	if (pTry == NULL && pTarget < (PUCHAR)0xffffffff40000000) {
		pTry = DetourAllocRegionFromHi(pTarget, pTarget + 0x40000000);
	}
#endif

	// Try anything below.
	if (pTry == nullptr) {
		pTry = DetourAllocRegionFromHi((PUCHAR)pLo, pTarget);
	}
	// Try anything above
	if (pTry == nullptr) {
		pTry = DetourAllocRegionFromLo((PUCHAR)pTarget, (PUCHAR)pHi);
	}
	return pTry;
}

static PDETOUR_TRAMPOLINE DetourAllocTrampoline(PUCHAR pTarget) {
	// We have to place trampolines within +/- 2GB of target.

	PDETOUR_TRAMPOLINE pLo;
	PDETOUR_TRAMPOLINE pHi;

	DetourFindJmpBounds(pTarget, &pLo, &pHi);

	PDETOUR_TRAMPOLINE pTrampoline = nullptr;

	// Insure that there is a default region.
	if (s_pRegion == nullptr && s_pRegions != nullptr) {
		s_pRegion = s_pRegions;
	}

	// First check the default region for an valid free block.
	if (s_pRegion != nullptr && s_pRegion->pFree != nullptr &&
		s_pRegion->pFree >= pLo && s_pRegion->pFree <= pHi) {

	found_region:
		pTrampoline = s_pRegion->pFree;
		// do a last sanity check on region
		if (pTrampoline<pLo || pTrampoline>pHi) {
			return nullptr;
		}
		s_pRegion->pFree = (PDETOUR_TRAMPOLINE)pTrampoline->pRemain;
		memset(pTrampoline, 0xcc, sizeof(*pTrampoline));
		return pTrampoline;
	}

	// Then check the existing regions for a valid free block.
	for (s_pRegion = s_pRegions; s_pRegion != nullptr; s_pRegion = s_pRegion->pNext) {
		if (s_pRegion != nullptr && s_pRegion->pFree != nullptr &&
			s_pRegion->pFree >= pLo && s_pRegion->pFree <= pHi) {
			goto found_region;
		}
	}

	// We need to allocate a new region.

	 // Round pTarget down to 64KB block.
	// /RTCc RuntimeChecks breaks PtrToUlong.
	pTarget = pTarget - (ULONG)((ULONG_PTR)pTarget & 0xffff);


}

static void DetourFreeTrampoline(PDETOUR_TRAMPOLINE pTrampoline) {
	PDETOUR_REGION pRegion = (PDETOUR_REGION)
		((ULONG_PTR)pTrampoline & ~(ULONG_PTR)0xffff);

	memset(pTrampoline, 0, sizeof(*pTrampoline));
	pTrampoline->pRemain = (PUCHAR)pRegion->pFree;
	pRegion->pFree = pTrampoline;
}


PVOID NTAPI DetourCodeFromPointer(_In_ PVOID pPointer,
	_Out_opt_ PVOID* ppGlobals) {
	return DetourSkipJmp((PUCHAR)pPointer, ppGlobals);
}

LONG NTAPI DetourTransactionBegin() {
	// Only one transaction is allowed at a time

	if (s_PendingThreadId != 0) {
		return STATUS_UNSUCCESSFUL;
	}

	if (InterlockedCompareExchange(&s_PendingThreadId, HandleToUlong(PsGetCurrentThreadId()), 0) != 0) {
		return STATUS_UNSUCCESSFUL;
	}

	s_pPendingOperations = nullptr;
	s_pPendingThreads = nullptr;
	s_ppPendingError = nullptr;

	// Make sure the trampoline pages are writable
	s_PendingError = DetourWritableTrampolineRegions();

	return s_PendingError;
}

NTSTATUS NTAPI DetourUpdateThread(_In_ HANDLE hThread) {
	NTSTATUS error;
	// If any of the pending operation falied, then we don't need to do this
	if (s_PendingError != STATUS_SUCCESS) {
		return s_PendingError;
	}

	// Silently (and safely) drop any attempt to suspend our own thread.
	if (hThread == ZwCurrentThread()) {
		return STATUS_SUCCESS;
	}

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI DetourTransactionCommit() {
	
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

	if (s_PendingThreadId != HandleToUlong(PsGetCurrentThreadId())) {
		LogDebug("transaction conflict with thread id = %ld\n", s_PendingThreadId);
		return STATUS_INVALID_PARAMETER;
	}

	// If any the pending operations failed, then we don't need to do this.
	if (s_PendingError != STATUS_SUCCESS) {
		LogDebug("pending transaction error=0x%X", s_PendingError);
		return s_PendingError;
	}

	if (ppPointer == nullptr) {
		LogDebug("ppPointer is null\n");
		return STATUS_INVALID_HANDLE;
	}

	if (*ppPointer == nullptr) {
		status = STATUS_INVALID_HANDLE;
		s_PendingError = status;
		s_ppPendingError = ppPointer;
		LogDebug("*ppPointer is null (ppPointer=%p)\n", ppPointer);
		DbgBreakPoint();
		return status;
	}

	PUCHAR pTarget = (PUCHAR)*ppPointer;
	PDETOUR_TRAMPOLINE pTrampoline = nullptr;
	DetourOperation* o = nullptr;

#ifdef DETOURS_IA64
	PPLABEL_DESCRIPTOR ppldDetour = (PPLABEL_DESCRIPTOR)pDetour;
	PPLABEL_DESCRIPTOR ppldTarget = (PPLABEL_DESCRIPTOR)pbTarget;
	PVOID pDetourGlobals = NULL;
	PVOID pTargetGlobals = NULL;

	pDetour = (PBYTE)DetourCodeFromPointer(ppldDetour, &pDetourGlobals);
	pbTarget = (PBYTE)DetourCodeFromPointer(ppldTarget, &pTargetGlobals);
	DETOUR_TRACE(("  ppldDetour=%p, code=%p [gp=%p]\n",
		ppldDetour, pDetour, pDetourGlobals));
	DETOUR_TRACE(("  ppldTarget=%p, code=%p [gp=%p]\n",
		ppldTarget, pbTarget, pTargetGlobals));
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
		s_PendingError = status;
		DbgBreakPoint();
	stop:
		if (pTrampoline != nullptr) {
			DetourFreeTrampoline(pTrampoline);
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
		s_ppPendingError = ppPointer;
		return status;
	}

	
}