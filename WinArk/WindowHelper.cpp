#include "stdafx.h"
#include "ProcessHelper.h"
#include "WindowHelper.h"

CString WindowHelper::WindowStyleToString(HWND hWnd) {
	auto style = CWindow(hWnd).GetStyle();
	CString text;

	static struct {
		DWORD style;
		PCWSTR text;
	}styles[] = {
		{ WS_POPUP			, L"POPUP"},
		{ WS_CHILD			, L"CHILD"},
		{ WS_MINIMIZE		, L"MINIMIZE"},
		{ WS_VISIBLE		, L"VISIBLE"},
		{ WS_DISABLED		, L"DISABLED" },
		{ WS_CLIPSIBLINGS	, L"CLIPSIBLINGS" },
		{ WS_CLIPCHILDREN	, L"CLIPCHILDREN" },
		{ WS_MAXIMIZE		, L"MAXIMIZE" },
		{ WS_BORDER			, L"BORDER" },
		{ WS_DLGFRAME		, L"DLGFRAME" },
		{ WS_VSCROLL		, L"VSCROLL" },
		{ WS_HSCROLL		, L"HSCROLL" },
		{ WS_SYSMENU		, L"SYSMENU" },
		{ WS_THICKFRAME		, L"THICKFRAME" },
		{ WS_MINIMIZEBOX	, L"MINIMIZEBOX" },
		{ WS_MAXIMIZEBOX	, L"MAXIMIZEBOX" },
	};

	for (auto& item : styles) {
		if (style & item.style)
			text += CString(item.text) += L", ";
	}
	if (text.IsEmpty())
		text = L"OVERLAPPED, ";

	return L"(WS_) " + text.Left(text.GetLength() - 2);
}

CString WindowHelper::ClassStyleToString(HWND hWnd) {
	auto style = ::GetClassLongPtr(hWnd, GCL_STYLE);
	CString text;

	static struct {
		DWORD style;
		PCWSTR text;
	} styles[] = {
		{ CS_HREDRAW			, L"HREDRAW" },
		{ CS_VREDRAW			, L"VREDRAW" },
		{ CS_DBLCLKS			, L"DBLCLKS" },
		{ CS_OWNDC				, L"OWNDC" },
		{ CS_CLASSDC			, L"CLASSDC" },
		{ CS_PARENTDC			, L"PARENTDC" },
		{ CS_SAVEBITS			, L"SAVEBITS" },
		{ CS_GLOBALCLASS		, L"GLOBALCLASS" },
		{ CS_BYTEALIGNCLIENT	, L"BYTEALIGNCLIENT" },
		{ CS_BYTEALIGNWINDOW	, L"BYTEALIGWINDOW" },
	};

	for (auto& item : styles) {
		if (style & item.style)
			text += CString(item.text) += L", ";
	}
	if (text.IsEmpty())
		return L"";

	return L"(CS_) " + text.Left(text.GetLength() - 2);
}

CString WindowHelper::WindowExtendedStyleToString(HWND hWnd) {
	auto style = CWindow(hWnd).GetExStyle();
	CString text;

	static struct {
		DWORD style;
		PCWSTR text;
	} styles[] = {
		{ WS_EX_DLGMODALFRAME		, L"DLGMODALFRAME" },
		{ WS_EX_NOPARENTNOTIFY		, L"NOPARENTNOTIFY" },
		{ WS_EX_TOPMOST				, L"TOPMOST" },
		{ WS_EX_ACCEPTFILES			, L"ACCEPTFILES" },
		{ WS_EX_TRANSPARENT			, L"TRANSPARENT" },
		{ WS_EX_MDICHILD			, L"MDICHILD" },
		{ WS_EX_TOOLWINDOW			, L"TOOLWINDOW" },
		{ WS_EX_WINDOWEDGE			, L"WINDOWEDGE" },
		{ WS_EX_CLIENTEDGE			, L"CLIENTEDGE" },
		{ WS_EX_CONTEXTHELP			, L"CONTEXTHELP" },
		{ WS_EX_RIGHT				, L"RIGHT" },
		{ WS_EX_RTLREADING			, L"RTLREADING" },
		{ WS_EX_LTRREADING			, L"LTRREADING" },
		{ WS_EX_LEFTSCROLLBAR		, L"LEFTSCROLLBAR" },
		{ WS_EX_CONTROLPARENT		, L"CONTROLPARENT" },
		{ WS_EX_STATICEDGE			, L"STATICEDGE" },
		{ WS_EX_APPWINDOW			, L"APPWINDOW" },
		{ WS_EX_LAYERED				, L"LAYERED" },
		{ WS_EX_NOINHERITLAYOUT		, L"NOINHERITLAYOUT" },
		//{ WS_EX_NOREDIRECTIONBITMAP , L"NOREDIRECTIONBITMAP" },
		{ WS_EX_LAYOUTRTL			, L"LAYOUTRTL" },
		{ WS_EX_COMPOSITED			, L"COMPOSITED" },
		{ WS_EX_NOACTIVATE			, L"NOACTIVATE" },
	};

	for (auto& item : styles) {
		if (style & item.style)
			text += CString(item.text) += L", ";
	}

	if (text.IsEmpty())
		text = L"LEFT, RIGHISCROLLBAR";

	return L"(WS_EX_) " + text.Left(text.GetLength() - 2);
}

CString WindowHelper::WindowRectToString(HWND hWnd) {
	CWindow win(hWnd);
	CRect rc;
	CString text;

	win.GetWindowRect(&rc);
	text.Format(L"(%d,%d)-(%d,%d) [%d x %d]", rc.left, rc.top, rc.right, rc.bottom, rc.Width(), rc.Height());
	return text;
}

CString WindowHelper::GetWindowClassName(HWND hWnd) {
	WCHAR name[128];
	::GetClassName(hWnd, name, _countof(name));
	return name;
}

HICON WindowHelper::GetWindowOrProcessIcon(HWND hWnd) {
	auto hIcon = GetWindowIcon(hWnd);
	if (!hIcon) {
		DWORD pid = 0;
		::GetWindowThreadProcessId(hWnd, &pid);
		if (pid) {
			::ExtractIconEx(ProcessHelper::GetFullProcessName(pid), 0, nullptr, &hIcon, 1);
		}
	}

	return hIcon;
}

HICON WindowHelper::GetWindowIcon(HWND hWnd) {
	HICON hIcon{ nullptr };
	::SendMessageTimeout(hWnd, WM_GETICON, ICON_SMALL2, 0, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 100, (DWORD_PTR*)&hIcon);
	if (!hIcon) {
		hIcon = (HICON)::GetClassLongPtr(hWnd, GCLP_HICONSM);
	}
	return hIcon;
}