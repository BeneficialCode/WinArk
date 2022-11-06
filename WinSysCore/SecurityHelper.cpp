#include "pch.h"
#include "SecurityHelper.h"
#include <sddl.h>
#include <shellapi.h>
#include "DriverHelper.h"
#include <WtsApi32.h>
#include <WinBase.h>

#pragma comment(lib,"wtsapi32")

bool SecurityHelper::IsSysRun() {
	static bool runningSystem = false;
	static bool runningSystemCheck = false;
	if (runningSystemCheck)
		return runningSystem;

	runningSystemCheck = true;
	wil::unique_handle hToken;
	if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, hToken.addressof()))
		return false;

	BYTE buffer[128];
	DWORD len;
	if (::GetTokenInformation(hToken.get(), TokenUser, &buffer, sizeof(buffer), &len)) {
		auto user = reinterpret_cast<TOKEN_USER*>(buffer);
		runningSystem = IsSystemSid(user->User.Sid);
	}
	return runningSystem;
}

bool SecurityHelper::IsSystemSid(PSID sid) {
	return ::IsWellKnownSid(sid, WinLocalSystemSid);
}

bool SecurityHelper::IsRunningElevated() {
	static bool runningElevated = false;
	static bool runningElevatedCheck = false;
	if (runningElevatedCheck)
		return runningElevated;

	runningElevatedCheck = true;
	wil::unique_handle hToken;
	if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, hToken.addressof()))
		return false;

	TOKEN_ELEVATION te;
	DWORD len;
	if (::GetTokenInformation(hToken.get(), TokenElevation, &te, sizeof(te), &len)) {
		runningElevated = te.TokenIsElevated ? true : false;
	}
	return runningElevated;
}

HICON SecurityHelper::GetShieldIcon() {
	SHSTOCKICONINFO ssii = { sizeof(ssii) };
	if(FAILED(::SHGetStockIconInfo(SIID_SHIELD,SHGSI_SMALLICON|SHGSI_ICON,&ssii)))
		return nullptr;

	return ssii.hIcon;
}

bool SecurityHelper::RunElevated(PCWSTR param, bool ui) {
	WCHAR path[MAX_PATH];
	::GetModuleFileName(nullptr, path, _countof(path));

	SHELLEXECUTEINFO shi = { sizeof(shi) };
	shi.lpFile = path;
	shi.nShow = SW_SHOWDEFAULT;
	shi.lpVerb = L"runas";
	shi.lpParameters = param;
	shi.fMask = (ui ? 0 : (SEE_MASK_FLAG_NO_UI | SEE_MASK_NO_CONSOLE)) | SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS;
	auto ok = ::ShellExecuteEx(&shi);
	if (!ok)
		return false;

	DWORD rc = WAIT_OBJECT_0;
	if (!ui) {
		rc = ::WaitForSingleObject(shi.hProcess, 5000);
	}
	::CloseHandle(shi.hProcess);
	return rc == WAIT_OBJECT_0;
}

// find first SYSTEM process with a good token to use
HANDLE SecurityHelper::OpenSystemProcessToken() {
	PWTS_PROCESS_INFO pInfo;
	DWORD count;
	if (!::WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pInfo, &count)) {
		return nullptr;
	}

	HANDLE hToken{};
	for (DWORD i = 0; i < count && !hToken; i++) {
		if (pInfo[i].SessionId == 0 && IsSystemSid(pInfo[i].pUserSid)
			&& !_wcsicmp(pInfo[i].pProcessName,L"services.exe")) {
			auto hProcess = DriverHelper::OpenProcess(pInfo[i].ProcessId, PROCESS_QUERY_INFORMATION);
			if (hProcess) {
				::OpenProcessToken(hProcess, TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_QUERY | TOKEN_IMPERSONATE, &hToken);
				::CloseHandle(hProcess);
			}
		}
	}

	::WTSFreeMemory(pInfo);
	return hToken;
}

bool SecurityHelper::SysRun(PCWSTR param) {
	auto hToken = OpenSystemProcessToken();
	if (!hToken)
		return false;

	HANDLE hDupToken, hPrimary;
	::DuplicateTokenEx(hToken, TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_PRIVILEGES,
		nullptr, SecurityImpersonation, TokenImpersonation, &hDupToken);
	::DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, nullptr, SecurityImpersonation, TokenPrimary, &hPrimary);
	::CloseHandle(hToken);

	if (hDupToken == nullptr) {
		return false;
	}

	WCHAR path[MAX_PATH];
	::GetModuleFileName(nullptr, path, _countof(path));

	STARTUPINFO si = { sizeof(si) };
	si.lpDesktop = const_cast<wchar_t*>(L"Winsta0\\default");

	PROCESS_INFORMATION pi;
	BOOL impersonated = ::SetThreadToken(nullptr, hDupToken);
	if (!impersonated)
		return false;

	HANDLE hCurrentToken = INVALID_HANDLE_VALUE;
	DWORD session = 0, len = sizeof(session);
	bool ok = ::OpenProcessToken(::GetCurrentProcess(), TOKEN_ALL_ACCESS, &hCurrentToken);
	if (!ok)
		return false;
	::GetTokenInformation(hCurrentToken, TokenSessionId, &session, len, &len);
	::CloseHandle(hCurrentToken);

	if (!SetPrivilege(hDupToken, SE_ASSIGNPRIMARYTOKEN_NAME, true) ||
		!SetPrivilege(hDupToken, SE_INCREASE_QUOTA_NAME, true)) {
		return false;
	}

	std::wstring commandLine = path;
	commandLine += L" ";
	commandLine += param;

	ok = ::SetTokenInformation(hPrimary, TokenSessionId, &session, sizeof(session));

	GetCurrentDirectory(MAX_PATH, path);
	if (!::CreateProcessAsUser(hPrimary, nullptr,
		const_cast<wchar_t*>(commandLine.c_str()), nullptr, nullptr, 
		FALSE, 0, nullptr,path,
		&si, &pi)) {
		DWORD error = ::GetLastError();
		if (error == ERROR_PRIVILEGE_NOT_HELD) {
			if (!SetPrivilege(hDupToken, SE_IMPERSONATE_NAME, true))
				return false;
			if (!::CreateProcessWithTokenW(hPrimary, 0, nullptr,
				const_cast<wchar_t*>(commandLine.c_str()), 0, nullptr, path,
				&si, &pi)) {
				return false;
			}
		}
		else
			return false;
	}
	::CloseHandle(hPrimary);
	::CloseHandle(hDupToken);

	return true;
}

bool SecurityHelper::EnablePrivilege(PCWSTR privName, bool enable) {
	HANDLE hToken;
	if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
		return false;

	bool result = false;
	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;
	if (::LookupPrivilegeValue(nullptr, privName, &tp.Privileges[0].Luid)) {
		if (::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr))
			result = ::GetLastError() == ERROR_SUCCESS;
	}
	::CloseHandle(hToken);
	return result;
}

bool SecurityHelper::SetPrivilege(HANDLE hToken, PCWSTR privName, bool enable) {
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (!::LookupPrivilegeValue(nullptr, privName, &luid))
		return false;

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (enable)
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes = 0;

	// Enable the privilege or disable all privileges.
	if (!::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr)) {
		return false;
	}

	if (::GetLastError() == ERROR_NOT_ALL_ASSIGNED)
		return false;

	return true;
}

std::wstring SecurityHelper::GetSidFromUser(PCWSTR name) {
	BYTE sid[SECURITY_MAX_SID_SIZE];
	DWORD size = sizeof(sid);
	SID_NAME_USE use;
	WCHAR domain[64];
	DWORD domainSize = _countof(domain);
	if (!::LookupAccountName(nullptr, name, (PSID)sid, &size, domain, &domainSize, &use))
		return L"";

	PWSTR ssid;
	if (::ConvertSidToStringSid((PSID)sid, &ssid)) {
		std::wstring result(ssid);
		::LocalFree(ssid);
		return result;
	}
	return L"";
}

HANDLE SecurityHelper::DupHandle(HANDLE hSource, DWORD sourcePid, DWORD access) {
	auto h = DriverHelper::DupHandle(hSource, sourcePid, access, access == 0 ? DUPLICATE_SAME_ACCESS : 0);
	if (!h) {
		wil::unique_handle hProcess(::OpenProcess(PROCESS_DUP_HANDLE, FALSE, sourcePid));
		if (!hProcess)
			return nullptr;

		::DuplicateHandle(hProcess.get(), hSource, ::GetCurrentProcess(), &h, access, FALSE,
			access == 0 ? DUPLICATE_SAME_ACCESS : 0);
	}
	return h;
}