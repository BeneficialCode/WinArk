#include "pch.h"
#include "Section.h"
#include "Logging.h"
#include "Memory.h"
#include "ObjectAttributes.h"
#include <ntimage.h>
#include "Helpers.h"
#include "NtMmApi.h"



extern PDRIVER_OBJECT g_DriverObject;


NTSTATUS Section::Initialize(SectionType type){
	// Initialize this object
	_type = type;

	// Initialize our singleton
	RtlRunOnceInitialize(&_sectionSingletonState);

	return STATUS_SUCCESS;
}

// Get DLL section object
NTSTATUS Section::GetSection(DllStats** ppSectionInfo) {
	NTSTATUS status = STATUS_SUCCESS;

	// make sure that Initialize was called!
	ASSERT(_type == SectionType::Native || _type == SectionType::Wow);
	// Use the single approach 
	PVOID Context = nullptr;
	
	status = RtlRunOnceBeginInitialize(&_sectionSingletonState, 0, &Context);
	if (!NT_SUCCESS(status)) {
		LogError("RtlRunOnceBeginInitialize failed 0x%x", status);
		return status;
	}
	if (status == STATUS_PENDING) {
		// We get here only during the first initialization
		Context = nullptr;
		// Allocate memory
		DllStats* pDllState = new (PagedPool, 'mSDk') DllStats;
		do
		{
			if (!pDllState) {
				break;
			}

			// Need to "trick" the system into creating a KnownDll section for us with the SD of
			// the kernel32.dll section

			// Temporarily attach the current thread to the address space of the system process
			KAPC_STATE state;
			KeStackAttachProcess(PsInitialSystemProcess, &state);

			// Create our KnownDll section to bypass code integrity guard
			status = CreateKnownDllSection(*pDllState);

			// Revert back
			KeUnstackDetachProcess(&state);

			// Check the result
			if (NT_SUCCESS(status)) {
				Context = pDllState;
			}
			else {
				LogError("ERROR: (0x%x), sectionType=%d", status, _type);
				// Free memory
				delete pDllState;
				pDllState = nullptr;
			}
		} while (false);

		// Finilize our singleton
		status = RtlRunOnceComplete(&_sectionSingletonState, 0, Context);
		if (!NT_SUCCESS(status)) {
			// Error
			LogError("ERROR: (0x%x) RtlRunOnceComplete, section type=%d", status, _type);
			ASSERT(nullptr);

			// Free memory
			if (pDllState) {
				delete pDllState;
				pDllState = nullptr;
			}
			Context = nullptr;
		}
	}

	if (ppSectionInfo)
		*ppSectionInfo = (DllStats*)Context;

	return status;
}

NTSTATUS Section::FreeSection() {
	// Release resources held for the mapped section
	NTSTATUS status;

	PVOID Context = nullptr;
	status = RtlRunOnceBeginInitialize(&_sectionSingletonState, 0, &Context);
	if (NT_SUCCESS(status)) {
		// We have initilized our singleton

		// do we have the context - otherwise there's nothing to delete
		if (Context) {
			LogInfo("FreeSection ,sectionType=%d", _type);
			DllStats* pDllStats = (DllStats*)Context;

			ObMakeTemporaryObject(pDllStats->Section);

			ObDereferenceObjectWithTag(pDllStats->Section, 'k23w');
			pDllStats->Section = nullptr;

			// Free memory
			delete pDllStats;
			Context = nullptr;
		}

		// Reset the singleton back
		RtlRunOnceInitialize(&_sectionSingletonState);
	}
	else if (status == STATUS_UNSUCCESSFUL) {
		// GetSection() wasn't called yet
		status = STATUS_SUCCESS;
	}
	else {
		// Error
		LogError("Error: 0x%x RtlRunOnceBeginInitialize", status);
		ASSERT(nullptr);
	}

	return status;
}

NTSTATUS Section::CreateKnownDllSection(DllStats& dllStats) {
	// Create a known dll system section of our own
	NTSTATUS status;

	// Clear the return data (assuming only primitive data types)
	memset(&dllStats, 0, sizeof(dllStats));

	// need to steal a security descriptor from a existing KnownDll - we'll use kernel32
	HANDLE hSectionK32;

	POBJECT_ATTRIBUTES pKernel32Attr{};
	PUNICODE_STRING pObjName{};
	POBJECT_ATTRIBUTES pDllAttr{};
#ifdef _WIN64
	if (_type == SectionType::Wow) {
		UNICODE_STRING name = RTL_CONSTANT_STRING(L"\\KnownDlls32\\kernel32.dll");
		static ObjectAttributes attr(&name, ObjectAttributesFlags::Caseinsensive);
		static UNICODE_STRING objName = RTL_CONSTANT_STRING(L"\\KnownDlls32\\" INJECTED_DLL_FILE_NAME32);
		UNICODE_STRING path = RTL_CONSTANT_STRING(INJECTED_DLL_NT_PATH_WOW);
		static ObjectAttributes dllAttr(&path, ObjectAttributesFlags::Caseinsensive);
		pKernel32Attr = &attr;
		pObjName = &objName;
		pDllAttr = &dllAttr;
	}
	else
#endif
	{
		UNICODE_STRING name = RTL_CONSTANT_STRING(L"\\KnownDlls\\kernel32.dll");
		static ObjectAttributes attr(&name, ObjectAttributesFlags::Caseinsensive);
		static UNICODE_STRING objName = RTL_CONSTANT_STRING(L"\\KnownDlls\\" INJECTED_DLL_FILE_NAME32);
		UNICODE_STRING path = RTL_CONSTANT_STRING(INJECTED_DLL_NT_PATH_WOW);
		static ObjectAttributes dllAttr(&path, ObjectAttributesFlags::Caseinsensive);
		pKernel32Attr = &attr;
		pObjName = &objName;
		pDllAttr = &dllAttr;
	}

	status = ZwOpenSection(&hSectionK32, READ_CONTROL, pKernel32Attr);

	if (!NT_SUCCESS(status)) {
		LogError("ERROR: (0x%X) ZwOpenSection(hSectionK32), PID=%u, sectionType=%c\n",
			status,
			PsGetCurrentProcessId(),
			_type);
		return status;
	}

	status = STATUS_GENERIC_COMMAND_FAILED;
	// INFO: Make our section "permanent", which means that it won't be deleted if all of its handle are closed
	// and we will need to call ZwMakeTemporaryObject() on it first to allow it.

	ObjectAttributes objAttr(pObjName, ObjectAttributesFlags::Caseinsensive | ObjectAttributesFlags::Permanent);

	LogInfo("Got KnownDll Ok!\n");
	ULONG size = 0;
	for (;;) {
		ULONG needSize = 0;
		status = ZwQuerySecurityObject(hSectionK32, PROCESS_TRUST_LABEL_SECURITY_INFORMATION |
			DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,
			objAttr.SecurityDescriptor,	// SD
			size,	// mem size
			&needSize);

		if (NT_SUCCESS(status)) {
			// Got it
			break;
		}
		else if (STATUS_BUFFER_TOO_SMALL == status) {
			// We need more memory
			ASSERT(needSize > size);
			if (objAttr.SecurityDescriptor) {
				// Free previous memory
				ExFreePool(objAttr.SecurityDescriptor);
				objAttr.SecurityDescriptor = nullptr;
			}

			// Allocate mem
			objAttr.SecurityDescriptor = ExAllocatePoolWithTag(PagedPool, needSize, 'm23k');
			if (objAttr.SecurityDescriptor) {
				// Need to retry
				size = needSize;
			}
			else {
				// Error
				status = STATUS_MEMORY_NOT_ALLOCATED;
				LogError("0x%x\n", status);

				break;
			}
		}
		else {
			// Error
			LogError("0x%x\n", status);
			break;
		}
	}


	// Close section
	status = ZwClose(hSectionK32);

	if (NT_SUCCESS(status)) {
		// Now we can create our own seciton for our injected DLL in the KnownDlls kernel object directory
		HANDLE hFile;
		IO_STATUS_BLOCK iosb;

		do
		{
			status = ZwOpenFile(&hFile, FILE_GENERIC_READ | FILE_EXECUTE,
				pDllAttr, &iosb, FILE_SHARE_READ,
				FILE_SYNCHRONOUS_IO_NONALERT);
			if (!NT_SUCCESS(status)) {
				LogError("ERROR: (0x%X) ZwOpenFile, PID=%u, sectionType=%c, path=\"%wZ\"\n",
					status, PsGetCurrentProcessId(),
					_type, pDllAttr->ObjectName);
				break;
			}

			// Open PE file as section
			HANDLE hSection;
			status = ZwCreateSection(&hSection, SECTION_MAP_EXECUTE | SECTION_QUERY,
				&objAttr, 0, PAGE_EXECUTE, SEC_IMAGE, hFile);
			if (!NT_SUCCESS(status)) {
				LogError("ERROR: (0x%X) ZwCreateSection, PID=%u, sectionType=%c\n",
					status, PsGetCurrentProcessId(), _type);
				break;
			}

			PVOID base = nullptr;
			SIZE_T viewSize = 0;
			status = ZwMapViewOfSection(hSection, NtCurrentProcess(), &base,
				0, 0, 0, &viewSize, ViewUnmap, 0, PAGE_READONLY);
			if (!NT_SUCCESS(status)) {
				LogError("ERROR: (0x%X) ZwMapViewOfSection, PID=%u, sectionType=%c\n",
					status, PsGetCurrentProcessId(),
					_type);
				break;
			}

			ULONG ordIndex = 1;
			ULONG rva = 0;
			__try {
				status = STATUS_INVALID_IMAGE_FORMAT;
				PIMAGE_NT_HEADERS pNtHdr = RtlImageNtHeader(base);
				if (!pNtHdr) {
					LogError("ERROR: (0x%X) RtlImageNtHeader, PID=%u, sectionType=%c\n",
						status, PsGetCurrentProcessId(),
						_type);
					status = STATUS_INVALID_IMAGE_FORMAT;
					break;
				}

				ULONG size;
				PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY)
					RtlImageDirectoryEntryToData(base, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &size);
				if (pExportDir && size > sizeof(IMAGE_EXPORT_DIRECTORY)) {
					ordIndex -= pExportDir->Base;
					if (ordIndex < pExportDir->NumberOfFunctions) {
						PULONG pFuncAddr = (PULONG)((BYTE*)base + pExportDir->AddressOfFunctions);
						if (!pFuncAddr) {
							LogError("ERROR: (0x%X) Bad AddressOfFunctions, PID=%u, sectionType=%c\n",
								status,
								(ULONG)(ULONG_PTR)PsGetCurrentProcessId(),
								_type);
							break;
						}
						rva = pFuncAddr[ordIndex];
						if (rva > 0) {
							UINT rvaGuid = Helpers::FindStringByGuid(base,
								pNtHdr->OptionalHeader.SizeOfImage,
								&dllGuid);
							if (rvaGuid != -1) {
								SECTION_IMAGE_INFORMATION info;
								status = ZwQuerySection(hSection,
									SectionImageInformation, &info, sizeof(info), nullptr);
								if (!NT_SUCCESS(status)) {
									LogError("ERROR: (0x%X) ZwQuerySection, PID=%u, sectionType=%c\n",
										status, PsGetCurrentProcessId(), _type);
									break;
								}

								status = ObReferenceObjectByHandleWithTag(hSection,
									0, nullptr, KernelMode, 'SkFh',
									&dllStats.Section, nullptr);
								if (NT_SUCCESS(status)) {
									dllStats.Type = _type;
									dllStats.SizeOfImage = pNtHdr->OptionalHeader.SizeOfImage;
									dllStats.ShellCodeRVA = rva;
									dllStats.DllNameRVA = rvaGuid;

									dllStats.PreferredAddress = (BYTE*)info.TransferAddress -
										pNtHdr->OptionalHeader.AddressOfEntryPoint;
								}
								else {
									LogError("ERROR: (0x%X) ObReferenceObjectByHandle, PID=%u, sectionType=%c\n",
										status,
										PsGetCurrentProcessId(),
										_type);
									break;
								}
							}
						}
						else {
							LogError("ERROR: (0x%X) Bad RVA=%d, PID=%u, sectionType=%c\n",
								status, rva, PsGetCurrentProcessId(),
								_type);
							break;
						}
					}
					else {
						LogError("ERROR: (0x%X) Bad ordinal, PID=%u cnt=%u, sectionType=%c\n",
							status,
							PsGetCurrentProcessId(),
							pExportDir->NumberOfFunctions,
							_type);
						break;
					}
				}
				else {
					LogError("ERROR: (0x%X) Export directory, PID=%u size=%u, sectionType=%c\n",
						status, PsGetCurrentProcessId(),
						size, _type);
					break;
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				LogError("#EXCEPTION: (0x%X) CreateKnownDllSection(PE-scan), PID=%u, sectionType=%c\n",
					GetExceptionCode(),
					PsGetCurrentProcessId(),
					_type);
				status = STATUS_INVALID_IMAGE_FORMAT;
			}

			status = ZwUnmapViewOfSection(NtCurrentProcess(), base);
		} while (false);
		

	}

	// Free our memory
	if (objAttr.SecurityDescriptor) {
		ExFreePool(objAttr.SecurityDescriptor);
		objAttr.SecurityDescriptor = nullptr;
	}

	return status;
}

NTSTATUS Section::InjectDLL(DllStats* pDllStats) {
	NTSTATUS status = STATUS_SUCCESS;

	if (!pDllStats->IsVaid()) {
		// invalid data
		ASSERT(NULL);
		return STATUS_INVALID_PARAMETER_MIX;
	}
	// https://dennisbabkin.com/inside_nt_apc/
	PKAPC pApc = (PKAPC)ExAllocatePoolWithTag(NonPagedPool, sizeof(KAPC), 'cpak');

	if (!pApc) {
		return STATUS_MEMORY_NOT_ALLOCATED;
	}

	/*KeInitializeApc(pApc, KeGetCurrentThread(),
		OriginalApcEnvironment,
		KernelRoutine, RundownRoutine, NormalRoutine,
		KernelMode,
		pApc, );*/

	//Prevent our driver from unloading be incrementing its reference count
	ObReferenceObject(g_DriverObject);

	return status;
}

NTSTATUS Section::MapSectionForShellCode(DllStats* pDllStats,
	PVOID* pBaseAddr) {
	ASSERT(pDllStats);
	NTSTATUS status = STATUS_SUCCESS;

	PVOID baseAddress = nullptr;

	if (!pDllStats->IsVaid()) {
		status = STATUS_INVALID_PARAMETER_MIX;
		return status;
	}

	PVOID reservedAddr = pDllStats->PreferredAddress;
	SIZE_T regionSize = pDllStats->SizeOfImage;

	status = ZwAllocateVirtualMemory(NtCurrentProcess(),
		&reservedAddr, 0, &regionSize,
		MEM_RESERVE, PAGE_NOACCESS);

	if (status != STATUS_SUCCESS &&
		status != STATUS_CONFLICTING_ADDRESSES) {
		LogError("Error: (0x%X) MapSectionForShellCode > ZwAllocateVirtualMemory, PID=%u, sectionType=%c, "
			"addr=0x%p, sz=0x%X\n",
			status, PsGetCurrentProcessId(),
			pDllStats->Type, pDllStats->PreferredAddress,
			pDllStats->SizeOfImage);
		return status;
	}

	SIZE_T viewSize = 0;
	LARGE_INTEGER offset{};

	status = MmMapViewOfSection(pDllStats->Section,
		IoGetCurrentProcess(), &baseAddress, 0, 0,
		&offset, &viewSize, ViewUnmap, 0, PAGE_EXECUTE);

	if (!NT_SUCCESS(status)) {
		LogError("ERROR: (0x%X) MapSectionForShellCode > MmMapViewOfSection, "
			"PID = % u, sectionType = % c",
			status,
			PsGetCurrentProcessId(),
			pDllStats->Type
		);
		return status;
	}

	regionSize = 0;
	status = ZwFreeVirtualMemory(NtCurrentProcess(), &reservedAddr, &regionSize, MEM_RELEASE);

	if (pBaseAddr)
		*pBaseAddr = baseAddress;

	return status;
}