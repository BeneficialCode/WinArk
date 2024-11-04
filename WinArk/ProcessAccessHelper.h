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
	
	static bool ReadHeaderFromFile(BYTE* pBuffer, DWORD bufferSize, const WCHAR* pFilePath);

	static SIZE_T GetSizeOfImageProcess(HANDLE hProcess, DWORD_PTR moduleBase);

	static LONGLONG GetFileSize(HANDLE hFile);
	static LONGLONG GetFileSize(const WCHAR* pFilePath);

	static DWORD GetEntryPointFromFile(const WCHAR* pFilePath);

	static bool CreateBackupFile(const WCHAR* pFilePath);

	static bool IsPageExecutable(DWORD protect);
	static bool IsPageAccessable(DWORD protect);
};