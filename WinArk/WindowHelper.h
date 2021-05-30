#pragma once

struct WindowHelper abstract final {
	static CString WindowStyleToString(HWND hWnd);
	static CString ClassStyleToString(HWND hWnd);
	static CString WindowExtendedStyleToString(HWND hWnd);
	static CString WindowRectToString(HWND hWnd);
	static CString GetWindowClassName(HWND hWnd);
	static HICON GetWindowIcon(HWND hWnd);
	static HICON GetWindowOrProcessIcon(HWND hWnd);
};