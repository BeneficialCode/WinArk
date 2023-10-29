#pragma once

enum class SectionType {
    None,
    Native, // Native section - meaning: 64bit on a 64 bit OS, or 32-bit on a 32-bit OS
    Wow,    // Wow64 section - meaning 32-bit on a 64-bit OS
};

#ifndef SEC_IMAGE
#define SEC_IMAGE 0x01000000
#endif

struct DllStats {
    SectionType Type;
    PVOID Section{ nullptr };   // Section Object
	ULONG ShellCodeRVA = 0;
	ULONG DllNameRVA{};
	PVOID PreferredAddress{};
	ULONG SizeOfImage{};

    bool IsVaid() {
		return Section != nullptr &&
			ShellCodeRVA != 0 &&
			DllNameRVA != 0 &&
			PreferredAddress != nullptr &&
			SizeOfImage != 0;
    }
};

struct Section {
    SectionType _type;
    RTL_RUN_ONCE _sectionSingletonState;

    NTSTATUS Initialize(SectionType type);
    NTSTATUS GetSection(DllStats** ppSectionInfo = nullptr);
    NTSTATUS FreeSection();
    NTSTATUS InjectDLL(DllStats* pDllStats);
	static NTSTATUS MapSectionForShellCode(DllStats* pDllStats,
		PVOID* pBaseAddr = nullptr);

private:
    NTSTATUS CreateKnownDllSection(DllStats& pDllState);
};

extern "C"
PIMAGE_NT_HEADERS RtlImageNtHeader(
	_In_ PVOID BaseAddress
);

extern "C" {
	enum KAPC_ENVIRONMENT
	{
		OriginalApcEnvironment,
		AttachedApcEnvironment,
		CurrentApcEnvironment,
		InsertApcEnvironment
	};

	enum SECTION_INFORMATION_CLASS
	{
		SectionBasicInformation,
		SectionImageInformation
	};


	typedef
		VOID NTAPI
		KNORMAL_ROUTINE(
			_In_opt_ PVOID NormalContext,
			_In_opt_ PVOID SystemArgument1,
			_In_opt_ PVOID SystemArgument2
		);
	typedef KNORMAL_ROUTINE* PKNORMAL_ROUTINE;

	typedef
		VOID NTAPI
		KKERNEL_ROUTINE(
			_In_ struct _KAPC* Apc,
			_Inout_opt_ PKNORMAL_ROUTINE* NormalRoutine,
			_Inout_opt_ PVOID* NormalContext,
			_Inout_opt_ PVOID* SystemArgument1,
			_Inout_opt_ PVOID* SystemArgument2
		);
	typedef KKERNEL_ROUTINE* PKKERNEL_ROUTINE;

	typedef
		VOID NTAPI
		KRUNDOWN_ROUTINE(
			_In_ struct _KAPC* Apc
		);
	typedef KRUNDOWN_ROUTINE* PKRUNDOWN_ROUTINE;


    __declspec(dllimport) void KeInitializeApc(
        _In_ PKAPC Apc,
        _In_ PKTHREAD Thread,
        _In_ KAPC_ENVIRONMENT ApcIndex,
        _In_ PKKERNEL_ROUTINE KernelRoutine,
        _In_ PKRUNDOWN_ROUTINE RundownRoutine,
        _In_ PKNORMAL_ROUTINE NormalRoutine,
        _In_ ULONG ApcMode,
        _In_ PVOID NormalContext
    );

	__declspec(dllimport) BOOLEAN KeInsertQueueApc(
		_In_ PKAPC Apc,
		_In_ PVOID SystemArgument1,
		_In_ PVOID SystemArgument2,
		_In_ ULONG PriorityIncrement
	);

	__declspec(dllimport) NTSTATUS MmMapViewOfSection(
		_In_ PVOID SectionToMap,
		_In_ PEPROCESS Process,
		_Inout_ PVOID* CapturedBase,
		_In_ ULONG_PTR ZeroBits,
		_In_ SIZE_T CommitSize,
		_Inout_ PLARGE_INTEGER SectionOffset,
		_Inout_ PSIZE_T CapturedViewSize,
		_In_ SECTION_INHERIT InheritDisposition,
		_In_ ULONG AllocationType,
		_In_ ULONG Protect
	);

	__declspec(dllimport) NTSTATUS ZwQuerySection(
		_In_ HANDLE SectionHandle,
		_In_ ULONG SectionInformationClass,
		_Out_ PVOID SectionInformation,
		_In_ ULONG SectionInformationLength,
		_Out_ PSIZE_T ResultLength OPTIONAL
	);

	__declspec(dllimport) PVOID RtlImageDirectoryEntryToData(PVOID Base, BOOLEAN MappedAsImage, USHORT DirectoryEntry, PULONG Size);
}