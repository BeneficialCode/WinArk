#include "pch.h"
#include "Helpers.h"

// AntiRootkit!khook::GetSystemServiceTable
ULONG_PTR Helpers::SearchSignature(ULONG_PTR address, PUCHAR signature, ULONG size,ULONG memSize) {
	UCHAR code[256];
	int i = memSize;
    ULONG_PTR maxAddress = address + memSize - size;
    
	while (i--&&address < maxAddress) {
		memcpy(code, (void*)address, size);
		if (memcmp(signature, code, size) == 0) {
			return address;
		}
		address++;
	}
	return 0;
}