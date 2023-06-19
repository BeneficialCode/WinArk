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
	WCHAR proc[25];
	_itow_s(pid, proc, 10);
	std::wstring title = L"Dump Process: pid = ";
	title += proc;
	title += bitness;
	title += m_px.GetExecutablePath();
	SetWindowText(title.c_str());

	// Register ourselves to receive PreTranslateMessage
	/*CMessageLoop* pLoop = _Module.GetMessageLoop();
	pLoop->AddMessageFilter(this);*/

	SetupStatusBar();
	
	// attach controls
	// DoDataExchange();
	

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
