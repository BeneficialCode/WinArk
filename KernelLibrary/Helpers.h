#pragma once

class Helpers {
public:
	static ULONG_PTR SearchSignature(ULONG_PTR address, PUCHAR signature, ULONG size,ULONG kernelImageSize);
};