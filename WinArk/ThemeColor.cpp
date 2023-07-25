#include "stdafx.h"
#include "ThemeColor.h"

ThemeColor::ThemeColor(PCWSTR name, COLORREF defaultColor, bool changed)
	: Name(name), DefaultColor(defaultColor), Color(defaultColor),
	Changed(changed) {
}
