#pragma once

/*
 * This file is part of the Process Hacker project - https://processhacker.sourceforge.io/
 *
 * You can redistribute this file and/or modify it under the terms of the
 * Attribution 4.0 International (CC BY 4.0) license.
 *
 * You must give appropriate credit, provide a link to the license, and
 * indicate if changes were made. You may do so in any reasonable manner, but
 * not in any way that suggests the licensor endorses you or your use.
 */

 // 32-bit definitions
#define WOW64_SYSTEM_DIRECTORY_U L"SysWOW64"

#define WOW64_POINTER(Type) ULONG

#define GDI_HANDLE_BUFFER_SIZE32 34
#define GDI_HANDLE_BUFFER_SIZE64 60

#ifndef _WIN64
#define GDI_HANDLE_BUFFER_SIZE GDI_HANDLE_BUFFER_SIZE32
#else
#define GDI_HANDLE_BUFFER_SIZE GDI_HANDLE_BUFFER_SIZE64
#endif

typedef ULONG GDI_HANDLE_BUFFER32[GDI_HANDLE_BUFFER_SIZE32];

typedef struct _RTL_BALANCED_NODE32
{
	union
	{
		WOW64_POINTER(struct _RTL_BALANCED_NODE*) Children[2];
		struct
		{
			WOW64_POINTER(struct _RTL_BALANCED_NODE*) Left;
			WOW64_POINTER(struct _RTL_BALANCED_NODE*) Right;
		};
	};
	union
	{
		WOW64_POINTER(UCHAR) Red : 1;
		WOW64_POINTER(UCHAR) Balance : 2;
		WOW64_POINTER(ULONG_PTR) ParentValue;
	};
} RTL_BALANCED_NODE32, * PRTL_BALANCED_NODE32;

typedef struct _PEB32
{
	BOOLEAN InheritedAddressSpace;
	BOOLEAN ReadImageFileExecOptions;
	BOOLEAN BeingDebugged;
	union
	{
		BOOLEAN BitField;
		struct
		{
			BOOLEAN ImageUsesLargePages : 1;
			BOOLEAN IsProtectedProcess : 1;
			BOOLEAN IsImageDynamicallyRelocated : 1;
			BOOLEAN SkipPatchingUser32Forwarders : 1;
			BOOLEAN IsPackagedProcess : 1;
			BOOLEAN IsAppContainer : 1;
			BOOLEAN IsProtectedProcessLight : 1;
			BOOLEAN IsLongPathAwareProcess : 1;
		};
	};
	WOW64_POINTER(HANDLE) Mutant;

	WOW64_POINTER(PVOID) ImageBaseAddress;
	WOW64_POINTER(PPEB_LDR_DATA) Ldr;
	WOW64_POINTER(PRTL_USER_PROCESS_PARAMETERS) ProcessParameters;
	WOW64_POINTER(PVOID) SubSystemData;
	WOW64_POINTER(PVOID) ProcessHeap;
	WOW64_POINTER(PRTL_CRITICAL_SECTION) FastPebLock;
	WOW64_POINTER(PVOID) AtlThunkSListPtr;
	WOW64_POINTER(PVOID) IFEOKey;
	union
	{
		ULONG CrossProcessFlags;
		struct
		{
			ULONG ProcessInJob : 1;
			ULONG ProcessInitializing : 1;
			ULONG ProcessUsingVEH : 1;
			ULONG ProcessUsingVCH : 1;
			ULONG ProcessUsingFTH : 1;
			ULONG ReservedBits0 : 27;
		};
	};
	union
	{
		WOW64_POINTER(PVOID) KernelCallbackTable;
		WOW64_POINTER(PVOID) UserSharedInfoPtr;
	};
	ULONG SystemReserved;
	ULONG AtlThunkSListPtr32;
	WOW64_POINTER(PVOID) ApiSetMap;
	ULONG TlsExpansionCounter;
	WOW64_POINTER(PVOID) TlsBitmap;
	ULONG TlsBitmapBits[2];
	WOW64_POINTER(PVOID) ReadOnlySharedMemoryBase;
	WOW64_POINTER(PVOID) HotpatchInformation;
	WOW64_POINTER(PVOID*) ReadOnlyStaticServerData;
	WOW64_POINTER(PVOID) AnsiCodePageData;
	WOW64_POINTER(PVOID) OemCodePageData;
	WOW64_POINTER(PVOID) UnicodeCaseTableData;

	ULONG NumberOfProcessors;
	ULONG NtGlobalFlag;

	LARGE_INTEGER CriticalSectionTimeout;
	WOW64_POINTER(SIZE_T) HeapSegmentReserve;
	WOW64_POINTER(SIZE_T) HeapSegmentCommit;
	WOW64_POINTER(SIZE_T) HeapDeCommitTotalFreeThreshold;
	WOW64_POINTER(SIZE_T) HeapDeCommitFreeBlockThreshold;

	ULONG NumberOfHeaps;
	ULONG MaximumNumberOfHeaps;
	WOW64_POINTER(PVOID*) ProcessHeaps;

	WOW64_POINTER(PVOID) GdiSharedHandleTable;
	WOW64_POINTER(PVOID) ProcessStarterHelper;
	ULONG GdiDCAttributeList;

	WOW64_POINTER(PRTL_CRITICAL_SECTION) LoaderLock;

	ULONG OSMajorVersion;
	ULONG OSMinorVersion;
	USHORT OSBuildNumber;
	USHORT OSCSDVersion;
	ULONG OSPlatformId;
	ULONG ImageSubsystem;
	ULONG ImageSubsystemMajorVersion;
	ULONG ImageSubsystemMinorVersion;
	WOW64_POINTER(ULONG_PTR) ActiveProcessAffinityMask;
	GDI_HANDLE_BUFFER32 GdiHandleBuffer;
	WOW64_POINTER(PVOID) PostProcessInitRoutine;

	WOW64_POINTER(PVOID) TlsExpansionBitmap;
	ULONG TlsExpansionBitmapBits[32];

	ULONG SessionId;

	ULARGE_INTEGER AppCompatFlags;
	ULARGE_INTEGER AppCompatFlagsUser;
	WOW64_POINTER(PVOID) pShimData;
	WOW64_POINTER(PVOID) AppCompatInfo;

	UNICODE_STRING32 CSDVersion;

	WOW64_POINTER(PVOID) ActivationContextData;
	WOW64_POINTER(PVOID) ProcessAssemblyStorageMap;
	WOW64_POINTER(PVOID) SystemDefaultActivationContextData;
	WOW64_POINTER(PVOID) SystemAssemblyStorageMap;

	WOW64_POINTER(SIZE_T) MinimumStackCommit;

	WOW64_POINTER(PVOID) SparePointers[4];
	ULONG SpareUlongs[5];
	//WOW64_POINTER(PVOID *) FlsCallback;
	//LIST_ENTRY32 FlsListHead;
	//WOW64_POINTER(PVOID) FlsBitmap;
	//ULONG FlsBitmapBits[FLS_MAXIMUM_AVAILABLE / (sizeof(ULONG) * 8)];
	//ULONG FlsHighIndex;

	WOW64_POINTER(PVOID) WerRegistrationData;
	WOW64_POINTER(PVOID) WerShipAssertPtr;
	WOW64_POINTER(PVOID) pContextData;
	WOW64_POINTER(PVOID) pImageHeaderHash;
	union
	{
		ULONG TracingFlags;
		struct
		{
			ULONG HeapTracingEnabled : 1;
			ULONG CritSecTracingEnabled : 1;
			ULONG LibLoaderTracingEnabled : 1;
			ULONG SpareTracingBits : 29;
		};
	};
	ULONGLONG CsrServerReadOnlySharedMemoryBase;
	WOW64_POINTER(PVOID) TppWorkerpListLock;
	LIST_ENTRY32 TppWorkerpList;
	WOW64_POINTER(PVOID) WaitOnAddressHashTable[128];
	WOW64_POINTER(PVOID) TelemetryCoverageHeader; // REDSTONE3
	ULONG CloudFileFlags;
	ULONG CloudFileDiagFlags; // REDSTONE4
	CHAR PlaceholderCompatibilityMode;
	CHAR PlaceholderCompatibilityModeReserved[7];
} PEB32, * PPEB32;

typedef struct _PEB_LDR_DATA32
{
	ULONG Length;
	BOOLEAN Initialized;
	WOW64_POINTER(HANDLE) SsHandle;
	LIST_ENTRY32 InLoadOrderModuleList;
	LIST_ENTRY32 InMemoryOrderModuleList;
	LIST_ENTRY32 InInitializationOrderModuleList;
	WOW64_POINTER(PVOID) EntryInProgress;
	BOOLEAN ShutdownInProgress;
	WOW64_POINTER(HANDLE) ShutdownThreadId;
} PEB_LDR_DATA32, * PPEB_LDR_DATA32;

typedef struct _LDR_DATA_TABLE_ENTRY32
{
	LIST_ENTRY32 InLoadOrderLinks;
	LIST_ENTRY32 InMemoryOrderLinks;
	union
	{
		LIST_ENTRY32 InInitializationOrderLinks;
		LIST_ENTRY32 InProgressLinks;
	};
	WOW64_POINTER(PVOID) DllBase;
	WOW64_POINTER(PVOID) EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING32 FullDllName;
	UNICODE_STRING32 BaseDllName;
	union
	{
		UCHAR FlagGroup[4];
		ULONG Flags;
		struct
		{
			ULONG PackagedBinary : 1;
			ULONG MarkedForRemoval : 1;
			ULONG ImageDll : 1;
			ULONG LoadNotificationsSent : 1;
			ULONG TelemetryEntryProcessed : 1;
			ULONG ProcessStaticImport : 1;
			ULONG InLegacyLists : 1;
			ULONG InIndexes : 1;
			ULONG ShimDll : 1;
			ULONG InExceptionTable : 1;
			ULONG ReservedFlags1 : 2;
			ULONG LoadInProgress : 1;
			ULONG LoadConfigProcessed : 1;
			ULONG EntryProcessed : 1;
			ULONG ProtectDelayLoad : 1;
			ULONG ReservedFlags3 : 2;
			ULONG DontCallForThreads : 1;
			ULONG ProcessAttachCalled : 1;
			ULONG ProcessAttachFailed : 1;
			ULONG CorDeferredValidate : 1;
			ULONG CorImage : 1;
			ULONG DontRelocate : 1;
			ULONG CorILOnly : 1;
			ULONG ChpeImage : 1;
			ULONG ReservedFlags5 : 2;
			ULONG Redirected : 1;
			ULONG ReservedFlags6 : 2;
			ULONG CompatDatabaseProcessed : 1;
		};
	};
	USHORT ObsoleteLoadCount;
	USHORT TlsIndex;
	LIST_ENTRY32 HashLinks;
	ULONG TimeDateStamp;
	WOW64_POINTER(struct _ACTIVATION_CONTEXT*) EntryPointActivationContext;
	WOW64_POINTER(PVOID) Lock;
	WOW64_POINTER(PLDR_DDAG_NODE) DdagNode;
	LIST_ENTRY32 NodeModuleLink;
	WOW64_POINTER(struct _LDRP_LOAD_CONTEXT*) LoadContext;
	WOW64_POINTER(PVOID) ParentDllBase;
	WOW64_POINTER(PVOID) SwitchBackContext;
	RTL_BALANCED_NODE32 BaseAddressIndexNode;
	RTL_BALANCED_NODE32 MappingInfoIndexNode;
	WOW64_POINTER(ULONG_PTR) OriginalBase;
	LARGE_INTEGER LoadTime;
	ULONG BaseNameHashValue;
	LDR_DLL_LOAD_REASON LoadReason;
	ULONG ImplicitPathOptions;
	ULONG ReferenceCount;
	ULONG DependentLoadFlags;
	UCHAR SigningLevel; // since REDSTONE2
} LDR_DATA_TABLE_ENTRY32, * PLDR_DATA_TABLE_ENTRY32;