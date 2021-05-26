#include "stdafx.h"
#include "resource.h"
#include "BinaryValueDlg.h"


#ifdef _DEBUG
#pragma comment(lib, "..\\HexControl\\HexControld.lib")
#else
#pragma comment(lib, "..\\HexControl\\HexControl.lib")
#endif

void CBinaryValueDlg::SetName(const CString& name, bool readonly) {
	m_Name = name;
	m_ReadOnlyName = readonly;
}

void CBinaryValueDlg::SetBuffer(IBufferManager* buffer) {
	m_HexEdit.SetBufferManager(buffer);
}

LRESULT CBinaryValueDlg::OnTextChanged(WORD, WORD, HWND, BOOL&) {
	return LRESULT();
}

LRESULT CBinaryValueDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
	//DialogHelper::AdjustOKCancelButtons(this);

	DoDataExchange(FALSE);
	RECT rc;
	GetDlgItem(IDC_BUTTON1).GetWindowRect(&rc);
	ScreenToClient(&rc);
	m_HexEdit.SetBytesPerLine(8);
	m_HexEdit.Create(m_hWnd, &rc, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP, WS_EX_CLIENTEDGE, IDC_HEX);
	m_HexEdit.SetFocus();

	if (m_ReadOnlyName) {
		GetDlgItem(IDC_NAME).SendMessage(EM_SETREADONLY, TRUE);
	}

	m_HexEdit.SetReadOnly(!m_CanModify);

	CenterWindow(GetParent());
	return TRUE;
}

LRESULT CBinaryValueDlg::OnCloseCmd(WORD, WORD wID, HWND, BOOL&) {
	if (wID == IDCANCEL) {
		EndDialog(IDCANCEL);
		return 0;
	}
	if (DoDataExchange(TRUE)) {
		EndDialog(IDOK);
	}
	return 0;
}