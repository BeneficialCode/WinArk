#include "pch.h"
#include "WinExtHosts.h"

ULONG WinExtHosts::GetCount(PLIST_ENTRY pExpHostList) {
    PLIST_ENTRY listHead = pExpHostList;
    PLIST_ENTRY nextEntry = listHead->Flink;
    ULONG count = 0;
    while (nextEntry != listHead) {
        count++;
        nextEntry = nextEntry->Flink;
    }
    return count;
}

bool WinExtHosts::Enum(PLIST_ENTRY pExpHostList, WinExtHostInfo* pInfo) {
	if (!pExpHostList)
		return false;

    PLIST_ENTRY listHead = pExpHostList;
    PLIST_ENTRY nextEntry = listHead->Flink;
    PEX_HOST hostEntry = NULL;
    ULONG i = 0;
    while (nextEntry != listHead) {
        hostEntry = CONTAINING_RECORD(nextEntry, EX_HOST, HostListEntry);
        if (hostEntry != nullptr) {
            pInfo[i].BindNotification = (void*)hostEntry->HostParameters.BindNotification;
            pInfo[i].Id = hostEntry->HostParameters.HostBinding.ExtensionId;
            pInfo[i].BindNotificationContext = hostEntry->HostParameters.BindNotificationContext;
            pInfo[i].ExHost = hostEntry;
            pInfo[i].ExtensionTable = hostEntry->ExtensionTable;
            pInfo[i].HostTable = hostEntry->HostParameters.HostTable;
            pInfo[i].Version = hostEntry->HostParameters.HostBinding.ExtensionVersion;
            pInfo[i].FunctionCount = hostEntry->HostParameters.HostBinding.FunctionCount;
            pInfo[i].Flags = hostEntry->Flags;
            i++;
        }
        nextEntry = nextEntry->Flink;
    }

	return true;
}

bool WinExtHosts::EnumExtTable(ExtHostData* pData, PVOID* pInfo) {
    using PExpFindHost = PEX_HOST(NTAPI*)(USHORT Id, USHORT Version);
    PExpFindHost pExpFindHost = (PExpFindHost)pData->ExpFindHost;
    USHORT id = pData->Id;
    USHORT version = pData->Version;
    ULONG count = pData->Count;

    PEX_HOST host = pExpFindHost(id, version);
    if (host == nullptr) {
        return false;
    }
    PVOID* pExtensionTable = (PVOID*)host->ExtensionTable;
    if (pExtensionTable == nullptr) {
        return false;
    }
    for (int i = 0; i < count; i++) {
        pInfo[i] = pExtensionTable[i];
    }

    return true;
}