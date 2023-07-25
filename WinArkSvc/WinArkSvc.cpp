// WinArkSvc.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "pch.h"
#include "ServiceCommon.h"

#pragma comment(lib,"wtsapi32")

int Error(const char* msg, DWORD error = ::GetLastError());
void WINAPI ServiceMain(DWORD dwNumServicesArgs, LPTSTR* args);
DWORD WINAPI ServiceHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
void SetStatus(DWORD status);
void HandleMessage(const AlarmMessage& msg);
void NTAPI OnTimerExpired(
	_Inout_     PTP_CALLBACK_INSTANCE Instance,
	_Inout_opt_ PVOID                 Context,
	_Inout_     PTP_TIMER             Timer);
int HandleCommandLine(int argc, const char* argv[]);
bool IsRunningElevated();

// service control
int InstallService();
int StartWinArkService();
int StopService();
int UninstallService();


SERVICE_STATUS_HANDLE g_hService;
SERVICE_STATUS g_Status;
HANDLE g_hStopEvent;
HANDLE g_hMainsolt;
PTP_TIMER g_Timer;

int Error(const char* msg, DWORD error) {
	printf("%s (%u)\n", msg, error);
	return 1;
}

int main(int argc, const char* argv[]) {
	if (argc > 1)
		return HandleCommandLine(argc, argv);

	// Note that the service name is typed as LPTSTR, meaning it should not be part of readonly
	// memory, and that¡¯s why the name is first set to a variable on the stack(which is
	// always read / write), before passing it to the structure.
	WCHAR name[] = L"winark";

	const SERVICE_TABLE_ENTRY table[] = {
		{name,ServiceMain},
		{nullptr,nullptr}
	};

	if (!::StartServiceCtrlDispatcher(table))
		return 1;

	return 0;
}

void WINAPI ServiceMain(DWORD dwNumServicesArgs, LPTSTR* args) {
	g_Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_Status.dwWaitHint = 500;

	bool error = true;
	do {	// non-loop for easy breaking
		g_hService = ::RegisterServiceCtrlHandlerEx(L"WinArkSvc",
			ServiceHandler, nullptr);
		if (!g_hService)
			break;

		g_hStopEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (!g_hStopEvent)
			break;

		SetStatus(SERVICE_START_PENDING);

		// create a mailslotand configure
		// its security descriptor to allow full control to the user running the service, 
		// with all other users having write/read access only

		// create the Everyone SID
		// 
		BYTE worldSid[SECURITY_MAX_SID_SIZE];
		DWORD len = sizeof(worldSid);
		auto pWorldSid = (PSID)worldSid;

		if (!::CreateWellKnownSid(WinWorldSid, nullptr, pWorldSid, &len))
			break;

		//
		// get SID of the user running this process
		//
		HANDLE hToken;
		if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hToken))
			break;

		BYTE userBuffer[SECURITY_MAX_SID_SIZE + sizeof(TOKEN_USER) + sizeof(SID_AND_ATTRIBUTES)];
		auto user = (TOKEN_USER*)userBuffer;
		BOOL ok = ::GetTokenInformation(hToken, TokenUser, userBuffer, sizeof(userBuffer), &len);
		::CloseHandle(hToken);
		if (!ok)
			break;
		auto ownerSid = user->User.Sid;

		//
		// allocate and initialize a new security descriptor
		//
		PSECURITY_DESCRIPTOR sd = ::HeapAlloc(::GetProcessHeap(), 0, SECURITY_DESCRIPTOR_MIN_LENGTH);
		if (!sd)
			break;
		if (!::InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION)) {
			break;
		}

		//
		// build ACEs
		//
		EXPLICIT_ACCESS ea[2] = { 0 };
		ea[0].grfAccessPermissions = FILE_ALL_ACCESS;
		ea[0].grfAccessMode = SET_ACCESS;
		ea[0].grfInheritance = NO_INHERITANCE;
		ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea[0].Trustee.ptstrName = (PWSTR)ownerSid;

		ea[1].grfAccessPermissions =
			FILE_GENERIC_WRITE | FILE_GENERIC_READ;
		ea[1].grfAccessMode = SET_ACCESS;
		ea[1].grfInheritance = NO_INHERITANCE;
		ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea[1].Trustee.ptstrName = (PWSTR)pWorldSid;

		//
		// create DACL from ACEs
		//
		PACL dacl;
		if (ERROR_SUCCESS != ::SetEntriesInAcl(_countof(ea), ea,
			nullptr, &dacl))
			break;

		//
		// apply owner and DACL
		//
		if (!::SetSecurityDescriptorOwner(sd, ownerSid, FALSE))
			break;

		if (!::SetSecurityDescriptorDacl(sd, TRUE, dacl, FALSE))
			break;

		//
		// create mailsolt
		//
		SECURITY_ATTRIBUTES sa = { sizeof(sa) };
		sa.lpSecurityDescriptor = sd;
		g_hMainsolt = ::CreateMailslot(L"\\\\.\\mailslot\\winark", 1024,
			MAILSLOT_WAIT_FOREVER, &sa);
		if (g_hMainsolt == INVALID_HANDLE_VALUE)
			break;

		// 
		// cleanup
		//
		::HeapFree(::GetProcessHeap(), 0, sd);
		::LocalFree(dacl);
		error = false;
	} while (false);

	if (error) {
		// The most common error reporting
		// mechanism used by services is writing to the Windows event log.
		SetStatus(SERVICE_STOPPED);
		return;
	}

	SetStatus(SERVICE_RUNNING);

	DWORD msgSize, count;
	AlarmMessage msg;

	while (::WaitForSingleObject(g_hStopEvent, 1000) == WAIT_TIMEOUT) {
		do
		{
			if (!::GetMailslotInfo(g_hMainsolt, nullptr, &msgSize, &count, nullptr))
				break;
			if (msgSize == MAILSLOT_NO_MESSAGE)
				break;

			DWORD bytes;
			if (msgSize == sizeof(msg) &&
				::ReadFile(g_hMainsolt, &msg, sizeof(msg), &bytes, nullptr)) {
				HandleMessage(msg);
			}
			count--;
		} while (count > 0);
	}
	SetStatus(SERVICE_STOPPED);

	::CloseHandle(g_hStopEvent);
	::CloseHandle(g_hMainsolt);
}

void SetStatus(DWORD status) {
	g_Status.dwCurrentState = status;
	g_Status.dwControlsAccepted = status == SERVICE_RUNNING ? SERVICE_ACCEPT_STOP : 0;
	::SetServiceStatus(g_hService, &g_Status);
}

void HandleMessage(const AlarmMessage& msg) {
	switch (msg.Type)
	{
	case MessageType::SetAlarm:
		if (g_Timer == nullptr)
			g_Timer = ::CreateThreadpoolTimer(OnTimerExpired, nullptr, nullptr);
		::SetThreadpoolTimer(g_Timer, (PFILETIME)&msg.Time, 0, 1000);
		break;

	case MessageType::CancelAlarm:
		if (g_Timer) {
			::CloseThreadpoolTimer(g_Timer);
			g_Timer = nullptr;
		}
		break;
	default:
		break;
	}
}

_Use_decl_annotations_
void NTAPI OnTimerExpired(PTP_CALLBACK_INSTANCE, PVOID, PTP_TIMER) {
	WCHAR title[] = L"!!! ALARM !!!";
	WCHAR msg[] = L"It's time to wake up!";
	DWORD response;

	::WTSSendMessage(nullptr, ::WTSGetActiveConsoleSessionId(),
		title, sizeof(title), msg, sizeof(msg),
		MB_OK | MB_ICONEXCLAMATION, 0, &response, FALSE);
}

DWORD WINAPI ServiceHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext) {
	switch (dwControl)
	{
	case SERVICE_CONTROL_STOP:
		SetStatus(SERVICE_STOP_PENDING);
		::SetEvent(g_hStopEvent);
		break;
	}
	return 0;
}

bool IsRunningElevated() {
	HANDLE hToken;
	if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hToken))
		return false;

	ULONG elevated = 0;
	DWORD len;
	::GetTokenInformation(hToken, TokenElevation, &elevated, sizeof(elevated), &len);
	::CloseHandle(hToken);

	return elevated ? true : false;
}

int HandleCommandLine(int argc, const char* argv[]) {
	if (!IsRunningElevated()) {
		printf("You must run elevated to configure the service.\n");
		return 1;
	}

	if (!_stricmp(argv[1], "install") || !_stricmp(argv[1], "/install")
		|| !_stricmp(argv[1], "-install")) {
		bool start = argc > 2 && _stricmp(argv[2], "start") == 0;
		auto error = InstallService();
		if (error == 0 && start)
			return StartWinArkService();
		return error;
	}
	if (!_stricmp(argv[1], "uninstall") || !_stricmp(argv[1], "-uninstall")
		|| !_stricmp(argv[1], "/uinstall")) {
		return UninstallService();
	}
	if (!_stricmp(argv[1], "start") || !_stricmp(argv[1], "-start")) {
		return StartWinArkService();
	}
	if (!_stricmp(argv[1], "stop") || !_stricmp(argv[1], "-stop")) {
		return StopService();
	}
	return 0;
}

int InstallService() {
	SC_HANDLE hScm = ::OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
	if (!hScm)
		return Error("Failed to open SCM database");

	WCHAR path[MAX_PATH];
	::GetModuleFileName(nullptr, path, _countof(path));

	SC_HANDLE hService = ::CreateService(hScm,
		L"WinArkSvc",
		L"WinArk Demo Service",
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		path,
		L"Base",
		nullptr, nullptr, L"NT AUTHORITY\\LocalService", nullptr);
	if (!hService)
		return Error("Failed to install service");

	printf("Service installed successfully.\n");

	::CloseServiceHandle(hService);
	::CloseServiceHandle(hScm);

	return 0;
}

int StartWinArkService() {
	SC_HANDLE hScm = ::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
	if (!hScm)
		return Error("Failed to open SCM database");

	SC_HANDLE hService = ::OpenService(hScm, L"WinArkSvc", SERVICE_START);
	if (!hService)
		return Error("Failed to open service");

	if (::StartService(hService, 0, nullptr))
		printf("Service started successfully.\n");
	else
		return Error("Failed to start service");
	::CloseServiceHandle(hService);
	::CloseServiceHandle(hScm);

	return 0;
}

int StopService() {
	auto hScm = ::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
	if (!hScm)
		return Error("Failed to open SCM database");

	auto hService = ::OpenService(hScm, L"WinArkSvc", SERVICE_STOP);
	if (!hService)
		return Error("Failed to open service");

	if (::ControlService(hService, SERVICE_CONTROL_STOP, &g_Status))
		printf("Service stopped successfully.\n");
	else
		return Error("Failed to stop service");
	::CloseServiceHandle(hService);
	::CloseServiceHandle(hScm);

	return 0;
}

int UninstallService() {
	auto hScm = ::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
	if (!hScm)
		return Error("Failed to open SCM database");

	auto hService = ::OpenService(hScm, L"WinArkSvc", DELETE | SERVICE_QUERY_STATUS);
	if (!hService)
		return Error("Failed to open service");
	SERVICE_STATUS status;
	if (::QueryServiceStatus(hService, &status) && status.dwCurrentState == SERVICE_RUNNING)
		StopService();

	if (::DeleteService(hService))
		printf("Service uninstalled successfully.\n");
	else
		Error("Failed to uninstall service");
	::CloseServiceHandle(hService);
	::CloseServiceHandle(hScm);
	return 0;
}