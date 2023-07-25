#pragma once
#include "stdafx.h"

class CCustomButtonParent :
	public CWindowImpl<CCustomButtonParent>,
	public CCustomDraw<CCustomButtonParent> {
public:
	void OnFinalMessage(HWND) override {
		delete this;
	}

	BEGIN_MSG_MAP(CCustomButtonParent)
		CHAIN_MSG_MAP(CCustomDraw<CCustomButtonParent>)
	END_MSG_MAP()

	DWORD OnPreErase(int, LPNMCUSTOMDRAW cd) {
		if (cd->hdr.hwndFrom != m_hWnd) {
			SetMsgHandled(FALSE);
			return CDRF_DODEFAULT;
		}
		return CDRF_DODEFAULT;
	}

	DWORD OnPostErase(int, LPNMCUSTOMDRAW cd) {
		if (cd->hdr.hwndFrom != m_hWnd) {
			SetMsgHandled(FALSE);
			return CDRF_DODEFAULT;
		}
		CDCHandle dc(cd->hdc);


		return CDRF_DODEFAULT;
	}

	void Init(HWND hWnd) {
		m_hWnd = hWnd;
		ATLVERIFY(SubclassWindow(::GetParent(hWnd)));
	}

	HWND m_hWnd;
};
