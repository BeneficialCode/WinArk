#pragma once

struct SecurityHelper final {
	static bool IsSysRun();
	static bool IsRunningElevated();
	static HICON GetShieldIcon();
	static bool IsSystemSid(PSID sid);
	static bool RunElevated(PCWSTR param, bool ui);
	static bool SysRun(PCWSTR param);
	static HANDLE OpenSystemProcessToken();
	static bool EnablePrivilege(PCWSTR privName, bool enable);
	static std::wstring GetSidFromUser(PCWSTR name);
	static HANDLE DupHandle(HANDLE hSoruce, DWORD sourcePid, DWORD access = 0);
	static bool SetPrivilege(HANDLE hToken, PCWSTR privName, bool enable);
};