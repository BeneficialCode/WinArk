#include "stdafx.h"
#include "TaskHelper.h"
#include "ComHelper.h"

HRESULT TaskSchedSvc::Init(PCWSTR machineName, PCWSTR userName, 
	PCWSTR domain, PCWSTR password) {
	auto hr = _svc.CoCreateInstance(__uuidof(TaskScheduler));
	if (FAILED(hr))
		return hr;

	hr = _svc->Connect(CComVariant(machineName), CComVariant(userName),
		CComVariant(domain), CComVariant(password));
	if (FAILED(hr))
		return hr;

	return S_OK;
}

PCWSTR TaskHelper::TaskStateToString(TASK_STATE state) {
	switch (state) {
	case TASK_STATE_DISABLED: return L"Disabled";
	case TASK_STATE_RUNNING: return L"Running";
	case TASK_STATE_QUEUED: return L"Queued";
	case TASK_STATE_READY: return L"Ready";
	}
	return L"(Unknown)";
}

PCWSTR TaskHelper::TriggerTypeToString(TASK_TRIGGER_TYPE2 type) {
	switch (type) {
	case TASK_TRIGGER_EVENT: return L"Event";
	case TASK_TRIGGER_TIME: return L"Time";
	case TASK_TRIGGER_DAILY: return L"Daily";
	case TASK_TRIGGER_WEEKLY: return L"Weekly";
	case TASK_TRIGGER_MONTHLY: return L"Monthly";
	case TASK_TRIGGER_MONTHLYDOW: return L"Monthly Day of Week";
	case TASK_TRIGGER_IDLE: return L"Idle";
	case TASK_TRIGGER_REGISTRATION: return L"Registration";
	case TASK_TRIGGER_BOOT: return L"Boot";
	case TASK_TRIGGER_LOGON: return L"Logon";
	case TASK_TRIGGER_SESSION_STATE_CHANGE: return L"Session State Change";
	case TASK_TRIGGER_CUSTOM_TRIGGER_01: return L"Custom";
	}
	return L"(Unknown)";
}

CString TaskHelper::GetProcessName(DWORD pid) {
	auto hProcess = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (!hProcess)
		return L"";

	WCHAR path[MAX_PATH *2];
	DWORD len = _countof(path);
	auto count = ::QueryFullProcessImageName(hProcess, 0, path, &len);
	::CloseHandle(hProcess);
	if (count) {
		return wcsrchr(path, L'\\') + 1;
	}
	return L"";
}

CString TaskHelper::FormatErrorMessage(DWORD error) {
	LPTSTR buffer;
	if (::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, error, 0, (LPTSTR)&buffer, 0, nullptr)) {
		CString msg(buffer);
		::LocalFree(buffer);
		return msg;
	}

	return L"";
}

CString TaskHelper::CreateDateTimeToString(PCWSTR dt) {
	auto dts = CString(dt);
	SYSTEMTIME st = { 0 };
	st.wYear = _wtoi(dts.Left(4));
	st.wMonth = _wtoi(dts.Mid(5, 2));
	st.wDay = _wtoi(dts.Mid(8, 2));
	st.wHour = _wtoi(dts.Mid(11, 2));
	st.wMinute = _wtoi(dts.Mid(14, 2));
	st.wSecond = _wtoi(dts.Mid(17, 2));

	return CTime(st).Format(L"%x %X");
}

CComPtr<IRegistrationInfo> TaskHelper::GetRegistrationInfo(IRegisteredTask* task) {
	CComPtr<ITaskDefinition> spDef;
	task->get_Definition(&spDef);
	CComPtr<IRegistrationInfo> spInfo;
	spDef->get_RegistrationInfo(&spInfo);
	return spInfo;
}

CString TaskHelper::GetTaskTriggers(IRegisteredTask* task) {
	CComPtr<ITaskDefinition> spDef;
	task->get_Definition(&spDef);
	CComPtr<ITriggerCollection> spColl;
	spDef->get_Triggers(&spColl);
	CString triggers;
	int count = 0;
	ComHelper::DoForeachNoVariant<ITriggerCollection, ITrigger>(spColl, [&](auto trigger) {
		TASK_TRIGGER_TYPE2 type;
		trigger->get_Type(&type);
		triggers += TriggerTypeToString(type);
		triggers += L", ";
		count++;
		});

	if (count > 0) {
		CString temp;
		temp.Format(L"(%d) %s", count, (PCWSTR)triggers);
		return temp.Left(temp.GetLength() - 2);
	}
	return L"";
}

CString TaskHelper::GetRepeatPattern(IRepetitionPattern* rep) {
	CComBSTR duration, interval;
	rep->get_Duration(&duration);
	CString text;
	if (duration.Length())
		text += L"Duration: " + CString(duration) + L" ";
	rep->get_Interval(&interval);
	if (interval.Length())
		text += L"Interval: " + CString(interval) + L" ";
	VARIANT_BOOL stopAtEnd;
	rep->get_StopAtDurationEnd(&stopAtEnd);

	return text + (stopAtEnd ? L"(Stop at duration end)" : L"");
}

PCWSTR TaskHelper::ActionTypeToString(TASK_ACTION_TYPE type) {
	switch (type) {
	case TASK_ACTION_EXEC: return L"Run Program";
	case TASK_ACTION_COM_HANDLER: return L"COM Handler";
	case TASK_ACTION_SEND_EMAIL: return L"Send Email";
	case TASK_ACTION_SHOW_MESSAGE: return L"Show Message";
	}
	return L"(Unknown)";
}

PCWSTR TaskHelper::StateChangeToString(TASK_SESSION_STATE_CHANGE_TYPE type) {
	switch (type) {
	case TASK_CONSOLE_CONNECT: return L"Console Connect";
	case TASK_CONSOLE_DISCONNECT: return L"Console Disconnect";
	case TASK_REMOTE_CONNECT: return L"Remote Connect";
	case TASK_REMOTE_DISCONNECT: return L"Remote Disconnect";
	case TASK_SESSION_LOCK: return L"Lock";
	case TASK_SESSION_UNLOCK: return L"Unlock";
	}
	return L"(Unknown)";
}