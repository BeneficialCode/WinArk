#include "stdafx.h"
#include "ProcessAccessHelper.h"



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
