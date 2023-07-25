#pragma once

typedef struct _EX_HOST_BINDING {
	USHORT ExtensionId;
	USHORT ExtensionVersion;
	USHORT FunctionCount;
} EX_HOST_BINDING;

typedef DWORD_PTR PEX_HOST_BIND_NOTIFICATION;

typedef struct _EX_HOST_PARAMETERS {
	EX_HOST_BINDING             HostBinding;
	POOL_TYPE                   PoolType;
	PVOID                       HostTable;
	PEX_HOST_BIND_NOTIFICATION  BindNotification;
	PVOID                       BindNotificationContext;
} EX_HOST_PARAMETERS, * PEX_HOST_PARAMETERS;

typedef struct _EX_HOST {
	LIST_ENTRY          HostListEntry;
	volatile LONG       RefCounter;
	EX_HOST_PARAMETERS  HostParameters;
	EX_RUNDOWN_REF      RundownProtection;
	EX_PUSH_LOCK        PushLock;
	PVOID               ExtensionTable;
	ULONG               Flags;
} EX_HOST, * PEX_HOST;

struct WinExtHosts final {
	static ULONG GetCount(PLIST_ENTRY pExpHostList);
	static bool Enum(PLIST_ENTRY pExpHostList, WinExtHostInfo* pInfo);
	static bool EnumExtTable(ExtHostData* pData, PVOID* pInfo);
};