// WinArk.cpp : main source file for WinArk.exe
//

#include "stdafx.h"

#include "resource.h"
#include "Table.h"

#include "aboutdlg.h"
#include "MainFrame.h"
#include "DriverHelper.h"
#include "SymbolFileInfo.h"
#include "PEParser.h"
#include <filesystem>
#include <Helpers.h>
#include "SymbolHelper.h"
#include "SecurityHelper.h"

CAppModule _Module;
HWND _hMainWnd;
AppSettings _Settings;

bool g_hasSymbol = true;
HANDLE g_hSingleInstMutex{ nullptr };

void InitSymbols(std::wstring fileName) {
	WCHAR path[MAX_PATH];
	::GetSystemDirectory(path, MAX_PATH);
	wcscat_s(path, L"\\");
	wcscat_s(path, fileName.c_str());
	PEParser parser(path);
	auto dir = parser.GetDataDirectory(IMAGE_DIRECTORY_ENTRY_DEBUG);
	if (dir != nullptr) {
		SymbolFileInfo info;
		auto entry = static_cast<PIMAGE_DEBUG_DIRECTORY>(parser.GetAddress(dir->VirtualAddress));
		ULONG_PTR VA = reinterpret_cast<ULONG_PTR>(parser.GetBaseAddress());
		info.GetPdbSignature(VA, entry);
		::GetCurrentDirectory(MAX_PATH, path);
		wcscat_s(path, L"\\Symbols");
		bool success = info.SymDownloadSymbol(path);
		if (!success)
			g_hasSymbol = false;
	}
	else {
		g_hasSymbol = false;
	}
}

void ClearSymbols() {
	WCHAR path[MAX_PATH];
	::GetCurrentDirectory(MAX_PATH, path);
	wcscat_s(path, L"\\Symbols");
	std::filesystem::remove_all(path);
}

// {A67605F7-BCFA-47F0-970D-92799D8F375E}
static const GUID iconGuid =
{ 0xa67605f7, 0xbcfa, 0x47f0, { 0x97, 0xd, 0x92, 0x79, 0x9d, 0x8f, 0x37, 0x5e } };

bool ShowBalloonTip(PCWSTR title, PCWSTR text, ULONG timeout) {
	NOTIFYICONDATA notifyIcon = { sizeof(NOTIFYICONDATA) };
	

	notifyIcon.uFlags = NIF_INFO | NIF_GUID;
	notifyIcon.hWnd = _hMainWnd;
	notifyIcon.uID = IDR_MAINFRAME;
	notifyIcon.guidItem = iconGuid;
	wcsncpy_s(notifyIcon.szInfoTitle, RTL_NUMBER_OF(notifyIcon.szInfoTitle), title, _TRUNCATE);
	wcsncpy_s(notifyIcon.szInfo, RTL_NUMBER_OF(notifyIcon.szInfo), text, _TRUNCATE);
	notifyIcon.uTimeout = timeout;
	notifyIcon.dwInfoFlags = NIIF_INFO;

	return Shell_NotifyIcon(NIM_MODIFY, &notifyIcon);
}

bool ShowIconNotication(PCWSTR title, PCWSTR text) {
	bool success = ShowBalloonTip(title, text, 10);
	return success;
}

bool AddNotifyIcon() {
	NOTIFYICONDATA notifyIcon = { sizeof(NOTIFYICONDATA) };
	notifyIcon.hWnd = _hMainWnd;
	notifyIcon.uID = IDR_MAINFRAME;
	notifyIcon.uFlags = NIF_ICON | NIF_GUID | NIF_TIP;
	notifyIcon.guidItem = iconGuid;

	return Shell_NotifyIcon(NIM_ADD, &notifyIcon);
}

bool RemoveNotifyIcon() {
	NOTIFYICONDATA notifyIcon = { sizeof(NOTIFYICONDATA) };

	notifyIcon.uFlags = NIF_GUID;
	notifyIcon.hWnd = _hMainWnd;
	notifyIcon.uID = IDR_MAINFRAME;
	notifyIcon.guidItem = iconGuid;

	return Shell_NotifyIcon(NIM_DELETE, &notifyIcon);
}

int Run(LPTSTR lpstrCmdLine = nullptr, int nCmdShow = SW_SHOWDEFAULT) {
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	HANDLE hThread = ::CreateThread(nullptr, 0, [](auto param)->DWORD {
		std::string name = Helpers::GetNtosFileName();
		std::wstring osFileName = Helpers::StringToWstring(name);
		InitSymbols(osFileName.c_str());
		if (!g_hasSymbol)
			return -1;
		InitSymbols(L"user32.dll");
		if (!g_hasSymbol)
			return -1;
		InitSymbols(L"ntdll.dll");
		if (!g_hasSymbol)
			return -1;
		InitSymbols(L"win32k.sys");
		if (!g_hasSymbol)
			return -1;
		InitSymbols(L"ci.dll");
		if (!g_hasSymbol)
			return -1;
		InitSymbols(L"drivers\\fltmgr.sys");
		if (!g_hasSymbol)
			return -1;


		return 0;
		}, nullptr, 0, nullptr);

	::WaitForSingleObject(hThread, INFINITE);
	if (!g_hasSymbol||NULL == hThread) {
		return 0;
	}
	::CloseHandle(hThread);


	SymbolHelper::Init();

	InitColorSys();
	InitFontSys();
	InitPenSys();
	InitBrushSys();
	InitSchemSys();

	CMainFrame wndMain;

	/*CString cmdLine(lpstrCmdLine);
	cmdLine.Trim(L" \"");
	if(!cmdLine.IsEmpty()&&cmdLine.Right(11).CompareNoCase(L"regedit.exe")!=0)*/
		
	// CreateEx才会加载 IDR_MAINFRAME相关的资源
	_hMainWnd = wndMain.CreateEx(NULL);
	if (_hMainWnd == NULL) {
		ATLTRACE(_T("Main dialog creation failed!\n"));
		return 0;
	}
	AddNotifyIcon();

	ShowIconNotication(L"WinArk Initialization", L"WinArk Initialize successfully!");

	wndMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	RemoveNotifyIcon();
	_Module.RemoveMessageLoop();
	return nRet;
}

bool CheckInstall(PCWSTR cmdLine) {
	bool parse = false, success = false;

	auto hRes = ::FindResource(nullptr, MAKEINTRESOURCE(IDR_DRIVER), L"BIN");

	if (!hRes)
		return false;

	auto hGlobal = ::LoadResource(nullptr, hRes);
	if (!hGlobal)
		return false;

	auto size = ::SizeofResource(nullptr, hRes);
	void* pBuffer = ::LockResource(hGlobal);

	if (::wcsstr(cmdLine, L"install")) {
		parse = true;
		success = DriverHelper::LoadDriver();
		if (!success)
			if (DriverHelper::InstallDriver(false,pBuffer,size))
				success = DriverHelper::LoadDriver();
		if (!success)
			AtlMessageBox(nullptr, L"Failed to install/load kernel driver", IDS_TITLE, MB_ICONERROR);
	}
	else if (::wcsstr(cmdLine, L"update")) {
		parse = true;
		success = DriverHelper::UpdateDriver(pBuffer,size);
		if (!success) {
			AtlMessageBox(nullptr, L"Failed to update kernel driver", IDS_TITLE, MB_ICONERROR);
		}
	}
	return parse;
}

LONG WINAPI SelfUnhandledExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo)
{
	if (EXCEPTION_ACCESS_VIOLATION == ExceptionInfo->ExceptionRecord->ExceptionCode) {
		// probably the network related thread failing during symbol loading when terminated abruptly
		::ExitThread(0);
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow) {
	g_hSingleInstMutex = ::CreateMutex(nullptr, FALSE, L"WinArkSingleInstanceMutex");
	if (!::wcsstr(lpstrCmdLine, L"runas")) {
		if (g_hSingleInstMutex) {
			if (::GetLastError() == ERROR_ALREADY_EXISTS) {
				MessageBox(nullptr, L"Please do not double start!!!", L"Error", MB_ICONERROR);
				return -1;
			}
		}
	}
	HRESULT hRes = ::CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE);
	ATLASSERT(SUCCEEDED(hRes));
	// add flags to support other controls
	AtlInitCommonControls(ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES);
	
	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));
	::SetPriorityClass(::GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	::SetUnhandledExceptionFilter(SelfUnhandledExceptionFilter);

	SecurityHelper::EnablePrivilege(SE_SYSTEM_ENVIRONMENT_NAME, true);

	if (CheckInstall(lpstrCmdLine))
		return 0;

	::SymSetOptions(SYMOPT_UNDNAME | SYMOPT_CASE_INSENSITIVE |
		SYMOPT_AUTO_PUBLICS | SYMOPT_INCLUDE_32BIT_MODULES |
		SYMOPT_OMAP_FIND_NEAREST);
	::SymInitialize(::GetCurrentProcess(), nullptr, TRUE);

	int nRet = Run(lpstrCmdLine, nCmdShow);
	
	DriverHelper::LoadDriver(false);

	_Module.Term();
	::CoUninitialize();
	::CloseHandle(g_hSingleInstMutex);

	::SymCleanup(::GetCurrentProcess());

	return nRet;
}

