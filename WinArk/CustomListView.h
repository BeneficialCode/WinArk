#pragma once
#include "stdafx.h"
#include "Theme.h"

class CCustomListView :public CWindowImpl<CCustomListView, CListViewCtrl> {
public:
	BEGIN_MSG_MAP(CCustomListView)
		MESSAGE_HANDLER(::RegisterWindowMessage(L"WTLHelperUpdateTheme"), OnUpdateTheme)
	END_MSG_MAP()

	void OnFinalMessage(HWND) override {
		delete this;
	}

	LRESULT OnUpdateTheme(UINT /*uMsg*/, WPARAM wp, LPARAM lParam, BOOL& /*bHandled*/) {
		auto theme = reinterpret_cast<Theme*>(lParam);
		SetBkColor(theme->BackColor);
		SetTextColor(theme->TextColor);

		return 0;
	}
};