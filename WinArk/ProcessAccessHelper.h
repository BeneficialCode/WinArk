#pragma once
#include "ProcessModuleTracker.h"


class ProcessAccessHelper {
public:
	static inline HANDLE _hProcess;
	static inline DWORD_PTR _targetImageBase;
	static inline DWORD_PTR _targetSizeOfImage;
	static inline DWORD_PTR _maxValidAddress;

	static inline const size_t PE_HEADER_BYTES_COUNT = 2000;
	static inline BYTE _fileHeaderFromDisk[PE_HEADER_BYTES_COUNT];

	static bool ReadMemoryFromProcess(DWORD_PTR address, SIZE_T size, LPVOID pData);
	static bool WriteMemoryToProcess(DWORD_PTR address, SIZE_T size, LPVOID pData);
	static bool ReadMemoryPartlyFromProcess(DWORD_PTR address, SIZE_T size, LPVOID pData);

	static bool ReadMemoryFromFile(HANDLE hFile, LONG offset, DWORD size, LPVOID pData);
	static bool WriteMemoryToFile(HANDLE hFile, LONG offset, DWORD size, LPVOID pData);

	static bool WriteMemoryToNewFile(const WCHAR* pFileName, DWORD size, LPVOID pData);
	static bool WriteMemoryToFileEnd(HANDLE hFile, DWORD size, LPVOID pData);

	static bool GetMemoryRegionFromAddress(DWORD_PTR address, DWORD_PTR* pMemoryRegionBase, SIZE_T* pMemoryRegionSize);

	static SIZE_T GetSizeOfImageProcess(HANDLE hProcess, DWORD_PTR moduleBase);

	static LONGLONG GetFileSize(HANDLE hFile);
	static LONGLONG GetFileSize(const WCHAR* pFilePath);

	static DWORD GetEntryPointFromFile(const WCHAR* pFilePath);

	static bool CreateBackupFile(const WCHAR* pFilePath);

	static bool IsPageExecutable(DWORD protect);
	static bool IsPageAccessable(DWORD protect);
};

typedef enum _MEMORY_INFORMATION_CLASS
{
	MemoryBasicInformation, // q: MEMORY_BASIC_INFORMATION
	MemoryWorkingSetInformation, // q: MEMORY_WORKING_SET_INFORMATION
	MemoryMappedFilenameInformation, // q: UNICODE_STRING
	MemoryRegionInformation, // q: MEMORY_REGION_INFORMATION
	MemoryWorkingSetExInformation, // q: MEMORY_WORKING_SET_EX_INFORMATION // since VISTA
	MemorySharedCommitInformation, // q: MEMORY_SHARED_COMMIT_INFORMATION // since WIN8
	MemoryImageInformation, // q: MEMORY_IMAGE_INFORMATION
	MemoryRegionInformationEx, // MEMORY_REGION_INFORMATION
	MemoryPrivilegedBasicInformation, // MEMORY_BASIC_INFORMATION
	MemoryEnclaveImageInformation, // MEMORY_ENCLAVE_IMAGE_INFORMATION // since REDSTONE3
	MemoryBasicInformationCapped, // 10
	MemoryPhysicalContiguityInformation, // MEMORY_PHYSICAL_CONTIGUITY_INFORMATION // since 20H1
	MemoryBadInformation, // since WIN11
	MemoryBadInformationAllProcesses, // since 22H1
	MemoryImageExtensionInformation, // MEMORY_IMAGE_EXTENSION_INFORMATION // since 24H2
	MaxMemoryInfoClass
} MEMORY_INFORMATION_CLASS;

typedef struct _MEMORY_REGION_INFORMATION
{
	PVOID AllocationBase;
	ULONG AllocationProtect;
	union
	{
		ULONG RegionType;
		struct
		{
			ULONG Private : 1;
			ULONG MappedDataFile : 1;
			ULONG MappedImage : 1;
			ULONG MappedPageFile : 1;
			ULONG MappedPhysical : 1;
			ULONG DirectMapped : 1;
			ULONG SoftwareEnclave : 1; // REDSTONE3
			ULONG PageSize64K : 1;
			ULONG PlaceholderReservation : 1; // REDSTONE4
			ULONG MappedAwe : 1; // 21H1
			ULONG MappedWriteWatch : 1;
			ULONG PageSizeLarge : 1;
			ULONG PageSizeHuge : 1;
			ULONG Reserved : 19;
		};
	};
	SIZE_T RegionSize;
	SIZE_T CommitSize;
	ULONG_PTR PartitionId; // 19H1
	ULONG_PTR NodePreference; // 20H1
} MEMORY_REGION_INFORMATION, * PMEMORY_REGION_INFORMATION;

extern "C"
NTSTATUS
NTAPI
NtQueryVirtualMemory(_In_ HANDLE ProcessHandle,
	_In_ PVOID Address,
	_In_opt_ MEMORY_INFORMATION_CLASS MemoryInformationClass,
	_Out_ PVOID MemoryInformation,
	_In_ SIZE_T MemoryInformationLength,
	_Out_opt_ PSIZE_T ReturnLength);