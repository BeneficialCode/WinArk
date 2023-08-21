#pragma once

struct TaskSchedSvc {
	HRESULT Init(PCWSTR machineName = nullptr, PCWSTR userName = nullptr,
		PCWSTR domain = nullptr,PCWSTR password = nullptr);

	ITaskService* operator->() const {
		return _svc.p;
	}

private:
	CComPtr<ITaskService> _svc;
};

struct TaskHelper final {
	static PCWSTR TaskStateToString(TASK_STATE state);
	static PCWSTR TriggerTypeToString(TASK_TRIGGER_TYPE2 type);
	static CString GetProcessName(DWORD pid);
	static CString FormatErrorMessage(DWORD error);
	static CString CreateDateTimeToString(PCWSTR dt);
	static CComPtr<IRegistrationInfo> GetRegistrationInfo(IRegisteredTask* task);
	static CString GetTaskTriggers(IRegisteredTask* task);
	static CString GetRepeatPattern(IRepetitionPattern* rep);
	static PCWSTR ActionTypeToString(TASK_ACTION_TYPE type);
	static PCWSTR StateChangeToString(TASK_SESSION_STATE_CHANGE_TYPE type);
};