#pragma once

struct ThemeColor {
	ThemeColor(PCWSTR name, COLORREF defaultColor,bool changed = false);
	ThemeColor(){}

	CString Name;
	COLORREF DefaultColor;
	COLORREF Color;
	bool Changed{ false };
};