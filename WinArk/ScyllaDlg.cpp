#include "stdafx.h"
#include "ScyllaDlg.h"


BOOL CScyllaDlg::PreTranslateMessage(MSG* pMsg) {
	if (m_Accelerators.TranslateAccelerator(m_hWnd, pMsg)) {
		return TRUE;
	}
	else if (IsDialogMessage(pMsg)) {
		return TRUE;
	}
	return FALSE;
}

LRESULT CScyllaDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	DWORD pid = static_cast<DWORD>(lParam);
	std::wstring bitness = m_px.GetBitness() == 64 ? L" (x64) " : L" (x86) ";
	WCHAR proc[25] = { 0 };
	_itow_s(pid, proc, 10);
	std::wstring title = L"Scylla v1.0.0 Dump Process: pid = ";
	title += proc;
	title += bitness;
	title += m_px.GetProcess()->GetName();
	SetWindowText(title.c_str());
	m_Icon.LoadIcon(IDI_SCYLLA);
	SetIcon(m_Icon, TRUE);

	SetupStatusBar();
	
	EnableDialogControls(FALSE);

	ProcessHandler();

	return TRUE;
}

LRESULT CScyllaDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return 0;
}

void CScyllaDlg::SetupStatusBar() {
	m_StatusBar.Create(m_hWnd, nullptr, L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN |
		WS_CLIPSIBLINGS | SBARS_TOOLTIPS);

	CRect rcMain, rcStatus;
	GetClientRect(&rcMain);
	m_StatusBar.GetWindowRect(&rcStatus);

	const int parts = 3;
	int widths[parts];
	widths[StatusParts::Count] = rcMain.Width() / 5;
	widths[StatusParts::Invalid] = widths[StatusParts::Count] + rcMain.Width() / 5;
	widths[StatusParts::ImageBase] = widths[StatusParts::Invalid] + rcMain.Width() / 3;

	m_StatusBar.SetParts(parts, widths);

	ResizeClient(rcMain.Width(), rcMain.Height() + rcStatus.Height(), false);
}

void CScyllaDlg::OnSize(UINT nType, CSize size) {
	m_StatusBar.SendMessage(WM_SIZE);
	SetMsgHandled(FALSE);
}

void CScyllaDlg::OnDestroy() {
	SetMsgHandled(FALSE);
}

void CScyllaDlg::ProcessHandler() {
	UpdateStatusBar();
}

void CScyllaDlg::UpdateStatusBar() {
	
}

void CScyllaDlg::OnContextMenu(CWindow wnd, CPoint point)
{
	switch (wnd.GetDlgCtrlID())
	{
		case IDC_LIST_LOG:
			DisplayContextMenuLog(wnd, point);
			return;
		default:
			break;
	}
}

void CScyllaDlg::DisplayContextMenuLog(CWindow, CPoint) {

}

void CScyllaDlg::EnableDialogControls(BOOL value) {
	GetDlgItem(IDC_BTN_PICKDLL).EnableWindow(value);
	GetDlgItem(IDC_BTN_DUMP).EnableWindow(value);
	GetDlgItem(IDC_BTN_FIX_DUMP).EnableWindow(value);
	GetDlgItem(IDC_BTN_AUTO_SEARCH).EnableWindow(value);
	GetDlgItem(IDC_BTN_GET_IMPORTS).EnableWindow(value);
	GetDlgItem(IDC_BTN_SHOW_SUSPECT).EnableWindow(value);
	GetDlgItem(IDC_BTN_SHOW_INVALID).EnableWindow(value);
	GetDlgItem(IDC_BTN_CLEAR).EnableWindow(value);
}