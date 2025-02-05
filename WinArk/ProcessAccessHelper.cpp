#include "stdafx.h"
#include "ProcessAccessHelper.h"
#include <Psapi.h>
#include <PEParser.h>
#include <DriverHelper.h>
#include "ProcessModuleTracker.h"

bool ProcessAccessHelper::OpenProcessHandle(DWORD pid) {
	if (pid > 0) {
		if (_hProcess)
			return false;
		else {
			_hProcess = DriverHelper::OpenProcess(pid, PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION
				| PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE
				| PROCESS_SUSPEND_RESUME | PROCESS_TERMINATE);
			if (_hProcess) {
				return true;
			}
			else {
				return false;
			}
		}
	}
	return false;
}

void ProcessAccessHelper::CloseProcessHandle() {
	if (_hProcess) {
		::CloseHandle(_hProcess);
		_hProcess = NULL;
	}

	_moduleList.clear();
	_targetImageBase = 0;
	_pSelectedModule = nullptr;
}

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
		return false;
	}
	return true;
}

LPVOID ProcessAccessHelper::CreateFileMappingViewRead(const WCHAR* filePath) {
	return CreateFileMappingView(filePath, GENERIC_READ, PAGE_READONLY | SEC_IMAGE, FILE_MAP_READ);
}

LPVOID ProcessAccessHelper::CreateFileMappingViewFull(const WCHAR* filePath)
{
	return CreateFileMappingView(filePath, GENERIC_ALL, PAGE_EXECUTE_READWRITE, FILE_MAP_ALL_ACCESS);
}

LPVOID ProcessAccessHelper::CreateFileMappingView(const WCHAR* filePath, DWORD accessFile, DWORD protect, DWORD accessMap) {
	HANDLE hFile = ::CreateFile(filePath, accessFile, FILE_SHARE_READ, nullptr, OPEN_EXISTING,0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return nullptr;

	HANDLE hMap = ::CreateFileMapping(hFile, nullptr, protect, 0, 0, nullptr);
	if (hMap == NULL) {
		return nullptr;
	}

	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		CloseHandle(hMap);
		return nullptr;
	}

	LPVOID pMappedDll = ::MapViewOfFile(hMap, accessMap, 0, 0, 0);
	if (pMappedDll == NULL) {
		CloseHandle(hMap);
		return nullptr;
	}

	CloseHandle(hMap);
	return pMappedDll;
}

bool ProcessAccessHelper::DecomposeMemory(BYTE* pMemory, SIZE_T size, DWORD_PTR startAddress) {
	ZeroMemory(&_codeInfo, sizeof(_CodeInfo));
	_codeInfo.code = pMemory;
	_codeInfo.codeLen = size;
	BOOL isWow64 = FALSE;
	::IsWow64Process(_hProcess, &isWow64);
	if (isWow64) {
		_codeInfo.dt = Decode32Bits;
	}
	else {
		_codeInfo.dt = Decode64Bits;
	}
	_codeInfo.codeOffset = startAddress;

	if (distorm_decompose(&_codeInfo, _insts, sizeof(_insts) / sizeof(_insts[0]), &_instCount) == DECRES_INPUTERR) {
		return false;
	}
	return true;
}

bool ProcessAccessHelper::DisassembleMemory(BYTE* pMemory, SIZE_T size, DWORD_PTR startOffset) {
	_DecodeResult result;
	_decodedInstCount = 0;
	_OffsetType offset = startOffset;
	_DecodeType type = Decode64Bits;
	BOOL isWow64 = FALSE;
	::IsWow64Process(_hProcess, &isWow64);
	if (isWow64) {
		type = Decode32Bits;
	}
	result = distorm_decode(offset, pMemory, size, type, _decodedInsts, MAX_INSTRUCTIONS, &_decodedInstCount);
	if (result == DECRES_INPUTERR)
		return false;

	return true;
}

bool ProcessAccessHelper::GetProcessModules(HANDLE hProcess, std::vector<ModuleInfo>& moduleList) {
	ModuleInfo module;
	DWORD needed = 0;

	_moduleList.reserve(20);

	WinSys::ProcessModuleTracker tracker(_pid);

	tracker.EnumModules();

	auto infos = tracker.GetModules();

	for (auto& info : infos) {
		std::wstring path = info.get()->Path;
		std::wstring name = info.get()->Name;
		if (path.empty())
			continue;
		if (_wcsicmp(L"kernelbase.dll", name.c_str()) == 0)
			continue;
		

		module._modBaseAddr = (DWORD_PTR)info.get()->Base;
		module._modBaseSize = info.get()->ModuleSize;
		module._isAlreadyParsed = false;
		module._parsing = false;

		module._fullPath[0] = 0;

		wcscpy_s(module._fullPath, path.c_str());

		_moduleList.push_back(module);
	}

	return true;
}