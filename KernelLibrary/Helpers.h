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

	static inline ULONG_PTR KiWaitNever, KiWaitAlways;
};

// Undocumented functions
////////////////////////////////
extern "C" {
	__declspec(dllimport) BOOLEAN PsIsProcessBeingDebugged(PEPROCESS Process);
	__declspec(dllimport) NTSTATUS ZwQueryInformationProcess(_In_ HANDLE ProcessHandle,
		_In_ PROCESSINFOCLASS ProcessInformationClass,
		_Out_ PVOID ProcessInformation,
		_In_ ULONG ProcessInformationLength,
		_Out_opt_ PULONG ReturnLength);
	
}
//////////////////////////////////