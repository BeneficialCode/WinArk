#include "stdafx.h"
#include "ScyllaDlg.h"
#include "ProcessAccessHelper.h"
#include "Architecture.h"
#include "IATSearcher.h"

CScyllaDlg::CScyllaDlg(const WinSys::ProcessManager& pm, ProcessInfoEx& px)
	: m_pm(pm), m_px(px), _importsHandling(_treeImports) {
	_hIconCheck.LoadIcon(IDI_CHECK);
	_hIconWarning.LoadIcon(IDI_WARNING);
	_hIconError.LoadIcon(IDI_ERROR);
}

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
	_pid = pid;
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

	DoDataExchange(); // attach controls
	
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

	const int parts = 4;
	int widths[parts];
	widths[StatusParts::Count] = rcMain.Width() / 5;
	widths[StatusParts::Invalid] = widths[StatusParts::Count] + rcMain.Width() / 5;
	widths[StatusParts::ImageBase] = widths[StatusParts::Invalid] + rcMain.Width() / 3;
	widths[StatusParts::Module] = -1;

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
	if (ProcessAccessHelper::_hProcess != 0) {
		ProcessAccessHelper::CloseProcessHandle();
		_ApiReader.ClearAll();
	}
	ProcessAccessHelper::_pid = _pid;
	if (!ProcessAccessHelper::OpenProcessHandle(_pid)) {
		EnableDialogControls(FALSE);
		UpdateStatusBar();
		return;
	}

	ProcessAccessHelper::GetProcessModules(ProcessAccessHelper::_hProcess, ProcessAccessHelper::_moduleList);

	_ApiReader.ReadApisFromModuleList();

	ProcessAccessHelper::_pSelectedModule = nullptr;

	ProcessAccessHelper::_targetImageBase = m_px.GetProcessInfo()->ImageBase;
	ProcessAccessHelper::_targetSizeOfImage = m_px.GetProcessInfo()->ImageSize;

	DWORD entryPoint = ProcessAccessHelper::GetEntryPointFromFile(m_px.GetExecutablePath().c_str());

	_oepAddress.SetValue(entryPoint + ProcessAccessHelper::_targetImageBase);

	EnableDialogControls(TRUE);

	UpdateStatusBar();
}

void CScyllaDlg::UpdateStatusBar() {
	unsigned int totalImports = _importsHandling.ThunkCount();
	unsigned int invalidImports = _importsHandling.InvalidThunkCount();

	swprintf_s(_text, L"\tImports: %u", totalImports);
	m_StatusBar.SetText(StatusParts::Count, _text);
	if (invalidImports > 0) {
		m_StatusBar.SetIcon(StatusParts::Invalid, _hIconError);
	}
	else {
		m_StatusBar.SetIcon(StatusParts::Invalid, _hIconCheck);
	}

	swprintf_s(_text, L"\tInvalid: %d", invalidImports);
	m_StatusBar.SetText(StatusParts::Invalid, _text);

	DWORD_PTR imageBase = 0;
	std::wstring fileName;
	if (ProcessAccessHelper::_pSelectedModule) {
		imageBase = ProcessAccessHelper::_pSelectedModule->_modBaseAddr;
		fileName = ProcessAccessHelper::_pSelectedModule->GetFileName();
	}
	else {
		imageBase = m_px.GetProcessInfo()->ImageBase;
		fileName = m_px.GetProcessInfo()->GetImageName();
	}

	swprintf_s(_text, L"\tImagebase: " PRINTF_DWORD_PTR_FULL, imageBase);
	m_StatusBar.SetText(StatusParts::ImageBase, _text);
	m_StatusBar.SetText(StatusParts::Module, fileName.c_str());
	m_StatusBar.SetTipText(StatusParts::Module, fileName.c_str());
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

void CScyllaDlg::OnAutoSearch(UINT uNotifyCode, int nID, CWindow wndCtl) {
	IATAutoSearchActionHandler();
}

void CScyllaDlg::IATAutoSearchActionHandler() {
	DWORD_PTR searchAddress = 0;
	DWORD_PTR addressIAT = 0, addressIATAdv = 0;
	DWORD sizeIAT = 0, sizeIATAdv = 0;
	IATSearcher iatSearch;

	if (_oepAddress.GetWindowTextLength() > 0) {
		searchAddress = _oepAddress.GetValue();
		if (searchAddress) {
			iatSearch.SearchImportAddressTableInProcess(searchAddress, &addressIATAdv, &sizeIATAdv, true);

			iatSearch.SearchImportAddressTableInProcess(searchAddress, &addressIAT, &sizeIAT, false);

			if (addressIAT != 0 && addressIATAdv == 0) {
				SetDialogIATAddressAndSize(addressIAT, sizeIAT);
			}
			else if (addressIAT == 0 && addressIATAdv != 0) {
				SetDialogIATAddressAndSize(addressIATAdv, sizeIAT);
			}
			else if (addressIAT != 0 && addressIATAdv != 0) {
				if (addressIAT != addressIATAdv || sizeIAT != sizeIATAdv) {
					int msgboxID = MessageBox(L"Result of advanced and normal search is different. Do you want to use the IAT Search Advanced result?",
						L"Information", MB_YESNO | MB_ICONINFORMATION);
					if (msgboxID == IDYES) {
						SetDialogIATAddressAndSize(addressIATAdv, sizeIATAdv);
					}
					else {
						SetDialogIATAddressAndSize(addressIAT, sizeIAT);
					}
				}
				else {
					SetDialogIATAddressAndSize(addressIAT, sizeIAT);
				}
			}
		}
	}
}

void CScyllaDlg::SetDialogIATAddressAndSize(DWORD_PTR addressIAT, DWORD sizeIAT) {
	_iatAddress.SetValue(addressIAT);
	_iatSize.SetValue(sizeIAT);

	swprintf_s(_text, L"IAT found:\r\n\r\nStart: " PRINTF_DWORD_PTR_FULL L"\r\nSize: 0x%04X (%d) ", addressIAT, sizeIAT, sizeIAT);
	MessageBox(_text, L"IAT found", MB_ICONINFORMATION);
}