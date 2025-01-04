#include "stdafx.h"
#include "ProcessAccessHelper.h"
#include <Psapi.h>
#include <PEParser.h>

bool ProcessAccessHelper::ReadMemoryFromProcess(DWORD_PTR address, SIZE_T size, LPVOID pData)
{
	SIZE_T read = 0;
	DWORD protect = 0;
	bool ret = false;

	if (!_hProcess) {
		return false;
	}

	if (!ReadProcessMemory(_hProcess, (LPVOID)address, pData, size, &read)) {
		if (!VirtualProtectEx(_hProcess, (LPVOID)address, size, PAGE_READWRITE, &protect))
		{
			ret = false;
		}
		else {
			if (!ReadProcessMemory(_hProcess, (LPVOID)address, pData, size, &read)) {
				ret = false;
			}
			else {
				ret = true;
			}
			VirtualProtectEx(_hProcess, (LPVOID)address, size, protect, &protect);
		}
	}
	else {
		ret = true;
	}

	if (ret) {
		if (size != read) {
			ret = false;
		}
		else {
			ret = true;
		}
	}

	return ret;
}

bool ProcessAccessHelper::ReadMemoryFromFile(HANDLE hFile, LONG offset, DWORD size, LPVOID pData) {
	DWORD bytes = 0;
	DWORD ret;
	DWORD error;

	if (hFile != INVALID_HANDLE_VALUE) {
		ret = SetFilePointer(hFile, offset, nullptr, FILE_BEGIN);
		if (ret == INVALID_SET_FILE_POINTER) {
			error = GetLastError();
			return false;
		}
		return ReadFile(hFile, pData, size, &bytes, nullptr);
	}
	return false;
}

bool ProcessAccessHelper::WriteMemoryToFile(HANDLE hFile, LONG offset, DWORD size, LPVOID pData) {
	DWORD bytes;
	DWORD ret;
	DWORD error;

	if (hFile != INVALID_HANDLE_VALUE) {
		ret = SetFilePointer(hFile, offset, nullptr, FILE_BEGIN);
		if (ret == INVALID_SET_FILE_POINTER) {
			error = GetLastError();
			return false;
		}
		return WriteFile(hFile, pData, size, &bytes, nullptr);
	}
	return false;
}

bool ProcessAccessHelper::WriteMemoryToProcess(DWORD_PTR address, SIZE_T size, LPVOID pData) {
	SIZE_T bytes;
	if (!_hProcess) {
		return false;
	}
	return WriteProcessMemory(_hProcess, (PVOID)address, pData, size, &bytes);
}

bool ProcessAccessHelper::ReadMemoryPartlyFromProcess(DWORD_PTR address, SIZE_T size, LPVOID pData) {
	DWORD_PTR pAddress = 0;
	DWORD_PTR readBytes = 0;
	DWORD_PTR bytesToRead = 0;
	MEMORY_BASIC_INFORMATION mbi = { 0 };
	bool ret = false;

	if (!_hProcess) {
		return ret;
	}

	ret = ReadMemoryFromProcess(address, size, pData);
	if (!ret) {
		do
		{
			pAddress = address;
			if (!VirtualQueryEx(_hProcess, (BYTE*)pAddress, &mbi, sizeof(mbi))){
				break;
			}
			bytesToRead = mbi.RegionSize;
			if ((readBytes + bytesToRead) > size) {
				bytesToRead = size - readBytes;
			}

			if (mbi.State == MEM_COMMIT) {
				if (!ReadMemoryFromProcess(pAddress, bytesToRead, (BYTE*)pData + readBytes))
					break;
			}
			else {
				ZeroMemory((BYTE*)pData + readBytes, bytesToRead);
			}

			readBytes += bytesToRead;

			pAddress += mbi.RegionSize;
		} while (readBytes < size);

		if (readBytes == size)
			ret = true;
	}

	return ret;
}

bool ProcessAccessHelper::WriteMemoryToNewFile(const WCHAR* pFileName, DWORD size, LPVOID pData) {
	HANDLE hFile = ::CreateFile(pFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	bool success = WriteMemoryToFile(hFile, 0, size, pData);
	CloseHandle(hFile);
	return success;
}

bool ProcessAccessHelper::WriteMemoryToFileEnd(HANDLE hFile, DWORD size, LPVOID pData) {
	DWORD bytes = 0;
	bool ret = false;

	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	SetFilePointer(hFile, 0, 0, FILE_END);
	return WriteFile(hFile, pData, size, &bytes, nullptr);
}

bool ProcessAccessHelper::GetMemoryRegionFromAddress(DWORD_PTR address, DWORD_PTR* pMemoryRegionBase, SIZE_T* pMemoryRegionSize) {
	MEMORY_BASIC_INFORMATION mbi;

	SIZE_T size = VirtualQueryEx(_hProcess, (BYTE*)address, &mbi, sizeof(mbi));
	if (size == 0) {
		return false;
	}

	*pMemoryRegionBase = (DWORD_PTR)mbi.BaseAddress;
	*pMemoryRegionSize = mbi.RegionSize;
	return true;
}

SIZE_T ProcessAccessHelper::GetSizeOfImageProcess(HANDLE hProcess, DWORD_PTR moduleBase) {
	SIZE_T size = 0;
	MEMORY_BASIC_INFORMATION mbi = { 0 };
	MEMORY_REGION_INFORMATION mri = { 0 };
	SIZE_T len = 0;
	NTSTATUS status = ::NtQueryVirtualMemory(_hProcess, (PVOID)moduleBase, MemoryRegionInformation, &mri, sizeof(mri), &len);
	if (status > 0) {
		return mri.RegionSize;
	}

	WCHAR name[MAX_PATH];
	GetMappedFileName(_hProcess, (LPVOID)moduleBase, name, _countof(name));

	WCHAR path[MAX_PATH];
	do
	{
		if (!VirtualQueryEx(_hProcess, (LPVOID)moduleBase, &mbi, sizeof(mbi)))
			return 0;

		GetMappedFileName(_hProcess, (LPVOID)moduleBase, path, _countof(path));

		moduleBase = moduleBase + mbi.RegionSize;

		size += mbi.RegionSize;

		if (_wcsicmp(name, path) != 0) {
			break;
		}
	} while (mbi.Type == MEM_IMAGE);

	return size;
}

LONGLONG ProcessAccessHelper::GetFileSize(HANDLE hFile) {
	LARGE_INTEGER fileSize = { 0 };
	if (hFile != INVALID_HANDLE_VALUE) {
		if (!GetFileSizeEx(hFile, &fileSize))
			return 0;
		return fileSize.QuadPart;
	}
	return 0;
}

LONGLONG ProcessAccessHelper::GetFileSize(const WCHAR* pFilePath)
{
	LONGLONG fileSize = 0;
	HANDLE hFile = ::CreateFile(pFilePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile != INVALID_HANDLE_VALUE) {
		fileSize = GetFileSize(hFile);
		CloseHandle(hFile);
	}
	return fileSize;
}

DWORD ProcessAccessHelper::GetEntryPointFromFile(const WCHAR* pFilePath)
{
	PEParser parser(pFilePath);
	return parser.GetAddressOfEntryPoint();
}

bool ProcessAccessHelper::CreateBackupFile(const WCHAR* pFilePath) {
	WCHAR backupFile[MAX_PATH];
	wcscpy_s(backupFile, pFilePath);
	wcscat_s(backupFile, L".bak");
	return CopyFile(pFilePath, backupFile, FALSE);
}

bool ProcessAccessHelper::IsPageExecutable(DWORD protect) {
	DWORD prots[] = { PAGE_EXECUTE,PAGE_EXECUTE_READ,PAGE_EXECUTE_READWRITE,PAGE_EXECUTE_WRITECOPY };

	for (DWORD prot : prots) {
		if (prot == (protect & 0xff))
			return true;
	}
	return false;
}

bool ProcessAccessHelper::IsPageAccessable(DWORD protect) {
	if (PAGE_NOACCESS == (protect & 0xff)) {
		return true;
	}
	return false;
}