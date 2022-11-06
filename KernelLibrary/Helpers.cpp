#include "pch.h"
#include "Helpers.h"
#include "Logging.h"

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

// Check if fullName is ends with ShortName
bool Helpers::IsSuffixedUnicodeString(PCUNICODE_STRING fullName, PCUNICODE_STRING shortName, bool caseInsensitive) {
	if (fullName && shortName &&
		shortName->Length <= fullName->Length) {
		UNICODE_STRING ustr = {
			shortName->Length,
			ustr.Length,
			(PWSTR)RtlOffsetToPointer(fullName->Buffer,fullName->Length - ustr.Length)
		};
		return RtlEqualUnicodeString(&ustr, shortName, caseInsensitive);
	}
	return false;
}

bool Helpers::IsMappedByLdrLoadDll(PCUNICODE_STRING shortName) {
	// smss.exe can map kernel32.dll during creation of \\KnownDlls (in that  case ArbitrayUserPointer will be 0)
	// Wow64 processes map kernel32.dll serverals times (32 and 64-bit version) with WOW64_IMAGE_SECTION or
	// NOT_AN_IMAGE

	UNICODE_STRING name;
	__try {
		PNT_TIB Teb = (PNT_TIB)PsGetCurrentThreadTeb();
		if (!Teb || !Teb->ArbitraryUserPointer) {
			// This is not it
			return false;
		}
		name.Buffer = (PWSTR)Teb->ArbitraryUserPointer;

		// Check that we hava a valid user-mode address
		ProbeForRead(name.Buffer, sizeof(WCHAR), __alignof(WCHAR));

		// Check buffer length
		name.Length = (USHORT)wcsnlen_s(name.Buffer, MAXSHORT);

		if (name.Length == MAXSHORT) {
			// name is too long
			return false;
		}

		name.Length *= sizeof(WCHAR);
		name.MaximumLength = name.Length;

		// See if it's our needed module
		return IsSuffixedUnicodeString(&name, shortName);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		LogDebug("#EXCEPTION: (0x%X) IsMappedByLdrLoadDll", GetExceptionCode());
	}

	return false;
}

// Checks if process with pid is a specific process by its file name
bool Helpers::IsSpecificProcess(HANDLE pid,const WCHAR* imageName,bool isDebugged) {
	ASSERT(imageName);
	bool result = false;

	PEPROCESS Process;
	NTSTATUS status = PsLookupProcessByProcessId(pid, &Process);
	if (NT_SUCCESS(status)) {
		// Check for kernel debugger?
		if (!isDebugged ||
			PsIsProcessBeingDebugged(Process)) {
			// Get process handle
			HANDLE hProcess;
			status = ObOpenObjectByPointer(Process, OBJ_KERNEL_HANDLE, nullptr, 
				PROCESS_ALL_ACCESS, *PsProcessType, KernelMode, &hProcess);
			if (NT_SUCCESS(status)) {
				// Get process name
				// INFO: we need for file name, thus can't use PsGetProcessImageFileName which will truncate it past 14 chars
				UCHAR buffer[280] = { 0 };
				status = ZwQueryInformationProcess(hProcess, ProcessImageFileName, buffer, sizeof(buffer) - sizeof(WCHAR), nullptr);
				if (NT_SUCCESS(status)) {
					auto path = (UNICODE_STRING*)buffer;
					auto bs = wcsrchr(path->Buffer, L'\\');
					NT_ASSERT(bs);
					if (bs + 1 != nullptr && !_wcsicmp(bs + 1, imageName))
						result = true;
				}
				// Close handle
				ZwClose(hProcess);
			}
		}

		// Dereference process object back
		ObDereferenceObject(Process);
	}


	return result;
}

UINT Helpers::FindStringByGuid(PVOID baseAddress, UINT size, const GUID* guid) {
	union
	{
		const BYTE* pS;
		const GUID* pG;
	};

	pS = (const BYTE*)baseAddress;
	for (const BYTE* pE = pS + size - sizeof(GUID); pS <= pE; pS++)
	{
		if (memcmp(pG, guid, sizeof(GUID)) == 0)
		{
			//Matched!
			return (UINT)(pS + sizeof(GUID) - (const BYTE*)baseAddress);
		}
	}

	return (UINT)-1;
}