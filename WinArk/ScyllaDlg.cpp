#include "stdafx.h"
#include "ScyllaDlg.h"
#include "ProcessAccessHelper.h"
#include "Architecture.h"
#include "IATSearcher.h"
#include <PEParser.h>
#include "ImportRebuilder.h"


CScyllaDlg::CScyllaDlg(const WinSys::ProcessManager& pm, ProcessInfoEx& px)
	: m_pm(pm), m_px(px), _importsHandling(_treeImports) {
	_hMenuImports.LoadMenu(IDR_IMPORTS);
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
		case IDC_TREE_IMPORTS:
			DisplayContextMenuImports(wnd, point);
			return;
		default:
			break;
	}
	SetMsgHandled(false);
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

void CScyllaDlg::OnGetImports(UINT uNotifyCode, int nID, CWindow wndCtl) {
	GetImportsHandler();
}

void CScyllaDlg::GetImportsHandler() {
	DWORD_PTR addressIAT = _iatAddress.GetValue();
	DWORD sizeIAT = _iatSize.GetValue();

	if (addressIAT && sizeIAT) {
		_ApiReader.ReadAndParseIAT(addressIAT, sizeIAT, _importsHandling.m_ModuleMap);
		_importsHandling.ScanAndFixModuleList();
		_importsHandling.DisplayAllImports();

		UpdateStatusBar();

		if (IsIATOutsidePEImage(addressIAT)) {
			MessageBox(L"WARNING! IAT is not inside the PE image, requires rebasing");
		}
	}
}

bool CScyllaDlg::IsIATOutsidePEImage(DWORD_PTR addressIAT) {
	DWORD_PTR minAddr = 0, maxAddr = 0;

	if (ProcessAccessHelper::_pSelectedModule) {
		minAddr = ProcessAccessHelper::_pSelectedModule->_modBaseAddr;
		maxAddr = minAddr + ProcessAccessHelper::_pSelectedModule->_modBaseSize;
	}
	else {
		minAddr = ProcessAccessHelper::_targetImageBase;
		maxAddr = ProcessAccessHelper::_targetSizeOfImage + minAddr;
	}

	if (addressIAT > minAddr && addressIAT < maxAddr) {
		return false;
	}
	else {
		return true;
	}
}

void CScyllaDlg::OnDump(UINT uNotifyCode, int nID, CWindow wndCtl) {
	DumpHandler();
}

void CScyllaDlg::DumpHandler() {
	WCHAR filePath[MAX_PATH] = { 0 };
	WCHAR defaultFileName[MAX_PATH] = { 0 };
	const WCHAR* pFileFilter;
	const WCHAR* pDefExtension;
	DWORD_PTR modBase = 0;
	SIZE_T modSize = 0;
	DWORD_PTR entryPoint = 0;
	WCHAR fileName[MAX_PATH] = { 0 };

	if (ProcessAccessHelper::_pSelectedModule) {
		pFileFilter = s_FilterDll;
		pDefExtension = L"dll";
	}
	else {
		pFileFilter = s_FilterExe;
		pDefExtension = L"exe";
	}

	GetCurrentModulePath(_text, _countof(_text));
	GetCurrentDefaultDumpFilename(defaultFileName, _countof(defaultFileName));
	if (ShowFileDialog(filePath, true, defaultFileName, pFileFilter, pDefExtension, _text)) {
		entryPoint = _oepAddress.GetValue();

		if (ProcessAccessHelper::_pSelectedModule) {
			modBase = ProcessAccessHelper::_pSelectedModule->_modBaseAddr;
			wcscpy_s(fileName, ProcessAccessHelper::_pSelectedModule->_fullPath);
			modSize = ProcessAccessHelper::_pSelectedModule->_modBaseSize;
		}
		else {
			modBase = ProcessAccessHelper::_targetImageBase;
			wcscpy_s(fileName, m_px.GetExecutablePath().c_str());
			modSize = ProcessAccessHelper::_targetSizeOfImage;
		}

		void* pImageBase = new BYTE[modSize];

		ProcessAccessHelper::ReadMemoryFromProcess(modBase, modSize, pImageBase);

		PEParser parser(pImageBase);
		if (parser.IsValid()) {
			bool success = parser.DumpProcess(modBase, entryPoint, filePath);
			if (!success) {
				MessageBox(L"Cannot dump image.", L"Failure", MB_ICONERROR);
			}
		}

		delete[] pImageBase;
	}
}

bool CScyllaDlg::GetCurrentModulePath(WCHAR* pBuffer, size_t bufSize) {
	if (ProcessAccessHelper::_pSelectedModule) {
		wcscpy_s(pBuffer, bufSize, ProcessAccessHelper::_pSelectedModule->_fullPath);
	}
	else {
		wcscpy_s(pBuffer, bufSize, m_px.GetExecutablePath().c_str());
	}

	WCHAR* pSlash = wcsrchr(pBuffer, L'\\');
	if (pSlash) {
		*(pSlash + 1) = L'\0';
	}

	return true;
}

bool CScyllaDlg::GetCurrentDefaultDumpFilename(WCHAR* pBuffer, size_t bufSize) {
	WCHAR fullPath[MAX_PATH] = { 0 };

	if (ProcessAccessHelper::_pSelectedModule) {
		wcscpy_s(fullPath, ProcessAccessHelper::_pSelectedModule->_fullPath);
	}
	else {
		wcscpy_s(fullPath, m_px.GetExecutablePath().c_str());
	}

	WCHAR* pTemp = wcsrchr(fullPath, L'\\');
	if (pTemp) {
		pTemp++;

		wcscpy_s(pBuffer, bufSize, pTemp);

		pTemp = wcsrchr(pBuffer, L'.');
		if (pTemp) {
			*pTemp = 0;
			if (ProcessAccessHelper::_pSelectedModule) {
				wcscat_s(pBuffer, bufSize, L"_dump.dll");
			}
			else {
				wcscat_s(pBuffer, bufSize, L"_dump.exe");
			}
		}

		return true;
	}
	return false;
}

bool CScyllaDlg::ShowFileDialog(WCHAR* pSelectedFile, bool save, const WCHAR* pDefFileName, const WCHAR* pFilter,
	const WCHAR* pDefExtension, const WCHAR* pDir)
{
	OPENFILENAME ofn = { 0 };
	if (pDefFileName) {
		wcscpy_s(pSelectedFile, MAX_PATH, pDefFileName);
	}
	else {
		pSelectedFile[0] = L'\0';
	}

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrFilter = pFilter;
	ofn.lpstrDefExt = pDefExtension; // only first 3 chars are used, no dots!
	ofn.lpstrFile = pSelectedFile;
	ofn.lpstrInitialDir = pDir;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

	if (save)
		ofn.Flags |= OFN_OVERWRITEPROMPT;
	else
		ofn.Flags |= OFN_FILEMUSTEXIST;

	if(save)
		return 0 != GetSaveFileName(&ofn);
	else
		return 0 != GetOpenFileName(&ofn);
}

void CScyllaDlg::OnFixDump(UINT uNotifyCode, int nID, CWindow wndCtl) {
	FixDumpHandler();
}

void CScyllaDlg::FixDumpHandler() {
	if (_treeImports.GetCount() < 2) {
		return;
	}

	WCHAR newFilePath[MAX_PATH] = { 0 };
	WCHAR selectedFilePath[MAX_PATH] = { 0 };
	const WCHAR* pFileFilter = nullptr;
	DWORD_PTR modBase = 0;
	DWORD_PTR entryPoint = _oepAddress.GetValue();

	if (ProcessAccessHelper::_pSelectedModule) {
		modBase = ProcessAccessHelper::_pSelectedModule->_modBaseAddr;
		pFileFilter = s_FilterDll;
	}
	else {
		modBase = ProcessAccessHelper::_targetImageBase;
		pFileFilter = s_FilterExe;
	}

	GetCurrentModulePath(_text, _countof(_text));
	if (ShowFileDialog(selectedFilePath, false, nullptr, pFileFilter, nullptr, _text)) {
		wcscpy_s(newFilePath, selectedFilePath);

		const WCHAR* pExtension = nullptr;
		WCHAR* pDot = wcsrchr(newFilePath, L'.');
		if (pDot) {
			*pDot = L'\0';
			pExtension = selectedFilePath + (pDot - newFilePath);
		}

		wcscat_s(newFilePath, L"_SCY");

		if (pExtension) {
			wcscat_s(newFilePath, pExtension);
		}

		ImportRebuilder rebuilder(selectedFilePath);
		rebuilder.EnableOFTSupport();

		rebuilder.RebuildImportTable(newFilePath, _importsHandling.m_ModuleMap);
	}
}

void CScyllaDlg::OnPERebuild(UINT uNotifyCode, int nID, CWindow wndCtl) {
	PERebuildHandler();
}

void CScyllaDlg::PERebuildHandler() {
	DWORD newSize = 0;
	WCHAR selectedFilePath[MAX_PATH] = { 0 };

	GetCurrentModulePath(_text, _countof(_text));
	if (ShowFileDialog(selectedFilePath, false, nullptr, s_FilterExeDll, nullptr, _text)) {
		DWORD fileSize = ProcessAccessHelper::GetFileSize(selectedFilePath);
		
		PEParser parser(selectedFilePath, true);

		if (!parser.IsValid()) {
			return;
		}

		parser.GetPESections();
		parser.SetDefaultFileAligment();
		parser.AlignAllSectionHeaders();
		parser.FixPEHeader();
		parser.SavePEFileToDisk(selectedFilePath);
	}
}

CTreeItem CScyllaDlg::FindTreeItem(CPoint pt, bool screenCoordinates) {
	if (screenCoordinates) {
		_treeImports.ScreenToClient(&pt);
	}
	UINT flags;
	CTreeItem over = _treeImports.HitTest(pt, &flags);
	if (over) {
		if (!(flags & TVHT_ONITEM))
		{
			over.m_hTreeItem = NULL;
		}
	}
	return over;
}

LRESULT CScyllaDlg::OnTreeImportsDoubleClick(const NMHDR* pnmh) {
	if (_treeImports.GetCount() < 1)
		return 0;

	CTreeItem over = FindTreeItem(CPoint(GetMessagePos()), true);
	if (over && _importsHandling.IsImport(over)) {
		// todo: Pick API Handler

	}

	return 0;
}

LRESULT CScyllaDlg::OnTreeImportsKeyDown(const NMHDR* pnmh) {
	const NMTVKEYDOWN* tkd = (NMTVKEYDOWN*)pnmh;
	switch (tkd->wVKey)
	{
		case VK_RETURN:
		{
			CTreeItem selected = _treeImports.GetFocusItem();
			if (!selected.IsNull() && _importsHandling.IsImport(selected))
			{
				// to do: Pick API handler
			}
		}
		return 1;
		case VK_DELETE:
			DeleteSelectedImportsHandler();
			return 1;
	}

	SetMsgHandled(FALSE);
	return 0;
}

void CScyllaDlg::DeleteSelectedImportsHandler() {
	CTreeItem selected = _treeImports.GetFirstSelectedItem();
	while (!selected.IsNull())
	{
		if (_importsHandling.IsModule(selected))
		{
			_importsHandling.CutModule(selected);
		}
		else
		{
			_importsHandling.CutImport(selected);
		}
		selected = _treeImports.GetNextSelectedItem(selected);
	}
	UpdateStatusBar();
}

void CScyllaDlg::OnInvalidImports(UINT uNotifyCode, int nID, CWindow wndCtl) {
	ShowInvalidImportsHandler();
}

void CScyllaDlg::OnSuspectImports(UINT uNotifyCode, int nID, CWindow wndCtl) {
	ShowSuspectImportsHandler();
}

void CScyllaDlg::OnClearImports(UINT uNotifyCode, int nID, CWindow wndCtl) {
	ClearImportsHandler();
}

void CScyllaDlg::ShowInvalidImportsHandler() {
	_importsHandling.SelectImports(true, false);
	GotoDlgCtrl(_treeImports);
}

void CScyllaDlg::ShowSuspectImportsHandler() {
	_importsHandling.SelectImports(false, true);
	GotoDlgCtrl(_treeImports);
}

void CScyllaDlg::ClearImportsHandler() {
	_importsHandling.ClearAllImports();
	UpdateStatusBar();
}

void CScyllaDlg::DisplayContextMenuImports(CWindow hwnd, CPoint pt) {
	if (_treeImports.GetCount() < 1)
		return;

	CTreeItem over, parent;

	if (pt.x == -1 && pt.y == -1) {
		CRect pos;
		over = _treeImports.GetFocusItem();
		if (over)
		{
			over.EnsureVisible();
			over.GetRect(&pos, TRUE);
			_treeImports.ClientToScreen(&pos);
		}
		else
		{
			_treeImports.GetWindowRect(&pos);
		}
		pt = pos.TopLeft();
	}
	else {
		over = FindTreeItem(pt, true);
	}

	SetupImportsMenuItems(over);

	CMenuHandle hSub = _hMenuImports.GetSubMenu(0);
	BOOL menuItem = hSub.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, hwnd);
	if (menuItem) {
		switch (menuItem)
		{
			case ID_INVALIDATE:
				if (_importsHandling.IsModule(over))
					_importsHandling.InvalidateModule(over);
				else
					_importsHandling.InvalidateImport(over);
				break;
			case ID_EXPAND_ALL_NODES:
				_importsHandling.ExpandAllTreeNodes();
				break;
			case ID_CUT_THUNK:
				_importsHandling.CutImport(over);
				break;
			case ID_DELETE_TREE_NODE:
				_importsHandling.CutModule(_importsHandling.IsImport(over) ? over.GetParent() : over);
				break;
			default:
				break;
		}
	}

	UpdateStatusBar();
}

void CScyllaDlg::SetupImportsMenuItems(CTreeItem item) {
	bool isItem, isImport = false;
	isItem = !item.IsNull();
	if (isItem) {
		isImport = _importsHandling.IsImport(item);
	}

	CMenuHandle hSub = _hMenuImports.GetSubMenu(0);

	UINT itemOnly = isItem ? MF_ENABLED : MF_GRAYED;
	UINT importOnly = isImport ? MF_ENABLED : MF_GRAYED;

	hSub.EnableMenuItem(ID_INVALIDATE, itemOnly);
	hSub.EnableMenuItem(ID_CUT_THUNK, importOnly);
	hSub.EnableMenuItem(ID_DELETE_TREE_NODE, itemOnly);
}