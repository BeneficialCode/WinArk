// WinArk.cpp : main source file for WinArk.exe
//

#include "stdafx.h"

#include "resource.h"
#include "Table.h"

#include "aboutdlg.h"
#include "MainDlg.h"
#include "DriverHelper.h"

CAppModule _Module;


int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT) {
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	InitColorSys();
	InitFontSys();
	InitPenSys();
	InitBrushSys();
	InitSchemSys();

	CMainDlg dlgMain;

	if (dlgMain.Create(NULL) == NULL) {
		ATLTRACE(_T("Main dialog creation failed!\n"));
		return 0;
	}

	dlgMain.ShowWindow(nCmdShow);

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
	if (CheckInstall(lpstrCmdLine))
		return 0;
	HRESULT hRes = ::CoInitialize(NULL);
	ATLASSERT(SUCCEEDED(hRes));

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls
	
	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}

