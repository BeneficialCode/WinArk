#include "pch.h"
#include "Section.h"
#include "Logging.h"
#include "Memory.h"
#include "ObjectAttributes.h"
#include "Common.h"


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
	do
	{
		status = RtlRunOnceBeginInitialize(&_sectionSingletonState, 0, &Context);
		if (status == STATUS_PENDING) {
			// We get here only during the first initialization
			Context = nullptr;

			// Allocate memory
			DllStats* pDllState = new (PagedPool,'mSDk') DllStats;
			if (pDllState) {
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

			}
			else {
				status = STATUS_MEMORY_NOT_ALLOCATED;
				LogError("ERROR: (0x%x) , section type=%d", status, _type);

				break;
			}
			

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
				break;
			}
		}
		else if (status != STATUS_SUCCESS) {
			// Error
			LogError("ERROR: (0x%x) RtlRunOnceBeginInitialize, section type=%d", status, _type);
		}
	} while (false);
	

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
			DllStats* pDllState = (DllStats*)Context;

			// Free memory
			delete pDllState;
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
		LogError("0x%x RtlRunOnceBeginInitialize", status);
		ASSERT(nullptr);
	}

	return status;
}

NTSTATUS Section::CreateKnownDllSection(DllStats& dllState) {
	// Create a known dll system section of our own
	NTSTATUS status;

	// Clear the return data (assuming only primitive data types)
	memset(&dllState, 0, sizeof(dllState));

	// need to steal a security descriptor from a existing KnownDll - we'll use kernel32
	HANDLE hSectionK32;

#ifdef _WIN64
	UNICODE_STRING name = RTL_CONSTANT_STRING(L"\\KnownDlls\\kernel32.dll");
	ObjectAttributes attr(&name, ObjectAttributesFlags::Caseinsensive);
	UNICODE_STRING objName = RTL_CONSTANT_STRING(INJECTED_DLL_FILE_NAME);
#else
	UNICODE_STRING name = RTL_CONSTANT_STRING(L"\\KnownDlls32\\kernel32.dll");
	ObjectAttributes attr(&name, ObjectAttributesFlags::Caseinsensive);
	UNICODE_STRING objName = RTL_CONSTANT_STRING(INJECTED_DLL_FILE_NAME);
#endif // _WIN64

	status = ZwOpenSection(&hSectionK32, READ_CONTROL, &attr);
	if (NT_SUCCESS(status)) {
		status = STATUS_GENERIC_COMMAND_FAILED;
		// INFO: Make our section "permanent", which means that it won't be deleted if all of its handle are closed
		// and we will need to call ZwMakeTemporaryObject() on it first to allow it.

		ObjectAttributes objAttr(&objName, ObjectAttributesFlags::Caseinsensive | ObjectAttributesFlags::Permanent);

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

		}

		// Free our memory
		if (objAttr.SecurityDescriptor) {
			ExFreePool(objAttr.SecurityDescriptor);
			objAttr.SecurityDescriptor = nullptr;
		}
	}
	else {
		// Error
		LogError("Error: 0x%x ZwOpenSection PID=%u sectionType=%d\n", status,
			PsGetCurrentProcessId(),_type);
	}
	return status;
}