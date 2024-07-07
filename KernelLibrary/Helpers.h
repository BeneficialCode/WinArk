#pragma once
#include "Common.h"

// Helper functions

class Helpers {
public:
	static ULONG_PTR SearchSignature(ULONG_PTR address, PUCHAR signature, ULONG size,ULONG memSize);
	static NTSTATUS OpenProcess(ACCESS_MASK accessMask, ULONG pid, PHANDLE phProcess);
	// kdexts!KiDecodePointer
	static ULONG_PTR KiDecodePointer(_In_ ULONG_PTR pointer, _In_ ULONG_PTR salt);

	static bool IsSuffixedUnicodeString(PCUNICODE_STRING fullName, PCUNICODE_STRING shortName, bool caseInsensitive = true);

	static bool IsMappedByLdrLoadDll(PCUNICODE_STRING shortName);

	static bool IsSpecificProcess(HANDLE pid, const WCHAR* imageName, bool isDebugged);

	static UINT FindStringByGuid(PVOID baseAddress, UINT size, const GUID* guid);

	static NTSTATUS DumpSysModule(DumpSysData* pData);

	static NTSTATUS DumpKernelMem(DumpMemData* pData, PVOID pInfo);

	static ULONG64 ReadBitField(PUCHAR readAddr, BitField* pBitField);
	static ULONG64 ReadFieldValue(PUCHAR readAddr, ULONG readSize);

	static NTSTATUS SearchPattern(PUCHAR pPattern, UCHAR wildcard, ULONG_PTR len, PVOID pBase,
		ULONG_PTR size, PVOID* ppFound);

	static NTSTATUS ReadKernelValue64(ULONG_PTR addr, PULONG_PTR pValue);

	static NTSTATUS ReadKernelValue32(ULONG_PTR addr, PLONG pValue);

	static NTSTATUS MmIsKernelAddressValid(
		_In_ PVOID VirtualAddress,
		_In_ ULONG Size
	);

	static NTSTATUS KillProcess(ULONG pid);

	static inline ULONG_PTR KiWaitNever, KiWaitAlways;
};

// Undocumented functions
////////////////////////////////
extern "C" {
	__declspec(dllimport) BOOLEAN PsIsProcessBeingDebugged(PEPROCESS Process);
	__declspec(dllimport) NTSTATUS NTAPI ZwQueryInformationProcess(
		_In_ HANDLE ProcessHandle,
		_In_ PROCESSINFOCLASS ProcessInformationClass,
		_Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
		_In_ ULONG ProcessInformationLength,
		_Out_opt_ PULONG ReturnLength
	);
	
}
//////////////////////////////////