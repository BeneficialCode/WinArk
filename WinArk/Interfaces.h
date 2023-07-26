#pragma once

class TraceManager;

struct AppSettings;

const DWORD ListViewDefaultStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA | LVS_SINGLESEL | LVS_SHAREIMAGELISTS;

struct IRegView abstract {
	virtual void OnFindStart() = 0;
	virtual void OnFindNext(PCWSTR path, PCWSTR name, PCWSTR data) = 0;
	virtual void OnFindEnd(bool cancelled) = 0;
	virtual bool GoToItem(PCWSTR path, PCWSTR name, PCWSTR data) = 0;
	virtual CString GetCurrentKeyPath() = 0;
	//virtual BOOL TrackPopupMenu(HMENU hMenu, DWORD flags, int x, int y, HWND hWnd = nullptr) = 0;
	virtual HWND GetHwnd() const = 0;
	virtual void DisplayError(PCWSTR text, HWND hWnd = nullptr, DWORD err = ::GetLastError()) const = 0;
	//virtual bool AddMenu(HMENU hMenu) = 0;
	virtual HTREEITEM GotoKey(CString const&, bool knownToExist) = 0;
};

struct IView {
	virtual bool IsFindSupported() const {
		return false;
	}
	virtual void DoFind(const CString& text,DWORD flags){}
};

struct QuickFindOptions {
	bool CaseSensitive : 1;
	bool SearchProcesses : 1{true};
	bool SearchEvents : 1{true};
	bool SearchDetails : 1;
	bool SearchDown : 1{true};
	bool FindNext : 1{true};
};

struct IMainFrame {
	virtual BOOL TrackPopupMenu(HMENU hMenu, HWND hWnd, POINT* pt = nullptr, UINT flags = 0) = 0;
	virtual void ViewDestroyed(void* p) = 0;
	virtual TraceManager& GetTraceManager() = 0;
	virtual HFONT GetMonoFont() = 0;
	virtual BOOL SetPaneText(int index, PCWSTR text) = 0;
	virtual BOOL SetPaneIcon(int index, HICON hIcon) = 0;
	virtual CUpdateUIBase* GetUpdateUI() = 0;
};

struct IQuickFind {
	virtual void DoFind(PCWSTR text, const QuickFindOptions& options) = 0;
	virtual void WindowClosed() = 0;
};