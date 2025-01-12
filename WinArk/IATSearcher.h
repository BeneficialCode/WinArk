#pragma once

#include "ApiReader.h"
#include <set>

class IATSearcher : protected ApiReader {
public:
	DWORD_PTR _address;
	DWORD_PTR _size;

	bool SearchImportAddressTableInProcess(DWORD_PTR startAddress, DWORD_PTR* pIAT, DWORD* pSize, bool advance);

private:
	DWORD_PTR FindAPIAddressInIAT(DWORD_PTR startAddress);
	bool FindIATAdvanced(DWORD_PTR startAddress, DWORD_PTR* pIAT, DWORD* pSize);
	DWORD_PTR FindNextFunctionAddress();
	DWORD_PTR FindIATPointer();
	bool IsIATPointerValid(DWORD_PTR iatPointer, bool checkRedirects);

	bool FindIATStartAndSize(DWORD_PTR address, DWORD_PTR* pIAT, DWORD* pSize);

	DWORD_PTR FindIATStartAddress(DWORD_PTR baseAddress, DWORD_PTR startAddress, BYTE* pData);
	DWORD FindIATSize(DWORD_PTR baseAddress, DWORD_PTR iatAddress, BYTE* pData, DWORD dataSize);
	
	void FindIATPointers(std::set<DWORD_PTR>& iatPointers);
	void FindExecutableMemoryPagesByStartAddress(DWORD_PTR startAddress, DWORD_PTR* pBaseAddress, SIZE_T* pMemorySize);
	void FilterIATPointersList(std::set<DWORD_PTR>& iatPointers);
	void GetMemoryBaseAndSizeForIAT(DWORD_PTR address, DWORD_PTR* pBaseAddress, DWORD* pBaseSize);

	void AdjustSizeForBigSections(DWORD* pBadValue);
	bool IsSectionSizeTooBig(SIZE_T size);
};