#include "stdafx.h"
#include "MutexObjectType.h"
#include "NtDll.h"

MutexObjectType::MutexObjectType(int index, PCWSTR name) : ObjectType(index, name) {
}

CString MutexObjectType::GetDetails(HANDLE hMutex) {
	MUTANT_BASIC_INFORMATION info;
	MUTANT_OWNER_INFORMATION owner;
	CString details;
	if (NT_SUCCESS(::NtQueryMutant(hMutex, MutantBasicInformation, &info, sizeof(info), nullptr))
		&& NT_SUCCESS(::NtQueryMutant(hMutex, MutantOwnerInformation, &owner, sizeof(owner), nullptr))) {
		details.Format(L"Owner TID: %d, Count: %d, Abandoned: %s",
			HandleToULong(owner.ClientId.UniqueThread),
			info.CurrentCount, info.AbandonedState ? L"True" : L"False");
	}
	return details;
}
