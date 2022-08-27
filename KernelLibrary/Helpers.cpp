#include "pch.h"
#include "Helpers.h"

// AntiRootkit!khook::GetSystemServiceTable
ULONG_PTR Helpers::SearchSignature(ULONG_PTR address, PUCHAR signature, ULONG size,ULONG memSize) {
	int i = memSize;
    ULONG_PTR maxAddress = address + memSize - size;
    
	while (i--&&address < maxAddress) {
		if (memcmp(signature, (void*)address, size) == 0) {
			return address;
		}
		address++;
	}
	return 0;
}

NTSTATUS Helpers::OpenProcess(ACCESS_MASK accessMask, ULONG pid, PHANDLE phProcess) {
	CLIENT_ID cid;
	cid.UniqueProcess = ULongToHandle(pid);
	cid.UniqueThread = nullptr;

	OBJECT_ATTRIBUTES procAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(nullptr, OBJ_KERNEL_HANDLE);
	return ZwOpenProcess(phProcess, accessMask, &procAttributes, &cid);
}

ULONG_PTR Helpers::KiDecodePointer(_In_ ULONG_PTR pointer, _In_ ULONG_PTR salt) {
	ULONG_PTR value = pointer;

#ifdef _WIN64
	value = RotateLeft64(value ^ KiWaitNever, KiWaitNever & 0xFF);
	value = RtlUlonglongByteSwap(value ^ salt)^ KiWaitAlways;
#else
	value = RotateLeft32(value ^ (ULONG)KiWaitNever, KiWaitNever & 0xFF);
	value = RtlUlongByteSwap(value ^ salt) ^ (ULONG)KiWaitAlways;
#endif // _WIN64
	return value;
}