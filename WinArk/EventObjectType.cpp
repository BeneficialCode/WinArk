#include "stdafx.h"
#include "EventObjectType.h"
#include "NtDll.h"

EventObjectType::EventObjectType(int index, PCWSTR name) : ObjectType(index, name) {
}

CString EventObjectType::GetDetails(HANDLE hEvent) {
	EVENT_BASIC_INFORMATION info;
	CString details;
	if (NT_SUCCESS(::NtQueryEvent(hEvent, EventBasicInformation, &info, sizeof(info), nullptr))) {
		details.Format(L"Type: %s, Signaled: %s", info.EventType == SynchronizationEvent ? L"Synchronization" : L"Notification",
			info.EventState ? L"True" : L"False");
	}

	return details;
}
