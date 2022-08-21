#pragma once
/////////////////////////////////////////////////////////////////////////////
//
//  Core Detours Functionality (detours.h of detours.lib)
//
//  Microsoft Research Detours Package, Version 4.0.1
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//

// 0x MAJOR c MINOR c PATCH
#define DETOURS_VERSION		0x4c0c1

#ifdef _X86_
#define DETOURS_X86
#define DETOURS_OPTION_BITS 64

#elif defined(_AMD64_)
#define DETOURS_X64
#define DETOURS_OPTION_BITS 32

#elif defined(_IA64_)
#define DETOURS_IA64
#define DETOURS_OPTION_BITS 32

#elif defined(_ARM_)
#define DETOURS_ARM

#elif defined(_ARM64_)
#define DETOURS_ARM64

#else
#error Unknown architecture (x86, amd64, ia64, arm, arm64)
#endif

#ifdef _WIN64
#undef DETOURS_32BIT
#define DETOURS_64BIT 1
#define DETOURS_BITS 64
// If all 64bit kernels can run one and only one 32bit architecture.
//#define DETOURS_OPTION_BITS 32
#else
#define DETOURS_32BIT 1
#undef DETOURS_64BIT
#define DETOURS_BITS 32
// If all 64bit kernels can run one and only one 32bit architecture.
//#define DETOURS_OPTION_BITS 32
#endif

//////////////////////////////////////////////////////////////////////////////
//
struct _DETOUR_ALIGN
{
	UCHAR    obTarget : 3;
	UCHAR    obTrampoline : 5;
};

C_ASSERT(sizeof(_DETOUR_ALIGN) == 1);

typedef struct _DETOUR_TRAMPOLINE DETOUR_TRAMPOLINE, * PDETOUR_TRAMPOLINE;

///////////////////////////////////////////////////////// Transaction Structs.
//


struct DetourOperation {
	DetourOperation* pNext;
	bool fIsRemove;
	PUCHAR* ppPointer;
	PUCHAR pTarget;
	PDETOUR_TRAMPOLINE pTrampoline;
};

/*
---- Detours定义了三个概念：

-- (1) Target函数：要拦截的函数，通常为Windows的API。
-- (2) Trampoline函数：Target函数的复制品。
	因为Detours将会改写Target函数，所以先把Target函数复制保存好，
	一方面仍然保存Target函数的过程调用语义，
	另一方面便于以后的恢复。
-- (3) Detour 函数：用来替代Target函数的函数。
*/

//////////////////////////////////////////////////////////// Transaction APIs.
//
NTSTATUS NTAPI DetourTransactionCommit();
NTSTATUS NTAPI DetourTransactionCommitEx();

////////////////////////////////////////////////////////////// Code Functions.
//
PVOID NTAPI DetourCodeFromPointer(_In_ PVOID pPointer,
	_Out_opt_ PVOID* ppGlobals);

///////////////////////////////////////////////////////////// Transacted APIs.
//
// 寻找可以Hook的地址（当前地址可能无法Hook），分配Trampline块，
// 并且确定Trampline中的拷贝的指令字节数，加入操作（Operation）列表
NTSTATUS NTAPI DetourAttach(_Inout_ PVOID* ppPointer,
	_In_ PVOID pDetour);

NTSTATUS NTAPI DetourAttachEx(_Inout_ PVOID* ppPointer,
	_In_ PVOID pDetour,
	_Out_opt_ PDETOUR_TRAMPOLINE* ppRealTrampoline,
	_Out_opt_ PVOID* ppRealTarget,
	_Out_opt_ PVOID* ppRealDetour);

NTSTATUS NTAPI DetourDetach(_Inout_ PVOID* ppPointer,
	_In_ PVOID pDetour);

using PZwProtectVirtualMemory = NTSTATUS(NTAPI*)(HANDLE, PVOID*, PULONG, ULONG, PULONG);

extern PZwProtectVirtualMemory pZwProtectVirtualMemory;