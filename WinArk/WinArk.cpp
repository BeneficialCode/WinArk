// WinArk.cpp : main source file for WinArk.exe
//

#include "stdafx.h"

#include "resource.h"
#include "Table.h"

#include "aboutdlg.h"
#include "MainFrame.h"
#include "DriverHelper.h"
#include "SymbolInfo.h"
#include "PEParser.h"
#include <filesystem>
#include <Helpers.h>

CAppModule _Module;

void InitSymbols(std::wstring fileName) {
	WCHAR path[MAX_PATH];
	::GetSystemDirectory(path, MAX_PATH);
	wcscat_s(path, L"\\");
	wcscat_s(path, fileName.c_str());
	PEParser parser(path);
	auto dir = parser.GetDataDirectory(IMAGE_DIRECTORY_ENTRY_DEBUG);
	SymbolInfo info;
	auto entry = static_cast<PIMAGE_DEBUG_DIRECTORY>(parser.GetAddress(dir->VirtualAddress));
	ULONG_PTR VA = reinterpret_cast<ULONG_PTR>(parser.GetBaseAddress());
	info.GetPdbSignature(VA, entry);
	::GetCurrentDirectory(MAX_PATH, path);
	wcscat_s(path, L"\\Symbols");
	std::filesystem::create_directory(path);
	info.SymDownloadSymbol(path);
}

int Run(LPTSTR lpstrCmdLine = nullptr, int nCmdShow = SW_SHOWDEFAULT) {
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	HANDLE hThread = ::CreateThread(nullptr, 0, [](auto param)->DWORD {
		std::string name = Helpers::GetNtosFileName();
		std::wstring osFileName(name.begin(), name.end());
		InitSymbols(osFileName.c_str());
		InitSymbols(L"user32.dll");
		InitSymbols(L"ntdll.dll");
		InitSymbols(L"win32k.sys");
		return 0;
		}, nullptr, 0, nullptr);

	::CloseHandle(hThread);

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
	if (wndMain.CreateEx(NULL) == NULL) {
		ATLTRACE(_T("Main dialog creation failed!\n"));
		return 0;
	}

	wndMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

bool CheckInstall(PCWSTR cmdLine) {
	bool parse = false, success = false;
#ifdef _WIN64
	auto hRes = ::FindResource(nullptr, MAKEINTRESOURCE(IDR_X64_DRIVER), L"BIN");
#else
	auto hRes = ::FindResource(nullptr, MAKEINTRESOURCE(IDR_X86_DRIVER), L"BIN");
#endif // __WIN64
	
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

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow) {
	HRESULT hRes = ::CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE);
	ATLASSERT(SUCCEEDED(hRes));
	// add flags to support other controls
	AtlInitCommonControls(ICC_BAR_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES);
	
	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	if (CheckInstall(lpstrCmdLine))
		return 0;

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}

