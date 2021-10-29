// WinArk.cpp : main source file for WinArk.exe
//

#include "stdafx.h"

#include "resource.h"
#include "Table.h"

#include "aboutdlg.h"
#include "MainFrame.h"
#include "DriverHelper.h"

CAppModule _Module;


int Run(LPTSTR lpstrCmdLine = nullptr, int nCmdShow = SW_SHOWDEFAULT) {
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

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

