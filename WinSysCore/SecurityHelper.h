#pragma once

struct SecurityHelper final {
	static bool IsRunningElevated();
	static HICON GetShieldIcon();
	static bool RunElevated(PCWSTR privName, bool enable);
	static bool EnablePrivilege(PCWSTR privName, bool enable);
	static std::wstring GetSidFromUser(PCWSTR name);
	static HANDLE DupHandle(HANDLE hSoruce, DWORD sourcePid, DWORD access = 0);
};