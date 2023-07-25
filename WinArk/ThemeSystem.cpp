#include "stdafx.h"
#include "ThemeSystem.h"
#include "AppSettings.h"
#include "IniFile.h"

int g_AvHighFont = 0x10;
int g_AvWidthFont = 0x8;
HFONT g_hAppFont;

HPEN g_hBlackPen, g_hWhitePen, g_hDarkGrayPen;
HPEN g_myPen[20];

COLORREF g_crLightGray, g_crDarkGray, g_crBlack;
COLORREF g_myColor[20];

HBRUSH g_hLightGrayBrush;
HBRUSH g_myBrush[20];

t_scheme g_myScheme[8];

HWND g_hWndMainFrame;
HWND g_hWndMDIClient;
HWND g_hWndTop = 0;

void InitColorSys() {
	HDC hdc = GetDC(g_hWndMainFrame);
	g_crLightGray = GetNearestColor(hdc, RGB(0xc0, 0xc0, 0xc0));
	g_crDarkGray = GetNearestColor(hdc, RGB(0x80, 0x80, 0x80));
	g_crBlack = GetNearestColor(hdc, RGB(0, 0, 0));

	g_myColor[0] = GetNearestColor(hdc, RGB(0, 0, 0));
	g_myColor[1] = GetNearestColor(hdc, RGB(0x80, 0, 0));
	g_myColor[2] = GetNearestColor(hdc, RGB(0, 0x80, 0));
	g_myColor[3] = GetNearestColor(hdc, RGB(0x80, 0x80, 0));
	g_myColor[4] = GetNearestColor(hdc, RGB(0, 0, 0x80));
	g_myColor[5] = GetNearestColor(hdc, RGB(0x80, 0, 0x80));
	g_myColor[6] = GetNearestColor(hdc, RGB(0, 0x80, 0x80));
	g_myColor[7] = GetNearestColor(hdc, RGB(0xc0, 0xc0, 0xc0));
	g_myColor[8] = GetNearestColor(hdc, RGB(0x80, 0x80, 0x80));
	g_myColor[9] = GetNearestColor(hdc, RGB(0xff, 0, 0));
	g_myColor[10] = GetNearestColor(hdc, RGB(0, 0xff, 0));
	g_myColor[11] = GetNearestColor(hdc, RGB(0xff, 0xff, 0));
	g_myColor[12] = GetNearestColor(hdc, RGB(0, 0, 0xff));
	g_myColor[13] = GetNearestColor(hdc, RGB(0xff, 0, 0xff));
	g_myColor[14] = GetNearestColor(hdc, RGB(0, 0xff, 0xff));
	g_myColor[15] = GetNearestColor(hdc, RGB(0xff, 0xff, 0xff));
	g_myColor[16] = GetNearestColor(hdc, RGB(0xc0, 0xdc, 0xc0));
	g_myColor[17] = GetNearestColor(hdc, RGB(0x16, 0xc6, 0x12));
	g_myColor[18] = GetNearestColor(hdc, RGB(0x53, 0x53, 0x53));
	g_myColor[19] = GetNearestColor(hdc, RGB(0xa4, 0xa0, 0xa0));
}

void InitFontSys() {
	LOGFONT lf;
	lf.lfHeight = 0;
	if (AppSettings::Get().Load(L"Software\\YuanOS\\WinArk")) {
		lf = AppSettings::Get().Font();
	}
	if (!lf.lfHeight) {
		lf.lfHeight = -14;
		lf.lfWidth = 0;
		lf.lfEscapement = 0;
		lf.lfOrientation = 0;
		lf.lfWeight = FW_NORMAL;
		lf.lfItalic = 0;
		lf.lfUnderline = 0;
		lf.lfStrikeOut = 0;
		lf.lfCharSet = GB2312_CHARSET;
		lf.lfOutPrecision = OUT_STROKE_PRECIS;
		lf.lfClipPrecision = CLIP_STROKE_PRECIS;
		lf.lfQuality = DRAFT_QUALITY;
		lf.lfPitchAndFamily = FF_SWISS | VARIABLE_PITCH;
		wcscpy_s(lf.lfFaceName, L"Î¢ÈíÑÅºÚ");
	}
	g_hAppFont = CreateFontIndirect(&lf);
}

void InitPenSys() {
	g_hBlackPen = static_cast<HPEN>(GetStockObject(BLACK_PEN));
	g_hWhitePen = static_cast<HPEN>(GetStockObject(WHITE_PEN));
	g_hDarkGrayPen = CreatePen(PS_SOLID, 0, g_crDarkGray);

	g_myPen[0] = (HPEN)GetStockObject(BLACK_PEN);
	g_myPen[1] = CreatePen(PS_SOLID, 0, g_myColor[1]);
	g_myPen[2] = CreatePen(PS_SOLID, 0, g_myColor[2]);
	g_myPen[3] = CreatePen(PS_SOLID, 0, g_myColor[3]);
	g_myPen[4] = CreatePen(PS_SOLID, 0, g_myColor[4]);
	g_myPen[5] = CreatePen(PS_SOLID, 0, g_myColor[5]);
	g_myPen[6] = CreatePen(PS_SOLID, 0, g_myColor[6]);
	g_myPen[7] = CreatePen(PS_SOLID, 0, g_myColor[7]);
	g_myPen[8] = CreatePen(PS_SOLID, 0, g_myColor[8]);
	g_myPen[9] = CreatePen(PS_SOLID, 0, g_myColor[9]);
	g_myPen[10] = CreatePen(PS_SOLID, 0, g_myColor[10]);
	g_myPen[11] = CreatePen(PS_SOLID, 0, g_myColor[11]);
	g_myPen[12] = CreatePen(PS_SOLID, 0, g_myColor[12]);
	g_myPen[13] = CreatePen(PS_SOLID, 0, g_myColor[13]);
	g_myPen[14] = CreatePen(PS_SOLID, 0, g_myColor[14]);
	g_myPen[15] = CreatePen(PS_SOLID, 0, g_myColor[15]);
	g_myPen[16] = CreatePen(PS_SOLID, 0, g_myColor[16]);
	g_myPen[17] = CreatePen(PS_SOLID, 0, g_myColor[17]);
	g_myPen[18] = CreatePen(PS_SOLID, 0, g_myColor[18]);
	g_myPen[19] = CreatePen(PS_SOLID, 0, g_myColor[19]);
}

void InitBrushSys() {
	g_hLightGrayBrush = CreateSolidBrush(g_crLightGray);

	g_myBrush[0] = (HBRUSH)GetStockObject(BLACK_BRUSH);
	g_myBrush[1] = CreateSolidBrush(g_myColor[1]);
	g_myBrush[2] = CreateSolidBrush(g_myColor[2]);
	g_myBrush[3] = CreateSolidBrush(g_myColor[3]);
	g_myBrush[4] = CreateSolidBrush(g_myColor[4]);
	g_myBrush[5] = CreateSolidBrush(g_myColor[5]);
	g_myBrush[6] = CreateSolidBrush(g_myColor[6]);
	g_myBrush[7] = CreateSolidBrush(g_myColor[7]);
	g_myBrush[8] = CreateSolidBrush(g_myColor[8]);
	g_myBrush[9] = CreateSolidBrush(g_myColor[9]);
	g_myBrush[10] = CreateSolidBrush(g_myColor[10]);
	g_myBrush[11] = CreateSolidBrush(g_myColor[11]);
	g_myBrush[12] = CreateSolidBrush(g_myColor[12]);
	g_myBrush[13] = CreateSolidBrush(g_myColor[13]);
	g_myBrush[14] = CreateSolidBrush(g_myColor[14]);
	g_myBrush[15] = CreateSolidBrush(g_myColor[15]);
	g_myBrush[16] = CreateSolidBrush(g_myColor[16]);
	g_myBrush[17] = CreateSolidBrush(g_myColor[17]);
	g_myBrush[18] = CreateSolidBrush(g_myColor[18]);
	g_myBrush[19] = CreateSolidBrush(g_myColor[19]);
}

void InitSchemSys() {
	g_myScheme[0].name = "Marine";
	g_myScheme[0].textcolor = Black;
	g_myScheme[0].hitextcolor = Red;
	g_myScheme[0].lowcolor = DarkGray;
	g_myScheme[0].bkcolor = White;
	g_myScheme[0].selbkcolor = Yellow;
	g_myScheme[0].linecolor = DarkBlue;
	g_myScheme[0].auxcolor = Blue;
	g_myScheme[0].condbkcolor = Magenta;
}

bool SaveColors(PCWSTR path, PCWSTR prefix, const ThemeColor* colors, int count) {
	IniFile file(path);
	CString text;

	for (int i = 0; i < count; i++) {
		text.Format(L"%s%d", prefix, i);
		auto& info = colors[i];
		if (!file.WriteColor(text, L"Color", info.Color))
			return false;
		file.WriteString(text, L"Name", info.Name);
	}
	return true;
}

bool LoadColors(PCWSTR path, PCWSTR prefix, ThemeColor* colors, int count) {
	IniFile file(path);
	if (!file.IsValid())
		return false;

	CString text;
	for (int i = 0; i < count; i++) {
		text.Format(L"%s%d", prefix, i);
		auto& info = colors[i];
		info.Color = file.ReadColor(text, L"Color", info.Color);
		info.Name = file.ReadString(text, L"Name", info.Name);
	}
	return true;
}