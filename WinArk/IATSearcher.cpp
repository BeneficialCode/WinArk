#include "stdafx.h"
#include "IATSearcher.h"

bool IATSearcher::SearchImportAddressTableInProcess(DWORD_PTR startAddress, DWORD_PTR* pIAT, DWORD* pSize, bool advance) {
	DWORD_PTR addressInIAT = 0;
	*pIAT = 0;
	*pSize = 0;

	if (advance) {
		return FindIATAdvanced(startAddress, pIAT, pSize);
	}

	addressInIAT = FindAPIAddressInIAT(startAddress);

	if (!addressInIAT) {
		return false;
	}
	else {
		return FindIATStartAndSize(addressInIAT, pIAT, pSize);
	}
}

bool IATSearcher::FindIATAdvanced(DWORD_PTR startAddress, DWORD_PTR* pIAT, DWORD* pSize) {
	std::unique_ptr<BYTE[]> buffer;
	DWORD_PTR baseAddress;
	SIZE_T memorySize;

	FindExecutableMemoryPagesByStartAddress(startAddress, &baseAddress, &memorySize);

	if (memorySize == 0)
		return false;

	buffer = std::make_unique<BYTE[]>(memorySize);
	if (!ReadMemoryFromProcess(baseAddress, memorySize, buffer.get())) {
		return false;
	}

	std::set<DWORD_PTR> iatPointers;
	DWORD_PTR next;
	BYTE* pTemp = buffer.get();
	while (DecomposeMemory(pTemp, memorySize, baseAddress) && _instCount != 0) {
		FindIATPointers(iatPointers);

		next = _insts[_instCount - 1].addr - baseAddress;
		next += _insts[_instCount - 1].size;

		pTemp += next;

		if (memorySize <= next) {
			break;
		}

		memorySize -= next;
		baseAddress += next;
	}

	if (iatPointers.size() == 0)
		return false;

	FilterIATPointersList(iatPointers);

	if (iatPointers.size() == 0)
		return false;

	auto iter = iatPointers.begin();
	DWORD_PTR start = *iter;
	*pIAT = start;
	iter = iatPointers.end();
	DWORD_PTR end = *(--iter);
	*pSize = (DWORD)(end - start + sizeof(DWORD_PTR));
	
	if (*pSize > (2000000 * sizeof(DWORD_PTR))) {
		*pIAT = 0;
		*pSize = 0;
		return false;
	}

	return true;
}

DWORD_PTR IATSearcher::FindAPIAddressInIAT(DWORD_PTR startAddress) {
	const size_t readSize = 200;
	BYTE pData[readSize];

	DWORD_PTR iatPointer = 0;
	int count = 0;

	_address = 0;
	_size = 0;

	do
	{
		count++;

		if (!ReadMemoryFromProcess(startAddress, sizeof(pData), pData)) {
			return 0;
		}

		if (DecomposeMemory(pData, sizeof(pData), startAddress)) {
			iatPointer = FindIATPointer();
			if (iatPointer) {
				if (IsIATPointerValid(iatPointer, true)) {
					return iatPointer;
				}
			}
		}

		startAddress = FindNextFunctionAddress();
	} while (startAddress != 0 && count != 8);


	return 0;
}


DWORD_PTR IATSearcher::FindNextFunctionAddress() {
	for (unsigned int i = 0; i < _instCount; i++) {
		if (_insts[i].flags == FLAG_NOT_DECODABLE) {
			continue;
		}
		if (META_GET_FC(_insts[i].meta) == FC_CALL || META_GET_FC(_insts[i].meta) == FC_UNC_BRANCH) {
			if (_insts[i].size >= 5) {
				if (_insts[i].ops[0].type == O_PC) {
					return INSTRUCTION_GET_TARGET(&_insts[i]);
				}
			}
		}
	}

	return 0;
}

DWORD_PTR IATSearcher::FindIATPointer() {
	for (unsigned int i = 0; i < _instCount; i++) {
		if (_insts[i].flags == FLAG_NOT_DECODABLE) {
			continue;
		}

		if (META_GET_FC(_insts[i].meta) == FC_CALL
			|| META_GET_FC(_insts[i].meta) == FC_UNC_BRANCH) {
			if (_insts[i].size >= 5) {
				BOOL isWow64 = FALSE;
				::IsWow64Process(_hProcess, &isWow64);
				if (isWow64) {
					if (_insts[i].ops[0].type == O_DISP) {
						// jmp dword ptr || call dword ptr
						return _insts[i].disp;
					}
				}
				else {
					if(_insts[i].flags & FLAG_RIP_RELATIVE)
						return INSTRUCTION_GET_RIP_TARGET(&_insts[i]);
				}
			}
		}
	}


	return 0;
}


bool IATSearcher::IsIATPointerValid(DWORD_PTR iatPointer, bool checkRedirects) {
	DWORD_PTR apiAddress = 0;
	if (!ReadMemoryFromProcess(iatPointer, sizeof(DWORD_PTR), &apiAddress)) {
		return false;
	}

	if (IsApiAddressValid(apiAddress)) {
		return true;
	}
	else {
		if (checkRedirects) {
			if (apiAddress > _address && apiAddress < (_address + _size)) {
				return true;
			}
			else {
				GetMemoryRegionFromAddress(apiAddress, &_address, &_size);
			}
		}
	}

	return false;
}

bool IATSearcher::FindIATStartAndSize(DWORD_PTR address, DWORD_PTR* pIAT, DWORD* pSize) {
	std::unique_ptr<BYTE[]> buffer;
	DWORD_PTR baseAddress = 0;
	DWORD baseSize = 0;

	GetMemoryBaseAndSizeForIAT(address, &baseAddress, &baseSize);

	if (!baseAddress)
		return false;

	size_t size = baseSize * sizeof(DWORD_PTR) * 3;
	buffer = std::make_unique<BYTE[]>(size);

	if (!buffer)
		return false;

	ZeroMemory(buffer.get(), size);

	if (!ReadMemoryFromProcess(baseAddress, baseSize, buffer.get())) {
		return false;
	}

	*pIAT = FindIATStartAddress(baseAddress, address, buffer.get());
	*pSize = FindIATSize(baseAddress, *pIAT, buffer.get(), baseSize);

	return true;
}

DWORD_PTR IATSearcher::FindIATStartAddress(DWORD_PTR baseAddress, DWORD_PTR startAddress, BYTE* pData) {
	DWORD_PTR* pIATAddress = nullptr;

	pIATAddress = (DWORD_PTR*)((startAddress - baseAddress) + (DWORD_PTR)pData);

	while ((DWORD_PTR)pIATAddress != (DWORD_PTR)pData) {
		if (IsInvalidMemoryForIAT(*pIATAddress)) {
			if ((DWORD_PTR)(pIATAddress - 1) >= (DWORD_PTR)pData) {
				if (IsInvalidMemoryForIAT(*(pIATAddress - 1))) {
					if ((DWORD_PTR)(pIATAddress - 2) >= (DWORD_PTR)pData) {
						DWORD_PTR* pIATStart = pIATAddress - 2;
						DWORD_PTR apiAddress = *pIATStart;
						if (IsApiAddressValid(apiAddress)) {
							if (!IsApiAddressValid(*(pIATStart - 1))) {
								return (((DWORD_PTR)pIATStart - (DWORD_PTR)pData) + baseAddress);
							}
						}
					}
				}
			}
		}

		pIATAddress--;
	}

	return baseAddress;
}


DWORD IATSearcher::FindIATSize(DWORD_PTR baseAddress, DWORD_PTR iatAddress, BYTE* pData, DWORD dataSize) {
	DWORD_PTR* pIATAddress = nullptr;
	pIATAddress = (DWORD_PTR*)((iatAddress - baseAddress) + (DWORD_PTR)pData);

	while ((DWORD_PTR)pIATAddress < ((DWORD_PTR)pData + dataSize - 1)) {
		if (IsInvalidMemoryForIAT(*pIATAddress)) {
			if (IsInvalidMemoryForIAT(*(pIATAddress + 1))) {
				DWORD_PTR apiAddress = *(pIATAddress + 2);
				if (!IsApiAddressValid(apiAddress) && apiAddress != 0) {
					return (DWORD)((DWORD_PTR)pIATAddress - (DWORD_PTR)pData - (iatAddress - baseAddress));
				}
			}
		}
		pIATAddress++;
	}

	return dataSize;
}

void IATSearcher::FindIATPointers(std::set<DWORD_PTR>& iatPointers) {
	for (unsigned int i = 0; i < _instCount; i++) {
		if (_insts[i].flags != FLAG_NOT_DECODABLE) {
			uint16_t fc = META_GET_FC(_insts[i].meta);
			if (fc == FC_CALL || fc == FC_UNC_BRANCH) {
				if (_insts[i].size >= 5) {
					BOOL isWow64 = FALSE;
					::IsWow64Process(_hProcess, &isWow64);
					if (isWow64) {
						if (_insts[i].ops[0].type == O_DISP) {
							iatPointers.insert((DWORD_PTR)_insts[i].disp);
						}
					}
					else {
						if (_insts[i].flags & FLAG_RIP_RELATIVE)
							iatPointers.insert(INSTRUCTION_GET_RIP_TARGET(&_insts[i]));
					}
				}
			}
		}
	}
}

void IATSearcher::FindExecutableMemoryPagesByStartAddress(DWORD_PTR startAddress,
	DWORD_PTR* pBaseAddress, SIZE_T* pMemorySize) {
	MEMORY_BASIC_INFORMATION mbi = { 0 };
	DWORD_PTR tempAddress;

	*pMemorySize = 0;
	*pBaseAddress = 0;

	SIZE_T len = sizeof(MEMORY_BASIC_INFORMATION);
	SIZE_T size = VirtualQueryEx(_hProcess, (LPCVOID)startAddress, &mbi, len);
	if (size != len) {
		return;
	}

	do
	{
		*pMemorySize = mbi.RegionSize;
		*pBaseAddress = (DWORD_PTR)mbi.BaseAddress;
		tempAddress = (DWORD_PTR)mbi.BaseAddress - 1;
		size = VirtualQueryEx(_hProcess, (LPCVOID)tempAddress, &mbi, len);
		if (size != len) {
			break;
		}
	} while (IsPageExecutable(mbi.Protect));

	tempAddress = *pBaseAddress;
	mbi.RegionSize = *pMemorySize;
	*pMemorySize = 0;
	do
	{
		tempAddress += mbi.RegionSize;
		*pMemorySize += mbi.RegionSize;

		if (VirtualQueryEx(_hProcess, (LPCVOID)tempAddress, &mbi, len) != len) {
			break;
		}
	} while (IsPageExecutable(mbi.Protect));
}


void IATSearcher::FilterIATPointersList(std::set<DWORD_PTR>& iatPointers) {
	std::set<DWORD_PTR>::iterator iter;

	if (iatPointers.size() <= 2)
		return;

	iter = iatPointers.begin();
	std::advance(iter, iatPointers.size() / 2);

	DWORD_PTR lastPointer = *iter;
	iter++;

	for (; iter != iatPointers.end(); iter++) {
		if ((*iter - lastPointer) > 0x100) {
			if (IsIATPointerValid(lastPointer, false) == false || IsIATPointerValid(*iter, false) == false) {
				iatPointers.erase(iter, iatPointers.end());
				break;
			}
			else {
				lastPointer = *iter;
			}
		}
		else {
			lastPointer = *iter;
		}
	}

	if (iatPointers.empty()) {
		return;
	}

	bool erased = true;
	while (erased) {
		if (iatPointers.size() <= 1)
			break;

		iter = iatPointers.begin();
		lastPointer = *iter;
		iter++;

		for (; iter != iatPointers.end(); iter++) {
			if ((*iter - lastPointer) > 0x100) //check pointer difference, a typical difference is 4 on 32bit systems
			{
				bool isLastValid = IsIATPointerValid(lastPointer, false);
				bool isCurrentValid = IsIATPointerValid(*iter, false);
				if (isLastValid == false || isCurrentValid == false)
				{
					if (isLastValid == false)
					{
						iter--;
					}

					iatPointers.erase(iter);
					erased = true;
					break;
				}
				else
				{
					erased = false;
					lastPointer = *iter;
				}
			}
			else
			{
				erased = false;
				lastPointer = *iter;
			}
		}
	}

}

void IATSearcher::AdjustSizeForBigSections(DWORD* pBadValue) {
	if (*pBadValue > 0x5F5E100) {
		*pBadValue = 0x5F5E100;
	}
}

bool IATSearcher::IsSectionSizeTooBig(SIZE_T size) {
	return size > 100'000'000;
}

void IATSearcher::GetMemoryBaseAndSizeForIAT(DWORD_PTR address, DWORD_PTR* pBaseAddress, DWORD* pBaseSize) {
	MEMORY_BASIC_INFORMATION first = { 0 };
	MEMORY_BASIC_INFORMATION second = { 0 };
	MEMORY_BASIC_INFORMATION third = { 0 };

	DWORD_PTR start = 0, end = 0;
	*pBaseAddress = 0;
	*pBaseSize = 0;
	DWORD len = sizeof(MEMORY_BASIC_INFORMATION);
	if (!VirtualQueryEx(_hProcess, (LPCVOID)address, &second, len)) {
		return;
	}

	*pBaseAddress = (DWORD_PTR)second.BaseAddress;
	*pBaseSize = (DWORD)second.RegionSize;


	if (::VirtualQueryEx(_hProcess, (LPCVOID)((DWORD_PTR)second.BaseAddress - 1), &first, len)) {
		if (::VirtualQueryEx(_hProcess, (LPCVOID)((DWORD_PTR)second.BaseAddress + (DWORD_PTR)second.RegionSize),
			&third, len)) {
			if (third.State != MEM_COMMIT ||
				first.State != MEM_COMMIT ||
				third.Protect & PAGE_NOACCESS ||
				first.Protect & PAGE_NOACCESS) {
				return;
			}
			else {
				if (IsSectionSizeTooBig(first.RegionSize) ||
					IsSectionSizeTooBig(second.RegionSize) ||
					IsSectionSizeTooBig(third.RegionSize)) {
					return;
				}

				start = (DWORD_PTR)first.BaseAddress;
				end = (DWORD_PTR)third.BaseAddress + (DWORD_PTR)third.RegionSize;

				*pBaseAddress = start;
				*pBaseSize = (DWORD)(end - start);
			}
		}
	}
}