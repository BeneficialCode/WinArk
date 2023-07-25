#include "stdafx.h"
#include <stdint.h>
#include "FormatHelper.h"
#include "TraceManager.h"

typedef enum _OBJECT_INFORMATION_CLASS {
	ObjectBasicInformation, // OBJECT_BASIC_INFORMATION
	ObjectNameInformation, // OBJECT_NAME_INFORMATION
	ObjectTypeInformation, // OBJECT_TYPE_INFORMATION
	ObjectTypesInformation, // OBJECT_TYPES_INFORMATION
	ObjectHandleFlagInformation, // OBJECT_HANDLE_FLAG_INFORMATION
	ObjectSessionInformation,
	ObjectSessionObjectInformation,
	MaxObjectInfoClass
} OBJECT_INFORMATION_CLASS;

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING* PUNICODE_STRING;
typedef const UNICODE_STRING* PCUNICODE_STRING;

extern "C" NTSTATUS NTAPI NtQueryObject(
	_In_opt_ HANDLE Handle,
	_In_ OBJECT_INFORMATION_CLASS ObjectInformationClass,
	_Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
	_In_ ULONG ObjectInformationLength,
	_Out_opt_ PULONG ReturnLength);

typedef struct _OBJECT_TYPE_INFORMATION {
	UNICODE_STRING TypeName;
	ULONG TotalNumberOfObjects;
	ULONG TotalNumberOfHandles;
	ULONG TotalPagedPoolUsage;
	ULONG TotalNonPagedPoolUsage;
	ULONG TotalNamePoolUsage;
	ULONG TotalHandleTableUsage;
	ULONG HighWaterNumberOfObjects;
	ULONG HighWaterNumberOfHandles;
	ULONG HighWaterPagedPoolUsage;
	ULONG HighWaterNonPagedPoolUsage;
	ULONG HighWaterNamePoolUsage;
	ULONG HighWaterHandleTableUsage;
	ULONG InvalidAttributes;
	GENERIC_MAPPING GenericMapping;
	ULONG ValidAccessMask;
	BOOLEAN SecurityRequired;
	BOOLEAN MaintainHandleCount;
	UCHAR TypeIndex; // since WINBLUE
	CHAR ReservedByte;
	ULONG PoolType;
	ULONG DefaultPagedPoolCharge;
	ULONG DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION, * POBJECT_TYPE_INFORMATION;

typedef struct _OBJECT_TYPES_INFORMATION {
	ULONG NumberOfTypes;
	OBJECT_TYPE_INFORMATION TypeInformation[1];
} OBJECT_TYPES_INFORMATION, * POBJECT_TYPES_INFORMATION;

typedef struct _SYSTEM_TIMEOFDAY_INFORMATION {
	LARGE_INTEGER BootTime;
	LARGE_INTEGER CurrentTime;
	LARGE_INTEGER TimeZoneBias;
	ULONG TimeZoneId;
	ULONG Reserved;
	ULONGLONG BootTimeBias;
	ULONGLONG SleepTimeBias;
} SYSTEM_TIMEOFDAY_INFORMATION, * PSYSTEM_TIMEOFDAY_INFORMATION;

enum SYSTEM_INFORMATION_CLASS {
	SystemTimeOfDayInformation = 3
};

extern "C" NTSTATUS NTAPI NtQuerySystemInformation(
	_In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
	_Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
	_In_ ULONG SystemInformationLength,
	_Out_opt_ PULONG ReturnLength);

using namespace WinSys;

CString FormatHelper::ToBinary(ULONGLONG value) {
	CString svalue;

	while (value) {
		svalue = ((value & 1) ? L"1" : L"0") + svalue;
		value >>= 1;
	}
	if (svalue.IsEmpty())
		svalue = L"0";
	return svalue;
}

CString FormatHelper::TimeSpanToString(int64_t ts) {
	auto str = CTimeSpan(ts / 10000000).Format(L"%D.%H:%M:%S");

	str.Format(L"%s %03d", str, (ts / 10000) % 1000);
	return str;
}

CString FormatHelper::FormatWithCommas(long long size) {
	CString result;
	result.Format(L"%lld", size);
	int i = 3;
	while (result.GetLength() - i > 0) {
		result = result.Left(result.GetLength() - i) + L"," + result.Right(i);
		i += 4;
	}
	return result;
}

CString FormatHelper::TimeToString(int64_t time, bool includeMS) {
	auto str = CTime(*(FILETIME*)&time).Format(L"%x %X");
	if (includeMS) {
		str.Format(L"%s.%03d", str, (time / 10000) % 1000);
	}
	return str;
}

PCWSTR FormatHelper::PriorityClassToString(WinSys::ProcessPriorityClass pc) {
	switch (pc) {
	case WinSys::ProcessPriorityClass::Normal: return L"Normal (8)";
	case WinSys::ProcessPriorityClass::AboveNormal: return L"Above Normal (10)";
	case WinSys::ProcessPriorityClass::BelowNormal: return L"Below Normal (6)";
	case WinSys::ProcessPriorityClass::High: return L"High (13)";
	case WinSys::ProcessPriorityClass::Idle: return L"Idle (4)";
	case WinSys::ProcessPriorityClass::RealTime: return L"Realtime (24)";
	}
	return L"";
}

CString FormatHelper::ProcessAttributesToString(ProcessAttributes attributes) {
	CString text;

	static const struct {
		ProcessAttributes Attribute;
		PCWSTR Text;
	} attribs[] = {
		{ ProcessAttributes::Managed, L"Managed" },
		{ ProcessAttributes::Immersive, L"Immersive" },
		{ ProcessAttributes::Protected, L"Protected" },
		{ ProcessAttributes::Secure, L"Secure" },
		{ ProcessAttributes::Service, L"Service" },
		{ ProcessAttributes::InJob, L"Job" },
		{ ProcessAttributes::Wow64, L"Wow64" },
	};

	for (auto& item : attribs)
		if ((item.Attribute & attributes) == item.Attribute)
			text += CString(item.Text) + ", ";
	if (!text.IsEmpty())
		text = text.Mid(0, text.GetLength() - 2);
	return text;
}

CString FormatHelper::SidAttributesToString(WinSys::SidGroupAttributes sidAttributes) {
	CString result;

	static struct {
		DWORD flag;
		PCWSTR text;
	}attributes[] = {
		{SE_GROUP_ENABLED,L"Enabled"},
		{SE_GROUP_ENABLED_BY_DEFAULT,L"Default Enabled"},
		{SE_GROUP_INTEGRITY,L"Integrity"},
		{SE_GROUP_INTEGRITY_ENABLED,L"Integrity Enabled"},
		{SE_GROUP_LOGON_ID,L"Logon ID"},
		{SE_GROUP_MANDATORY,L"Mandatory"},
		{SE_GROUP_OWNER,L"Owner"},
		{SE_GROUP_RESOURCE,L"Domain Local"},
		{SE_GROUP_USE_FOR_DENY_ONLY,L"Deny Only"},
	};

	for (const auto& attr : attributes)
		if ((attr.flag & (DWORD)sidAttributes) == attr.flag)
			(result += attr.text) += ", ";

	if (!result.IsEmpty())
		result = result.Left(result.GetLength() - 2);
	else
		result = L"(none)";

	return result;
}

PCWSTR FormatHelper::VirtualizationStateToString(VirtualizationState state) {
	switch (state) {
	case VirtualizationState::Disabled: return L"Disable";
	case VirtualizationState::Enabled: return L"Enabled";
	case VirtualizationState::NotAllowed:return L"Not Allowed";
	}
	return L"Unknown";
}

PCWSTR FormatHelper::IntegrityToString(IntegrityLevel level) {
	switch (level) {
	case IntegrityLevel::High: return L"High";
	case IntegrityLevel::Medium: return L"Media";
	case IntegrityLevel::MediumPlus: return L"Media+";
	case IntegrityLevel::Low: return L"Low";
	case IntegrityLevel::System: return L"System";
	case IntegrityLevel::Untrusted: return L"Untrusted";
	}

	return L"Unknown";
}

CString FormatHelper::PrivilegeAttributesToString(DWORD pattributes) {
	CString result;
	static struct {
		DWORD flag;
		PCWSTR text;
	} attributes[] = {
		{ SE_PRIVILEGE_ENABLED, L"Enabled" },
		{ SE_PRIVILEGE_ENABLED_BY_DEFAULT, L"Default Enabled" },
		{ SE_PRIVILEGE_REMOVED, L"Removed" },
		{ SE_PRIVILEGE_USED_FOR_ACCESS, L"Used for Access" },
	};

	for (const auto& attr : attributes)
		if ((attr.flag & pattributes) == attr.flag)
			(result += attr.text) += ", ";

	if (!result.IsEmpty())
		result = result.Left(result.GetLength() - 2);
	else
		result = "Disabled";
	return result;
}

PCWSTR FormatHelper::SidNameUseToString(SID_NAME_USE use) {
	switch (use) {
	case SidTypeUser: return L"User";
	case SidTypeGroup: return L"Group";
	case SidTypeDomain: return L"Domain";
	case SidTypeAlias: return L"Alias";
	case SidTypeWellKnownGroup: return L"Well Known Group";
	case SidTypeDeletedAccount: return L"Deleted Account";
	case SidTypeInvalid: return L"Invalid";
	case SidTypeComputer: return L"Computer";
	case SidTypeLabel: return L"Label";
	case SidTypeLogonSession: return L"Logon Session";
	}
	return L"Unknown";
}

CString FormatHelper::JobCpuRateControlFlagsToString(DWORD flags) {
	CString result;
	struct {
		DWORD flag;
		PCWSTR text;
	} data[] = {
		//{ JOB_OBJECT_CPU_RATE_CONTROL_ENABLE, L"Enabled" },
		{ JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP, L"Hard Cap" },
		{ JOB_OBJECT_CPU_RATE_CONTROL_MIN_MAX_RATE, L"Min Max Rate" },
		{ JOB_OBJECT_CPU_RATE_CONTROL_NOTIFY, L"Notify" },
		{ JOB_OBJECT_CPU_RATE_CONTROL_WEIGHT_BASED, L"Weight Based" },
	};
	for (const auto& attr : data)
		if ((attr.flag & flags) == attr.flag)
			(result += attr.text) += ", ";

	if (!result.IsEmpty())
		result = result.Left(result.GetLength() - 2);
	return result;
}

PCWSTR FormatHelper::IoPriorityToString(WinSys::IoPriority io) {
	switch (io) {
	case IoPriority::Critical: return L"Critical";
	case IoPriority::High: return L"High";
	case IoPriority::Low: return L"Low";
	case IoPriority::Normal: return L"Normal";
	case IoPriority::VeryLow: return L"Very Low";
	}
	return L"";
}

CString FormatHelper::ComFlagsToString(ComFlags flags) {
	static const struct {
		ComFlags Flag;
		PCWSTR Name;
	} sflags[] = {
		{ ComFlags::LocalTid, L"Local TID" },
		{ ComFlags::UuidInitialized, L"GUID Initialized" },
		{ ComFlags::InThreadDetach, L"In Thread Detach" },
		{ ComFlags::ChannelInitialized, L"Channel Initialized" },
		{ ComFlags::WowThread, L"Wow Thread" },
		{ ComFlags::ThreadUninitializing, L"Thread Uninitializing" },
		{ ComFlags::DisableOle1DDE, L"Disable Ole1 DDE" },
		{ ComFlags::STA, L"STA" },
		{ ComFlags::MTA, L"MTA" },
		{ ComFlags::Impersonating, L"Impersonating" },
		{ ComFlags::DisableEventLogger, L"Disable Event Logger" },
		{ ComFlags::InNeutralApartment, L"In NA" },
		{ ComFlags::DispatchThread, L"Disptach Thread" },
		{ ComFlags::HostThread, L"Host Thread" },
		{ ComFlags::AllowCoInit, L"Allow CoInit" },
		{ ComFlags::PendingUninit, L"Pending Uninit" },
		{ ComFlags::FirstMTAInit, L"First MTA Init" },
		{ ComFlags::FirstNTAInit, L"First TNA Init" },
		{ ComFlags::ApartmentInitializing, L"Apt Initializing" },
		{ ComFlags::UIMessageInModalLoop, L"UI Msg in Modal Loop" },
		{ ComFlags::MarshallingErrorObject, L"Marshaling Error Object" },
		{ ComFlags::WinRTInitialize, L"WinRT Init" },
		{ ComFlags::ASTA, L"ASTA" },
		{ ComFlags::InShutdownCallbacks, L"In Shutdown Callbacks" },
		{ ComFlags::PointerInputBlocked, L"Pointer Input Blocked" },
		{ ComFlags::InActivationFilter, L"In Activation Filter" },
		{ ComFlags::ASTAtoASTAExempQuirk, L"ASTA to STA Exep Quirk" },
		{ ComFlags::ASTAtoASTAExempProxy, L"ASTA to STA Exep Proxy" },
		{ ComFlags::ASTAtoASTAExempIndoubt, L"ASTA to STA Exep In Doubt" },
		{ ComFlags::DetectedUserInit, L"Detect User Init" },
		{ ComFlags::BridgeSTA, L"Bridge STA" },
		{ ComFlags::MainInitializing, L"Main Initializing" },
	};

	CString result;
	for (const auto& attr : sflags)
		if ((attr.Flag & flags) == attr.Flag)
			(result += attr.Name) += ", ";

	if (!result.IsEmpty())
		result = result.Left(result.GetLength() - 2);
	return result;
}

PCWSTR FormatHelper::ComApartmentToString(ComFlags flags) {
	if (flags == ComFlags::Error)
		return L"";
	if ((flags & ComFlags::ASTA) == ComFlags::ASTA)
		return L"ASTA";
	if ((flags & ComFlags::STA) == ComFlags::STA)
		return L"STA";
	if ((flags & ComFlags::MTA) == ComFlags::MTA)
		return L"MTA";

	return L"";
}

CString FormatHelper::FormatHWndOrNone(HWND hWnd) {
	CString text;
	if (hWnd)
		text.Format(L"0x%zX", hWnd);
	else
		text = L"(None)";
	return text;
}

PCWSTR FormatHelper::ObjectTypeToString(int type) {
	static std::unordered_map<int, std::wstring> types;
	if (types.empty()) {
		const ULONG len = 1 << 14;
		BYTE buffer[len];
		if (0 != ::NtQueryObject(nullptr, ObjectTypesInformation, buffer, len, nullptr))
			return L"";

		auto p = reinterpret_cast<OBJECT_TYPES_INFORMATION*>(buffer);
		auto count = p->NumberOfTypes;
		types.reserve(count);

		auto raw = &p->TypeInformation[0];
		std::wstring typeName;
		for (ULONG i = 0; i < count; i++) {
			typeName = std::wstring(raw->TypeName.Buffer, raw->TypeName.Length / sizeof(WCHAR));
			types.insert({ raw->TypeIndex,typeName });
			auto temp = (BYTE*)raw + sizeof(OBJECT_TYPE_INFORMATION) + raw->TypeName.MaximumLength;
			temp += sizeof(PVOID) - 1;
			raw = reinterpret_cast<OBJECT_TYPE_INFORMATION*>((ULONG_PTR)temp / sizeof(PVOID) * sizeof(PVOID));
		}
	}
	return types.empty() ? L"" : types[type].c_str();
}

CString FormatHelper::MajorFunctionToString(UCHAR mf) {
	static PCWSTR major[] = {
		L"CREATE",
		L"CREATE_NAMED_PIPE",
		L"CLOSE",
		L"READ",
		L"WRITE",
		L"QUERY_INFORMATION",
		L"SET_INFORMATION",
		L"QUERY_EA",
		L"SET_EA",
		L"FLUSH_BUFFERS",
		L"QUERY_VOLUME_INFORMATION",
		L"SET_VOLUME_INFORMATION",
		L"DIRECTORY_CONTROL",
		L"FILE_SYSTEM_CONTROL",
		L"DEVICE_CONTROL",
		L"INTERNAL_DEVICE_CONTROL",
		L"SHUTDOWN",
		L"LOCK_CONTROL",
		L"CLEANUP",
		L"CREATE_MAILSLOT",
		L"QUERY_SECURITY",
		L"SET_SECURITY",
		L"POWER",
		L"SYSTEM_CONTROL",
		L"DEVICE_CHANGE",
		L"QUERY_QUOTA",
		L"SET_QUOTA",
		L"PNP"
	};

	static PCWSTR major_flt[] = {
		L"ACQUIRE_FOR_SECTION_SYNCHRONIZATION",
		L"RELEASE_FOR_SECTION_SYNCHRONIZATION",
		L"ACQUIRE_FOR_MOD_WRITE",
		L"RELEASE_FOR_MOD_WRITE",
		L"ACQUIRE_FOR_CC_FLUSH",
		L"RELEASE_FOR_CC_FLUSH",
		L"QUERY_OPEN",
	};

	static PCWSTR major_flt2[] = {
		L"FAST_IO_CHECK_IF_POSSIBLE",
		L"NETWORK_QUERY_OPEN",
		L"MDL_READ",
		L"MDL_READ_COMPLETE",
		L"PREPARE_MDL_WRITE",
		L"MDL_WRITE_COMPLETE",
		L"VOLUME_MOUNT",
		L"VOLUME_DISMOUNT"
	};

	CString text;
	if (mf < _countof(major))
		text.Format(L"IRP_MJ_%s (%d)", major[mf], (int)mf);
	else if (mf >= 255 - 7)
		text.Format(L"IRP_MJ_%s (%d)", major_flt[255 - mf - 1], (int)mf);
	else if (mf >= 255 - 20)
		text.Format(L"IRP_MJ_%s (%d)", major_flt2[255 - mf - 7], (int)mf);
	else
		text.Format(L"%d", (int)mf);
	return text;
}

CString FormatHelper::VirtualAllocFlagsToString(DWORD flags, bool withNumeric) {
	CString text;
	if (flags & MEM_COMMIT)
		text += L"MEM_COMMIT | ";
	if (flags & MEM_RESERVE)
		text += L"MEM_RESERVE | ";

	if (!text.IsEmpty())
		text = text.Left(text.GetLength() - 3);

	if (withNumeric)
		text.Format(L"%s (0x%X)", text, flags);

	return text;
}

CString FormatHelper::FormatTime(LONGLONG ts) {
	CString text;
	text.Format(L".%06u", (ts / 10) % 1000000);
	return CTime(*(FILETIME*)&ts).Format(L"%x %X") + text;
}

uint64_t GetBootTime() {
	static int64_t time = 0;
	if (time == 0) {
		SYSTEM_TIMEOFDAY_INFORMATION info;
		if (0 == ::NtQuerySystemInformation(SystemTimeOfDayInformation, &info, sizeof(info), nullptr))
			time = info.BootTime.QuadPart;
	}

	return time;
}

std::wstring FormatHelper::FormatProperty(const EventData* data, const EventProperty& prop) {
	static const auto statusFunction = [](auto, auto& p) {
		CString result;
		ATLASSERT(p.GetLength() == sizeof(DWORD));
		result.Format(L"%0x%08X", p.GetValue<DWORD>());
		return (PCWSTR)result;
	};

	static const auto int64ToHex = [](auto, auto& p)->std::wstring {
		ATLASSERT(p.GetLength() == sizeof(LONGLONG));
		CString text;
		text.Format(L"0x%llX", p.GetValue<LONGLONG>());
		return (PCWSTR)text;
	};

	static const auto initialTimeToString = [](auto, auto& p) -> std::wstring {
		ATLASSERT(p.GetLength() == sizeof(LONGLONG));
		auto time = p.GetValue<time_t>();
		auto initTime = GetBootTime() + time;
		return std::wstring(FormatTime(initTime));
	};

	static const std::unordered_map<std::wstring,
		std::function<std::wstring(const EventData*, const EventProperty&)>> functions{
		{ L"Status", statusFunction },
		{ L"NtStatus", statusFunction },
		{ L"InitialTime", initialTimeToString },
		{ L"TimeDateStamp", statusFunction },
		{ L"PageFault/VirtualAlloc;Flags", [](auto, auto& p) -> std::wstring {
			ATLASSERT(p.GetLength() == sizeof(DWORD));
			return (PCWSTR)VirtualAllocFlagsToString(p.GetValue<DWORD>(), true);
			} },
		{ L"MajorFunction", [](auto, auto& p) -> std::wstring {
			ATLASSERT(p.GetLength() == sizeof(DWORD));
			return (PCWSTR)MajorFunctionToString((UCHAR)p.GetValue<DWORD>());
			} },
		{ L"FileName", [](auto data, auto& p) -> std::wstring {
			auto value = data->FormatProperty(p);
			if (value[0] == L'\\')
				value = TraceManager::GetDosNameFromNtName(value.c_str());
			return value;
			} },
		{ L"ObjectType", [](auto, auto& p) -> std::wstring {
			ATLASSERT(p.GetLength() == sizeof(USHORT));
			auto type = p.GetValue<USHORT>();
			return std::to_wstring(type) + L" (" + ObjectTypeToString(type) + L")";
			} },
		{ L"Tag", [](auto data, auto& p) -> std::wstring {
			if (p.GetLength() != sizeof(DWORD))
				return data->FormatProperty(p);
			auto tag = p.GetValue<DWORD>();
			auto chars = (const char*)&tag;
			CStringA str(chars[0]);
			((str += chars[1]) += chars[2]) += chars[3];
			CStringA text;
			text.Format("%s (0x%X)", str, tag);
			return std::wstring(CString(text));
			} },
	};

	auto it = functions.find(data->GetEventName() + L";" + prop.Name);
	if (it == functions.end())
		it = functions.find(prop.Name);
	if (it == functions.end())
		return L"";

	return (it->second)(data, prop);
}