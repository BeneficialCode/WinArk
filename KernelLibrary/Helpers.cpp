#include "pch.h"
#include "Helpers.h"

ULONG_PTR Helpers::SearchSignature(ULONG_PTR address, PUCHAR signature, ULONG size,ULONG kernelImageSize) {
	UCHAR code[256];
	int i = kernelImageSize;
	while (i--) {
		memcpy(code, (void*)address, size);
		if (memcmp(signature, code, size) == 0) {
			return address;
		}
		address++;
	}
	return 0;
}