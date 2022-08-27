#pragma once


class Helpers {
public:
	static ULONG_PTR SearchSignature(ULONG_PTR address, PUCHAR signature, ULONG size,ULONG memSize);
	static NTSTATUS OpenProcess(ACCESS_MASK accessMask, ULONG pid, PHANDLE phProcess);
	// kdexts!KiDecodePointer
	static ULONG_PTR KiDecodePointer(_In_ ULONG_PTR pointer, _In_ ULONG_PTR salt);

	static inline ULONG_PTR KiWaitNever, KiWaitAlways;
};