#pragma once

struct ThemeColor {
	ThemeColor(PCWSTR name, COLORREF defaultColor, bool changed = false);
	ThemeColor() {}

	CString Name;
	COLORREF DefaultColor;
	COLORREF Color;
	bool Changed{ false };
};

struct CellColorKey {
	CellColorKey(ULONG tag, int col = 0) :Tag(tag), Column(col) {}

	int Tag, Column;

	bool operator == (const CellColorKey& other) const {
		return Tag == other.Tag && Column == other.Column;
	}
};

template<>
struct std::hash<CellColorKey> {
	const size_t operator()(const CellColorKey& key) const {
		return (key.Column << 16) ^ key.Tag;
	}
};

struct CellColor :CellColorKey {
	using CellColorKey::CellColorKey;

	COLORREF TextColor, BackColor;
	DWORD64 TargetTime;
	LOGFONT Font;
};