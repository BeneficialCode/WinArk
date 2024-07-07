#include "pch.h"
#include "khook.h"
#include "Memory.h"
#include "Helpers.h"
#include "PEParser.h"
#include <intrin.h>
#include "Logging.h"
#include "Zydis/Zydis.h"
#include "reflector.h"
#include "hal.h"

SystemServiceTable* khook::_ntTable = nullptr;
SystemServiceTable* khook::_win32kTable = nullptr;
PVOID khook::_kernelImageBase = nullptr;
ULONG khook::_kernelImageSize = 0;
PVOID khook::_win32kImageBase = nullptr;
ULONG khook::_win32kImageSize = 0;
PVOID khook::_sectionStart = nullptr;
ULONG khook::_sectionSize = 0;
PVOID khook::_shadowSectionStart = nullptr;
ULONG khook::_shadowSectionSize = 0;
LinkedList<ShadowServiceNameEntry> khook::_shadowServiceNameList;
HANDLE khook::_pid = nullptr;


NTSTATUS khook::RtlSuperCopyMemory(_In_ VOID UNALIGNED* Destination, _In_ VOID UNALIGNED* Source, _In_ ULONG Length) {
	KIRQL oldIrql;
	KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);

	PMDL mdl = IoAllocateMdl(Destination, Length, FALSE, FALSE, nullptr);
	if (mdl == nullptr) {
		KeLowerIrql(oldIrql);
		return STATUS_NO_MEMORY;
	}

	MmBuildMdlForNonPagedPool(mdl);
	// Hack: prevent bugcheck from Driver Verifier and possible future version of Windows
#pragma prefast(push)
	// Disables the warnings specified in a given warning list.
#pragma prefast(disable:__WARNING_MODIFYING_MDL,"Trust me I'm a scientist")	
	CSHORT flags = mdl->MdlFlags;
	mdl->MdlFlags |= MDL_PAGES_LOCKED;
	mdl->MdlFlags &= ~MDL_SOURCE_IS_NONPAGED_POOL;

	// Map pages and do the copy
	PVOID mapped = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmCached, nullptr, FALSE, HighPagePriority);
	if (mapped == nullptr) {
		mdl->MdlFlags = flags;
		IoFreeMdl(mdl);
		KeLowerIrql(oldIrql);
		return STATUS_NONE_MAPPED;
	}

	RtlCopyMemory(mapped, Source, Length);

	MmUnmapLockedPages(mapped, mdl);
	mdl->MdlFlags = flags;
#pragma prefast(pop)

	IoFreeMdl(mdl);
	KeLowerIrql(oldIrql);

	return STATUS_SUCCESS;
}

PVOID khook::GetApiAddress(PCWSTR name) {
	UNICODE_STRING uname;
	RtlInitUnicodeString(&uname, name);
	return MmGetSystemRoutineAddress(&uname);
}

bool khook::Hook(PVOID api, void* newfunc) {
	ULONG_PTR addr = (ULONG_PTR)api;
	if (!addr)
		return false;
	// allocate structure
	_info = new (NonPagedPool) HookInfo;
	if (!_info) {
		return false;
	}
	// set hooking address
	_info->Address = addr;
	// set hooking opcode
#ifdef _WIN64
	_info->Opcodes.mov = 0xB848;
#else
	_info->Opcodes.mov = 0xB8;
#endif // _WIN64
	_info->Opcodes.addr = (ULONG_PTR)newfunc;
	_info->Opcodes.push = 0x50;
	_info->Opcodes.ret = 0xc3;

	// set original data
	RtlCopyMemory(&_info->Original, (const void*)addr, sizeof(OpcodesInfo));
	
	if (!NT_SUCCESS(RtlSuperCopyMemory((void*)addr, &_info->Opcodes, sizeof(OpcodesInfo)))) {
		delete _info;
		return false;
	}

	return true;
}

bool khook::Unhook() {
	if (!_info || !_info->Address)
		return false;
	if (!NT_SUCCESS(RtlSuperCopyMemory((void*)_info->Address, _info->Original, sizeof(OpcodesInfo)))) {
		return false;
	}
	delete _info;
	return true;
}

bool khook::GetShadowSystemServiceTable() {
	if (!_win32kTable) {
#ifndef _WIN64
		// x86 code
		UNICODE_STRING name = RTL_CONSTANT_STRING(L"KeServiceDescriptorTable");
		_ntTable = (SystemServiceTable*)MmGetSystemRoutineAddress(&name);
		_win32kTable = (SystemServiceTable*)((ULONG_PTR)_ntTable + 0x50);
#else
		// x64 code
		PEParser parser(_kernelImageBase);
		int count = parser.GetSectionCount();
		for (int i = 0; i < count; i++) {
			auto pSec = parser.GetSectionHeader(i);
			if (pSec->Characteristics & IMAGE_SCN_MEM_NOT_PAGED &&
				pSec->Characteristics & IMAGE_SCN_MEM_EXECUTE &&
				!(pSec->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) &&
				(*(PULONG)pSec->Name != 'TINI') &&
				(*(PULONG)pSec->Name != 'EGAP')) {
				ULONG_PTR searchAddr = (ULONG_PTR)((PUCHAR)_kernelImageBase + pSec->VirtualAddress);
				ULONG_PTR maxAddress = searchAddr + pSec->Misc.VirtualSize;
				// Find KiSystemServiceStart
				UCHAR pattern[] = { 0x8B, 0xF8, 0xC1, 0xEF, 0x07, 0x83, 0xE7, 0x20, 0x25, 0xFF, 0x0F, 0x00, 0x00 };
				ULONG_PTR label = Helpers::SearchSignature(searchAddr, pattern, sizeof(pattern), pSec->Misc.VirtualSize);
				if (label == 0)
					return false;
				searchAddr = label + sizeof(pattern);
				UCHAR signature[] = { 0x4c,0x8d,0x1d };
				ULONG size = maxAddress - searchAddr;
				ULONG_PTR addr = Helpers::SearchSignature(searchAddr, signature, sizeof(signature), size);
				if (addr == 0)
					return false;
				ULONG_PTR offsetAddr = addr + sizeof(signature);
				ULONG_PTR offset = 0;
				memcpy(&offset, (void*)offsetAddr, 4);
				if (offset == 0) {
					return false;
				}
				_win32kTable = (SystemServiceTable*)(addr + offset + 7) + 1;
			}
		}
#endif // 
	}

	return true;
}


bool khook::GetSystemServiceTable() {
	if (!_ntTable) {
#ifndef _WIN64
		// x86 code
		UNICODE_STRING name = RTL_CONSTANT_STRING(L"KeServiceDescriptorTable");
		_ntTable = (SystemServiceTable*)MmGetSystemRoutineAddress(&name);
#else
		// x64 code
		PEParser parser(_kernelImageBase);
		int count = parser.GetSectionCount();
		for (int i = 0; i < count; i++) {
			auto pSec = parser.GetSectionHeader(i);
			if (pSec->Characteristics & IMAGE_SCN_MEM_NOT_PAGED &&
				pSec->Characteristics & IMAGE_SCN_MEM_EXECUTE &&
				!(pSec->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) &&
				(*(PULONG)pSec->Name != 'TINI') &&
				(*(PULONG)pSec->Name != 'EGAP')) {
				ULONG_PTR searchAddr = (ULONG_PTR)((PUCHAR)_kernelImageBase + pSec->VirtualAddress);
				ULONG_PTR maxAddress = searchAddr + pSec->Misc.VirtualSize;
				UCHAR pattern[] = { 0x8B, 0xF8, 0xC1, 0xEF, 0x07, 0x83, 0xE7, 0x20, 0x25, 0xFF, 0x0F, 0x00, 0x00 };
				ULONG_PTR label = Helpers::SearchSignature(searchAddr,pattern, sizeof(pattern), pSec->Misc.VirtualSize);
				if (label == 0)
					return false;
				searchAddr = label + sizeof(pattern);
				UCHAR signature[] = { 0x4c,0x8d,0x15 };
				// IA32_LSTAR 0xC0000082H
				// 微软补丁，打了个向上的跳转，导致搜索不到，所以__readmsr不好使了。
				ULONG size = maxAddress - searchAddr;
				ULONG_PTR addr = Helpers::SearchSignature(searchAddr, signature, sizeof(signature), size);
				if (addr == 0)
					return false;
				ULONG_PTR offsetAddr = addr + sizeof(signature);
				ULONG_PTR offset = 0;
				memcpy(&offset, (void*)offsetAddr, 4);
				if (offset == 0) {
					return false;
				}
				// 7为指令长度
				_ntTable = (SystemServiceTable*)(addr + offset + 7);
			}
		}
#endif // !_WIN64
	}
	return true;
}

bool khook::GetShadowSystemServiceNumber(PUNICODE_STRING symbolName, PULONG index) {
	PLIST_ENTRY head = _shadowServiceNameList.GetHead();

	ShadowServiceNameEntry* item = _shadowServiceNameList.GetHeadItem();
	if (&item->Entry == head) {
		return false;
	}

	PLIST_ENTRY entry = nullptr;
	do 	{
		entry = item->Entry.Flink;  // 后继
		item = CONTAINING_RECORD(entry, ShadowServiceNameEntry, Entry);
		if (0 == RtlCompareUnicodeString(symbolName, &item->Name, false)) {
			*index = item->Index;
			return true;
		}
	} while (entry->Blink!=entry->Flink); // 头结点

	return false;
}

bool khook::GetSystemServiceNumber(const char* exportName,PULONG index) {
	PEParser parser(L"\\SystemRoot\\system32\\ntdll.dll");
	ULONG offset = parser.GetExportByName(exportName);
	if (offset != 0) {
		// get system service number
		unsigned char* exportData = (unsigned char*)parser.GetBaseAddress() + offset;
		for (int i = 0; i < 16; i++) {
			if (exportData[i] == 0xC2 || exportData[i] == 0xC3)  //RET
				break;
			if (exportData[i] == 0xB8)  //mov eax,X
			{
				ULONG* address = (ULONG*)(exportData + i + 1);
				*index = *address;
				address += 1;
				auto opcode = *(unsigned short*)address;
				unsigned char* p = (unsigned char*)address + 5;
				auto code = *(unsigned char*)p;
				if (code != 0xFF &&// x86
					opcode != 0x04F6  // win10 x64
					&& opcode != 0x050F) { // win7 x64
					break;
				}
				return true;
			}
		}
	}

	return false;
}

bool khook::GetSectionStart(ULONG_PTR va) {
	ULONG rva = va - (ULONG_PTR)_kernelImageBase;
	if (!_kernelImageBase) {
		return false;
	}
	IMAGE_DOS_HEADER* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(_kernelImageBase);
	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		return false;
	}
	else {
		auto ntHeader = (PIMAGE_NT_HEADERS)((PUCHAR)_kernelImageBase + dosHeader->e_lfanew);
		if (ntHeader->Signature != IMAGE_NT_SIGNATURE) {
			return false;
		}
		auto section = PEParser::RvaToSection(ntHeader, _kernelImageBase, rva);
		if (!section) {
			return false;
		}
		_sectionStart = (PVOID)((unsigned char*)_kernelImageBase + section->VirtualAddress);
		_sectionSize = section->SizeOfRawData;
		return true;
	}
}

bool khook::GetShadowSectionStart(ULONG_PTR va) {
	ULONG rva = va - (ULONG_PTR)_win32kImageBase;
	if (!_win32kImageBase) {
		return false;
	}
	IMAGE_DOS_HEADER* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(_win32kImageBase);
	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		return false;
	}
	else {
		auto ntHeader = (PIMAGE_NT_HEADERS)((PUCHAR)_win32kImageBase + dosHeader->e_lfanew);
		if (ntHeader->Signature != IMAGE_NT_SIGNATURE) {
			return false;
		}
		auto section = PEParser::RvaToSection(ntHeader, _win32kImageBase, rva);
		if (!section) {
			return false;
		}
		_shadowSectionStart = (PVOID)((unsigned char*)_win32kImageBase + section->VirtualAddress);
		_shadowSectionSize = section->SizeOfRawData;
		return true;
	}
}


PVOID khook::FindCaveAddress(PVOID start, ULONG size, ULONG caveSize) {
	unsigned char* opcode = (unsigned char*)start;

	for (unsigned int i = 0, j = 0; i < size; i++) {
		if (opcode[i] == 0x90 || opcode[i] == 0xCC) // NOP or INT3
			j++;
		else
			j = 0;
		if (j == caveSize)
			return (PVOID)((ULONG_PTR)start + i - caveSize + 1);
	}

	return nullptr;
}

bool khook::HookSSDT(const char* apiName, void* newfunc) {
	bool success = GetKernelAndWin32kBase();
	if (!success) {
		return false;
	}
	
	success = GetSystemServiceTable();
	if (!success) {
		return false;
	}
	

	ULONG_PTR base = (ULONG_PTR)_ntTable->ServiceTableBase;
	if (!base) {
		return false;
	}
	ULONG index = 0;
	success = GetSystemServiceNumber(apiName, &index);
	if (success) {
		if (index >= _ntTable->NumberOfServices) {
			return false;
		}

		LONG oldValue = _ntTable->ServiceTableBase[index];
		LONG newValue = 0;

#ifdef _WIN64
		/*
		x64 SSDT Hook;
		1) find API addr
		2) get code page+size
		3) find cave address
		4) hook cave address
		5) change SSDT value
		*/
		if (!_sectionStart) {
			ULONG_PTR lowest = base;
			ULONG_PTR highest = lowest + 0x0FFFFFFF;
			ULONG_PTR address = (oldValue >> 4) + base;
			success = GetSectionStart(address);
			if (!success) {
				return false;
			}
			if (!_sectionStart || !_sectionSize) {
				return false;
			}
			if ((ULONG_PTR)_sectionStart < lowest) {
				_sectionSize -= (ULONG)(lowest - (ULONG_PTR)_sectionStart);
				_sectionStart = (PVOID)lowest;
			}
		}

		PVOID caveAddress = FindCaveAddress(_sectionStart, _sectionSize, sizeof(OpcodesInfo));
		if (!caveAddress) {
			return false;
		}
		
		success = Hook(caveAddress, newfunc);
		if (!success)
			return false;

		newValue = (LONG)((ULONG_PTR)caveAddress - base);
		newValue = (newValue << 4) | oldValue & 0xF;

		_info->Index = index;
		_info->Old = oldValue;
		_info->New = newValue;
		// The lower 4 bits store the number of arguments passed on the stack to the system call
		_info->OriginalAddress = (oldValue >> 4) + base;
#else
		newValue = (ULONG)newfunc;

		_info = new (NonPagedPool) HookInfo;

		_info->Index = index;
		_info->Old = oldValue;
		_info->New = newValue;
		_info->OriginalAddress = oldValue;

#endif // _WIN64
		RtlSuperCopyMemory(&_ntTable->ServiceTableBase[index], &newValue, sizeof(newValue));

		_success = true;
		return true;
	}

	return false;
}

bool khook::UnhookSSDT() {
	RtlSuperCopyMemory(&_ntTable->ServiceTableBase[_info->Index], &_info->Old, sizeof(_info->Old));

#ifdef _WIN64
	bool success = Unhook();
	if (!success)
		return false;
#else
	delete _info;
#endif // _WIN64

	return true;
}

bool khook::UnhookShadowSSDT() {
	// Get the process id of the "winlogon.exe" process
	bool success = SearchSessionProcess();
	if (!success)
		return nullptr;

	PEPROCESS Process;
	NTSTATUS status = PsLookupProcessByProcessId(_pid, &Process);
	if (!NT_SUCCESS(status))
		return nullptr;

	KAPC_STATE apcState;
	KeStackAttachProcess(Process, &apcState);

	RtlSuperCopyMemory(&_win32kTable->ServiceTableBase[_info->Index], &_info->Old, sizeof(_info->Old));

	do 	{
#ifdef _WIN64
		success = Unhook();
		if (!success)
			break;
#else
		delete _info;
#endif // _WIN64
	} while (false);

	KeUnstackDetachProcess(&apcState);
	ObDereferenceObject(Process);

	return success;
}


bool khook::GetKernelAndWin32kBase() {
	if (_kernelImageBase && _win32kImageBase) {
		return true;
	}

	void* buffer = nullptr;
	ULONG size = 1 << 18;

	buffer = ExAllocatePool(NonPagedPool, size);
	if (!buffer) {
		return false;
	}
	NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, buffer, size, nullptr);
	if (!NT_SUCCESS(status)) {
		ExFreePool(buffer);
		return false;
	}

	auto info = (RTL_PROCESS_MODULES*)buffer;

	_kernelImageBase = info->Modules[0].ImageBase;
	_kernelImageSize = info->Modules[0].ImageSize;

	UNICODE_STRING win32kPath = RTL_CONSTANT_STRING(L"\\SystemRoot\\system32\\win32k.sys");
	UNICODE_STRING moduleName;
	auto entry = info->Modules;
	ANSI_STRING ansiModuleName;
	for(;;) {
		if (entry->ImageBase == 0) {
			break;
		}
		RtlInitAnsiString(&ansiModuleName, reinterpret_cast<PCSZ>(entry->FullPathName));
		RtlAnsiStringToUnicodeString(&moduleName, &ansiModuleName, TRUE);
		if (RtlCompareUnicodeString(&win32kPath, &moduleName, TRUE) == 0) {
			_win32kImageBase = entry->ImageBase;
			_win32kImageSize = entry->ImageSize;
			RtlFreeUnicodeString(&moduleName);
			break;
		}
		RtlFreeUnicodeString(&moduleName);

		entry += 1;
		if (entry == nullptr)
			break;
	}

	ExFreePool(buffer);
	return true;
}

PVOID khook::GetOriginalFunction(HookType type,const char* apiName,PUNICODE_STRING symbolName) {
	ULONG_PTR address = 0;
	switch (type) 	{
		case HookType::SSDT:
		{
			bool success = GetKernelAndWin32kBase();
			if (!success) {
				return nullptr;
			}

			success = GetSystemServiceTable();
			if (!success) {
				return nullptr;
			}
			

			ULONG_PTR base = (ULONG_PTR)_ntTable->ServiceTableBase;
			if (!base) {
				return nullptr;
			}
			ULONG index = 0;
			success = GetSystemServiceNumber(apiName, &index);
			if (success) {
				if (index >= _ntTable->NumberOfServices) {
					return nullptr;
				}
				LONG oldValue = _ntTable->ServiceTableBase[index];
#ifdef _WIN64
				address = (oldValue >> 4) + base;
#else
				address = oldValue;
#endif
			}
		}
			break;
		case HookType::ShadowSSDT:
		{
			bool success = GetKernelAndWin32kBase();
			if (!success) {
				return nullptr;
			}

			success = GetShadowSystemServiceTable();
			if (!success)
				return nullptr;

			

			// Get the process id of the "winlogon.exe" process
			success = SearchSessionProcess();
			if (!success)
				return nullptr;

			PEPROCESS Process;
			NTSTATUS status = PsLookupProcessByProcessId(_pid, &Process);
			if (!NT_SUCCESS(status))
				return nullptr;

			KAPC_STATE apcState;
			KeStackAttachProcess(Process, &apcState);

			do 	{
				ULONG_PTR base = (ULONG_PTR)_win32kTable->ServiceTableBase;
				if (!base) {
					break;
				}
				ULONG index;
				success = GetShadowSystemServiceNumber(symbolName, &index);
				if (success) {
					if (index > _win32kTable->NumberOfServices) {
						break;
					}

					LONG oldValue = _win32kTable->ServiceTableBase[index];
#ifdef _WIN64
					address = (oldValue >> 4) + base;
#else
					address = oldValue;
#endif // __WIN64
				}
			} while (false);

			KeUnstackDetachProcess(&apcState);
			ObDereferenceObject(Process);
		}
			break;
		default:
			break;
	}
	return (PVOID)address;
}

bool khook::SearchSessionProcess() {
	if (!_pid) {
		ULONG len = 0;
		NTSTATUS status = ZwQuerySystemInformation(SystemProcessInformation, nullptr, 0, &len);
		if (status != STATUS_INFO_LENGTH_MISMATCH) {
			return false;
		}
		ULONG size = len * 2;
		void* buffer = ExAllocatePool(NonPagedPool, size);
		if (!buffer) {
			return false;
		}

		status = ZwQuerySystemInformation(SystemProcessInformation, buffer, size, &len);
		if (!NT_SUCCESS(status)) {
			ExFreePool(buffer);
			return false;
		}

		auto info = (SYSTEM_PROCESS_INFORMATION*)buffer;
		UNICODE_STRING targetProcName = RTL_CONSTANT_STRING(L"winlogon.exe");

		for (;;) {
			if (info->NextEntryOffset == 0)
				break;

			//KdPrint(("ImageName: %wZ,SessionId: %d\n", &info->ImageName,info->SessionId));
			if (!RtlCompareUnicodeString(&targetProcName, &info->ImageName, TRUE)) {
				_pid = info->UniqueProcessId;
				break;
			}


			info = (SYSTEM_PROCESS_INFORMATION*)((PUCHAR)info + info->NextEntryOffset);
			if (info == nullptr)
				break;
		}

		ExFreePool(buffer);
		return _pid == nullptr ? false : true;
	}
	return true;
}

/*
ed nt!Kd_SXS_Mask 0
ed nt!Kd_FUSION_Mask 0
*/

bool khook::HookShadowSSDT(PUNICODE_STRING apiName, void* newfunc) {
	bool success = GetKernelAndWin32kBase();
	if (!success) {
		return false;
	}
	
	success = GetShadowSystemServiceTable();
	if (!success)
		return false;

	// Get the process id of the "winlogon.exe" process
	success = SearchSessionProcess();
	if (!success)
		return false;

	PEPROCESS Process;
	NTSTATUS status = PsLookupProcessByProcessId(_pid, &Process);
	if (!NT_SUCCESS(status))
		return false;

	// 此处必须配对
	KAPC_STATE apcState;
	KeStackAttachProcess(Process, &apcState);

	do {
		ULONG_PTR base = (ULONG_PTR)_win32kTable->ServiceTableBase;
		if (!base) {
			success = false;
			break;
		}
		ULONG index = 0;
		success = GetShadowSystemServiceNumber(apiName, &index);
		if (success) {
			if (index > _win32kTable->NumberOfServices) {
				success = false;
				break;
			}

			LONG oldValue = _win32kTable->ServiceTableBase[index];
			LONG newValue = 0;

#ifdef _WIN64
			/*
			x64 shadow ssdt hook
			1) find API addr
			2) get code page+size
			3) find cave address
			4) hook cave address
			5) change Shadow SSDT value
			*/
			if (!_shadowSectionStart) {
				ULONG_PTR lowest = base;
				ULONG_PTR highest = lowest + 0x0FFFFFFF;
				ULONG_PTR address = (oldValue >> 4) + base;
				success = GetShadowSectionStart(address);
				if (!success) {
					break;
				}
				if (!_shadowSectionStart || !_shadowSectionSize) {
					success = false;
					break;
				}
				if ((ULONG_PTR)_shadowSectionStart < lowest) {
					_shadowSectionSize -= (ULONG)(lowest - (ULONG_PTR)_sectionStart);
					_shadowSectionStart = (PVOID)lowest;
				}
			}

			PVOID caveAddress = FindCaveAddress(_shadowSectionStart, _shadowSectionSize, sizeof(OpcodesInfo));
			if (!caveAddress) {
				success = false;
				break;
			}
			KdPrint(("caveAddress: 0x%p", caveAddress));
			success = Hook(caveAddress, newfunc);
			if (!success)
				break;

			newValue = (LONG)((ULONG_PTR)caveAddress - base);
			newValue = (newValue << 4) | oldValue & 0xF;

			_info->Index = index;
			_info->Old = oldValue;
			_info->New = newValue;
			_info->OriginalAddress = (oldValue >> 4) + base;
#else
			newValue = (ULONG)newfunc;

			_info = new (NonPagedPool) HookInfo;

			_info->Index = index;
			_info->Old = oldValue;
			_info->New = newValue;
			_info->OriginalAddress = oldValue;

#endif // _WIN64
			RtlSuperCopyMemory(&_win32kTable->ServiceTableBase[index], &newValue, sizeof(newValue));

			success = true;
		}
	} while (false);

	KeUnstackDetachProcess(&apcState);
	ObDereferenceObject(Process);
	_success = success;
	return success ? true : false;
}

bool khook::HookKernelApi(PVOID api, void* newfunc,bool first) {
	if (first) {
		ULONG_PTR addr = (ULONG_PTR)newfunc;
		if (!addr)
			return false;
		// allocate structure
		_inlineInfo = new (NonPagedPool)InlineHookInfo;
		if (!_inlineInfo) {
			return false;
		}
		// set hooking address
		_inlineInfo->Address = (ULONG_PTR)api;
		_inlineInfo->Opcodes.lowAddr = addr & 0xFFFFFFFF;
#ifdef _WIN64
		_inlineInfo->Opcodes.highAddr = (addr >> 32) & 0xFFFFFFFF;
#endif

		// set original data
		RtlCopyMemory(&_inlineInfo->Original, api, sizeof(InlineOpcodesInfo));
	}
	
	NTSTATUS status = SecureExchange(&_inlineInfo->Opcodes);
	if (!NT_SUCCESS(status)) {
		delete _inlineInfo;
		return false;
	}
	_success = true;
	return true;
}

NTSTATUS khook::SecureExchange(PVOID opcodes) {
	PVOID address = reinterpret_cast<PVOID>(_inlineInfo->Address);
	ULONG length = sizeof(InlineOpcodesInfo);
	// Check for proper alignment, cmpxchg16b works only with 16-byte aligned address.
	if ((ULONG_PTR)address != ((ULONG_PTR)address & ~0xf)) {
		return STATUS_DATATYPE_MISALIGNMENT;
	}

	// Create memory descriptor list to map read-only (or RX) memory as read-write.
	PMDL mdl = IoAllocateMdl(address, length, FALSE, FALSE, nullptr);
	if (mdl == nullptr) {
		return STATUS_NO_MEMORY;
	}

	// Make memory pages resident in RAM and make sure they won't get paged out.
	__try {
		MmProbeAndLockPages(mdl, KernelMode, IoReadAccess);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		IoFreeMdl(mdl);
		return STATUS_INVALID_ADDRESS;
	}

	// Create new mapping for read-only memory
	PVOID mapped = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmNonCached, nullptr, FALSE, NormalPagePriority);
	if (mapped == nullptr) {
		MmUnlockPages(mdl);
		IoFreeMdl(mdl);

		return STATUS_NONE_MAPPED;
	}

	// Set new mapped page protection to read-write in order to modify it.
	NTSTATUS status = MmProtectMdlSystemAddress(mdl, PAGE_READWRITE);
	if (!NT_SUCCESS(status)) {
		MmUnmapLockedPages(mapped, mdl);
		MmUnlockPages(mdl);
		IoFreeMdl(mdl);

		return status;
	}

#ifdef _WIN64
	PLONG64 dest = reinterpret_cast<PLONG64>(mapped);
	LONG64 value[2];
	value[0] = dest[0];
	value[1] = dest[1];

	InterlockedCompareExchange128(dest, ((PLONG64)opcodes)[1], ((PLONG64)opcodes)[0], (LONG64*)value);
#else
	PLONG dest = reinterpret_cast<PLONG>(mapped);
	InterlockedExchange(dest, *(LONG*)opcodes);
#endif // _WIN64

	// Unlock and unmap pages, free MDL
	MmUnmapLockedPages(mapped, mdl);
	MmUnlockPages(mdl);
	IoFreeMdl(mdl);

	return STATUS_SUCCESS;
}

bool khook::UnhookKernelApi(bool end) {
	if (!_inlineInfo || !_inlineInfo->Address)
		return false;
	NTSTATUS status = SecureExchange(&_inlineInfo->Original);
	if (!NT_SUCCESS(status)) {
		return false;
	}
	if(end)
		delete _inlineInfo;
	return true;
}

bool khook::GetApiAddress(ULONG index,PVOID* address) {
	bool success = GetSystemServiceTable();
	if (!success) {
		return false;
	}

	ULONG_PTR base = (ULONG_PTR)_ntTable->ServiceTableBase;
	if (!base) {
		return false;
	}
	if (success) {
		if (index >= _ntTable->NumberOfServices) {
			return false;
		}

		LONG oldValue = _ntTable->ServiceTableBase[index];
#ifdef _WIN64
		*address = (PVOID)((oldValue >> 4) + base);
#else
		*address = (PVOID)oldValue;
#endif // _WIN64
		return true;
	}

	return false;
}

bool khook::GetShadowApiAddress(ULONG index, PVOID* address) {
	bool success = GetShadowSystemServiceTable();
	if (!success)
		return false;

	// Get the process id of the "winlogon.exe" process
	success = SearchSessionProcess();
	if (!success)
		return false;

	PEPROCESS Process;
	NTSTATUS status = PsLookupProcessByProcessId(_pid, &Process);
	if (!NT_SUCCESS(status))
		return false;

	// 此处必须配对
	KAPC_STATE apcState;
	KeStackAttachProcess(Process, &apcState);
	do
	{
		ULONG_PTR base = (ULONG_PTR)_win32kTable->ServiceTableBase;
		if (!base) {
			success = false;
			break;
		}
		if (success) {
			if (index > _win32kTable->NumberOfServices) {
				success = false;
				break;
			}

			LONG oldValue = _win32kTable->ServiceTableBase[index];
#ifdef _WIN64
			*address = (PVOID)((oldValue >> 4) + base);
#else
			*address = (PVOID)oldValue;
#endif // _WIN64

			success = true;
		}
	} while (false);

	KeUnstackDetachProcess(&apcState);
	ObDereferenceObject(Process);
	return success ? true : false;
}

ULONG khook::GetShadowServiceLimit() {
	if (!_win32kTable)
		return 0;

	return _win32kTable->NumberOfServices;
}

void khook::DetectInlineHook(KInlineData* pInfo,KernelInlineHookData* pData) {
	PVOID imageBase = (PVOID)pInfo->ImageBase;
	ULONG desiredCount = pInfo->Count;

	bool success = SearchSessionProcess();
	if (!success)
		return;

	PEPROCESS Process;
	NTSTATUS status = PsLookupProcessByProcessId(_pid, &Process);
	if (!NT_SUCCESS(status))
		return;

	KAPC_STATE apcState;
	KeStackAttachProcess(Process, &apcState);

#ifndef _WIN64
	// x86 code
	
#else
	// x64 code
	PEParser parser(imageBase);
	int count = parser.GetSectionCount();
	ULONG totalCount = 0;

	// Initialize Zydis decoder and formatter
	ZydisDecoder decoder;
	if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64)))
		return;

	ZydisFormatter formatter;
	if (!ZYAN_SUCCESS(ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL)))
		return;

	SIZE_T readOffset = 0;
	ZydisDecodedInstruction instruction;
	ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
	CHAR printBuffer[128];

	for (int i = 0; i < count; i++) {
		auto pSec = parser.GetSectionHeader(i);
		if (pSec == nullptr) {
			continue;
		}
		if (pSec->Characteristics & IMAGE_SCN_MEM_READ &&
			pSec->Characteristics & IMAGE_SCN_CNT_CODE &&
			pSec->Characteristics & IMAGE_SCN_MEM_EXECUTE &&
			!(pSec->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) &&
			(*(PULONG)pSec->Name != 'TINI')) {
			ULONG_PTR startAddr = (ULONG_PTR)((PUCHAR)imageBase + pSec->VirtualAddress);
			ULONG_PTR maxAddress = startAddr + pSec->Misc.VirtualSize;
			LogInfo("start address: %p max address: %p\n", startAddr, maxAddress);

			UCHAR pattern1[] = "\x48\xb8\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xff\xe0";
			ULONG patternSize = sizeof(pattern1) - 1;
			

			ULONG_PTR maxSearchAddr = maxAddress - patternSize;
			ULONG_PTR searchAddr = startAddr;
			ULONG remainSize = pSec->Misc.VirtualSize;
			while (searchAddr <= maxSearchAddr) {
				PVOID pFound = NULL;
				if (remainSize < patternSize) {
					break;
				}
				NTSTATUS status = Helpers::SearchPattern(pattern1, 0xCC, patternSize, 
					(void*)searchAddr, remainSize, &pFound);
				if (NT_SUCCESS(status)) {
					if (totalCount >= desiredCount) {
						break;
					}
					pData[totalCount].Address = (ULONG_PTR)pFound;
					pData[totalCount].Type = KernelHookType::x64HookType1;
					ULONG_PTR targetAddress = *(PULONG_PTR)((PUCHAR)pFound + 2);
					pData[totalCount].TargetAddress = targetAddress;
					LogInfo("Detect suspicious hook type 1 at %p\n", pFound);
					LogInfo("Target Address: 0x%-16llX\n", targetAddress);
					totalCount++;
					searchAddr = (ULONG_PTR)pFound + patternSize;
				}
				else {
					break;
				}
				remainSize = maxSearchAddr - searchAddr;
			}

			UCHAR pattern2[] = "\x68\xcc\xcc\xcc\xcc\xc7\x44\x24\x04\xcc\xcc\xcc\xcc\xc3";
			patternSize = sizeof(pattern2) - 1;
			maxSearchAddr = maxAddress - patternSize;
			searchAddr = startAddr;
			remainSize = pSec->Misc.VirtualSize;
			while (searchAddr <= maxSearchAddr) {
				PVOID pFound = NULL;
				if (remainSize < patternSize) {
					break;
				}
				NTSTATUS status = Helpers::SearchPattern(pattern2, 0xCC, patternSize, 
					(void*)searchAddr, remainSize, &pFound);
				if (NT_SUCCESS(status)) {
					if (totalCount >= desiredCount) {
						break;
					}
					pData[totalCount].Address = (ULONG_PTR)pFound;
					pData[totalCount].Type = KernelHookType::x64HookType2;
					ULONG lowAddr = *(PULONG)((PUCHAR)pFound + 1);
					ULONG highAddr = *(PULONG)((PUCHAR)pFound + 9);
					ULONG_PTR targetAddress = ((ULONG_PTR)highAddr << 32) | lowAddr;
					pData[totalCount].TargetAddress = targetAddress;
					LogInfo("Detect suspicious hook type 2 at %p\n", pFound);
					LogInfo("Target Address: 0x%-16llX\n", targetAddress);
					totalCount++;
					searchAddr = (ULONG_PTR)pFound + patternSize;
				}
				else {
					break;
				}
				remainSize = maxSearchAddr - searchAddr;
			}

			UCHAR pattern3[] = "\xe9\xcc\xcc\xcc\xcc";
			patternSize = sizeof(pattern3) - 1;
			maxSearchAddr = maxAddress - patternSize;
			searchAddr = startAddr;
			remainSize = pSec->Misc.VirtualSize;
			while (searchAddr <= maxSearchAddr) {
				PVOID pFound = NULL;
				if (remainSize < patternSize) {
					break;
				}
				NTSTATUS status = Helpers::SearchPattern(pattern3, 0xCC, patternSize, 
					(void*)searchAddr, remainSize, &pFound);
				if (NT_SUCCESS(status)) {
					do
					{
						status = ZydisDecoderDecodeFull(&decoder,
							pFound,
							patternSize, &instruction,
							operands);
						if (!ZYAN_SUCCESS(status))
						{
							break;
						}
						// Format and print the instruction
						const ZyanU64 instrAddress = (ZyanU64)pFound;
						status = ZydisFormatterFormatInstruction(
							&formatter, &instruction, operands, instruction.operand_count_visible, printBuffer,
							sizeof(printBuffer), instrAddress, NULL);
						if (!ZYAN_SUCCESS(status))
						{
							break;
						}

						readOffset += instruction.length;

						if (instruction.machine_mode != ZYDIS_MACHINE_MODE_LONG_64) {
							break;
						}
						if (instruction.mnemonic != ZYDIS_MNEMONIC_JMP) {
							break;
						}
						if (instruction.operand_count != 2) {
							break;
						}
						if (operands[0].type != ZYDIS_OPERAND_TYPE_IMMEDIATE) {
							break;
						}
						if (operands[1].type != ZYDIS_OPERAND_TYPE_REGISTER) {
							break;
						}

						if (operands[1].reg.value != ZYDIS_REGISTER_RIP) {
							break;
						}
						ULONG_PTR targetAddress = instrAddress + instruction.length
							+ operands[0].imm.value.u;
						if (targetAddress < maxAddress) {
							break;
						}
						PHYSICAL_ADDRESS physical = MmGetPhysicalAddress((PVOID)targetAddress);
						if (physical.QuadPart) {
							LogInfo("Physical Address: 0x%p\n", physical.QuadPart);
							LogInfo("Target Address: 0x%-16llX\n", targetAddress);

							pData[totalCount].Address = (ULONG_PTR)pFound;
							pData[totalCount].Type = KernelHookType::x64HookType3;
							pData[totalCount].TargetAddress = targetAddress;
							totalCount++;
							LogInfo("Detect suspicious hook type 3 at %p\n", pFound);
							LogInfo("0x%-16llX\t\t%hs\n", instrAddress, printBuffer);
						}
						
					} while (FALSE);

					searchAddr = (ULONG_PTR)pFound + patternSize;
				}
				else {
					break;
				}
				if (totalCount >= desiredCount) {
					break;
				}
				
				remainSize = maxSearchAddr - searchAddr;
			}
		}
	}
#endif

	KeUnstackDetachProcess(&apcState);
	ObDereferenceObject(Process);

}

ULONG khook::GetInlineHookCount(ULONG_PTR base) {
	ULONG totalCount = 0;
	PVOID imageBase = (PVOID)base;

	bool success = SearchSessionProcess();
	if (!success)
		return STATUS_UNSUCCESSFUL;

	PEPROCESS Process;
	NTSTATUS status = PsLookupProcessByProcessId(_pid, &Process);
	if (!NT_SUCCESS(status))
		return status;

	KAPC_STATE apcState;
	KeStackAttachProcess(Process, &apcState);

#ifndef _WIN64
	// x86 code

#else
	// x64 code
	PEParser parser(imageBase);
	int count = parser.GetSectionCount();
	

	// Initialize Zydis decoder and formatter
	ZydisDecoder decoder;
	if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64)))
		return 0;

	ZydisFormatter formatter;
	if (!ZYAN_SUCCESS(ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL)))
		return 0;

	SIZE_T readOffset = 0;
	ZydisDecodedInstruction instruction;
	ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
	CHAR printBuffer[128];

	for (int i = 0; i < count; i++) {
		auto pSec = parser.GetSectionHeader(i);
		if (pSec == nullptr) {
			continue;
		}
		if (pSec->Characteristics & IMAGE_SCN_MEM_READ &&
			pSec->Characteristics & IMAGE_SCN_CNT_CODE &&
			pSec->Characteristics & IMAGE_SCN_MEM_EXECUTE &&
			!(pSec->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) &&
			(*(PULONG)pSec->Name != 'TINI')) {
			ULONG_PTR startAddr = (ULONG_PTR)(base + pSec->VirtualAddress);
			ULONG_PTR maxAddress = startAddr + pSec->Misc.VirtualSize;

			UCHAR pattern1[] = "\x48\xb8\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xff\xe0";
			ULONG patternSize = sizeof(pattern1) - 1;
			LogInfo("start address: %p max address: %p\n", startAddr, maxAddress);

			ULONG_PTR maxSearchAddr = maxAddress - patternSize;
			ULONG_PTR searchAddr = startAddr;
			ULONG_PTR remainSize = pSec->Misc.VirtualSize;
			while (searchAddr <= maxSearchAddr) {
				PVOID pFound = NULL;
				NTSTATUS status = Helpers::SearchPattern(pattern1, 0xCC, patternSize, 
					(void*)searchAddr, remainSize, &pFound);
				if (NT_SUCCESS(status)) {
					LogInfo("Detect suspicious hook type 1 at %p\n", pFound);
					totalCount++;
					searchAddr = (ULONG_PTR)pFound + patternSize;
				}
				else {
					break;
				}
				remainSize = maxSearchAddr - searchAddr;
			}

			UCHAR pattern2[] = "\x68\xcc\xcc\xcc\xcc\xc7\x44\x24\x04\xcc\xcc\xcc\xcc\xc3";
			patternSize = sizeof(pattern2) - 1;
			maxSearchAddr = maxAddress - patternSize;
			searchAddr = startAddr;
			remainSize = pSec->Misc.VirtualSize;
			while (searchAddr <= maxSearchAddr) {
				PVOID pFound = NULL;
				NTSTATUS status = Helpers::SearchPattern(pattern2, 0xCC, patternSize, 
					(void*)searchAddr, remainSize, &pFound);
				if (NT_SUCCESS(status)) {
					LogInfo("Detect suspicious hook type 2 at %p\n", pFound);
					totalCount++;
					searchAddr = (ULONG_PTR)pFound + patternSize;
				}
				else {
					break;
				}
				remainSize = maxSearchAddr - searchAddr;
			}

			UCHAR pattern3[] = "\xe9\xcc\xcc\xcc\xcc";
			patternSize = sizeof(pattern3) - 1;
			maxSearchAddr = maxAddress - patternSize;
			searchAddr = startAddr;
			remainSize = pSec->Misc.VirtualSize;
			while (searchAddr <= maxSearchAddr) {
				PVOID pFound = NULL;
				NTSTATUS status = Helpers::SearchPattern(pattern3, 0xCC, patternSize, 
					(void*)searchAddr, remainSize, &pFound);
				if (NT_SUCCESS(status)) {
					do
					{
						status = ZydisDecoderDecodeFull(&decoder,
							pFound,
							patternSize, &instruction,
							operands);
						if (!ZYAN_SUCCESS(status))
						{
							break;
						}

						// Format and print the instruction
						const ZyanU64 instrAddress = (ZyanU64)pFound;
						status = ZydisFormatterFormatInstruction(
							&formatter, &instruction, operands, instruction.operand_count_visible, printBuffer,
							sizeof(printBuffer), instrAddress, NULL);
						if (!ZYAN_SUCCESS(status))
						{
							break;
						}

						readOffset += instruction.length;

						if (instruction.machine_mode != ZYDIS_MACHINE_MODE_LONG_64) {
							break;
						}
						if (instruction.mnemonic != ZYDIS_MNEMONIC_JMP) {
							break;
						}
						if (instruction.operand_count != 2) {
							break;
						}
						if (operands[0].type != ZYDIS_OPERAND_TYPE_IMMEDIATE) {
							break;
						}
						if (operands[1].type != ZYDIS_OPERAND_TYPE_REGISTER) {
							break;
						}

						if (operands[1].reg.value != ZYDIS_REGISTER_RIP) {
							break;
						}
						ULONG_PTR targetAddress = instrAddress + instruction.length
							+ operands[0].imm.value.u;
						if (targetAddress < maxAddress) {
							break;
						}
						PHYSICAL_ADDRESS physical = MmGetPhysicalAddress((PVOID)targetAddress);
						if (physical.QuadPart) {
							LogInfo("Target Address: 0x%-16llX\n", targetAddress);

							totalCount++;
							LogInfo("Detect suspicious hook type 3 at %p\n", pFound);
							LogInfo("0x%-16llX\t\t%hs\n", instrAddress, printBuffer);
						}
					} while (FALSE);
					searchAddr = (ULONG_PTR)pFound + patternSize;
				}
				else {
					break;
				}
				remainSize = maxSearchAddr - searchAddr;
			}


		}
	}
	
	KeUnstackDetachProcess(&apcState);
	ObDereferenceObject(Process);

#endif
	LogInfo("Total inline count: %d\n", totalCount);
	
	return totalCount;
}