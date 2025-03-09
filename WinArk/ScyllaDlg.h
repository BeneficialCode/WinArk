#pragma once
#include "resource.h"
#include "HexEdit.h"
#include "ImportsHandling.h"
#include "ApiReader.h"


class CScyllaDlg 
	:public CDialogImpl<CScyllaDlg>,
	public CWinDataExchange<CScyllaDlg>,
	public CMessageFilter{
public:
	enum {IDD = IDD_SCYLLA};

	// Dialog Data eXchange, attaches/subclasses child controls to wrappers
	// DDX_CONTROL : subclass
	// DDX_CONTROL_HANDLE : attach
	BEGIN_DDX_MAP(CScyllaDlg)
		DDX_CONTROL_HANDLE(IDC_LIST_LOG,m_ListLog)
		DDX_CONTROL(IDC_OEP_ADDRESS,_oepAddress)
		DDX_CONTROL(IDC_IAT_ADDRESS,_iatAddress)
		DDX_CONTROL(IDC_IAT_SIZE,_iatSize)
		DDX_CONTROL(IDC_TREE_IMPORTS,_treeImports)
	END_DDX_MAP()

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	CScyllaDlg(const WinSys::ProcessManager& pm, ProcessInfoEx& px);

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	void OnSize(UINT nType, CSize size);
	void OnDestroy();
	void OnContextMenu(CWindow wnd, CPoint point);

	void DisplayContextMenuLog(CWindow, CPoint);

	void EnableDialogControls(BOOL value);

	void ProcessHandler();
	void GetImportsHandler();
	void DumpHandler();
	void FixDumpHandler();
	void PERebuildHandler();
	void DeleteSelectedImportsHandler();

	void SaveTreeHandler();
	void LoadTreeHandler();

	CTreeItem FindTreeItem(CPoint pt, bool screenCoordinates);

	void ShowInvalidImportsHandler();
	void ShowSuspectImportsHandler();
	void ClearImportsHandler();
	void StartDisassembler(CTreeItem selectedTreeNode);
	void DisassemblerHandler();
	void PickDLLHandler();

	void OnInvalidImports(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnSuspectImports(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnClearImports(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnCutSelected(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnSaveTree(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnLoadTree(UINT uNotifyCode, int nID, CWindow wndCtl);

	void OnAutoSearch(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnGetImports(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnDump(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnFixDump(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnPERebuild(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnDisassembler(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnPickDLL(UINT uNotifyCode, int nID, CWindow wndCtl);

	LRESULT OnTreeImportsDoubleClick(const NMHDR* pnmh);
	LRESULT OnTreeImportsKeyDown(const NMHDR* pnmh);

	bool IsIATOutsidePEImage(DWORD_PTR addressIAT);

	void DisplayContextMenuImports(CWindow, CPoint);
	void SetupImportsMenuItems(CTreeItem item);

protected:
	void SetupStatusBar();
	void UpdateStatusBar();

	enum StatusParts {
		Count = 0,
		Invalid,
		ImageBase,
		Module,
	};

	void IATAutoSearchActionHandler();

	WCHAR _text[512];
	ImportsHandling _importsHandling;
	CMultiSelectTreeViewCtrl _treeImports;
	ApiReader _ApiReader;

	CIcon _hIconError;
	CIcon _hIconWarning;
	CIcon _hIconCheck;

protected:
	void SetDialogIATAddressAndSize(DWORD_PTR addressIAT, DWORD sizeIAT);

	bool ShowFileDialog(WCHAR* pSelectedFile, bool save, const WCHAR* pDefFileName, const WCHAR* pFilter = nullptr,
		const WCHAR* pDefExtension = nullptr, const WCHAR* pDir = nullptr);

	bool GetCurrentModulePath(WCHAR* pBuffer, size_t bufSize);
	bool GetCurrentDefaultDumpFilename(WCHAR* pBuffer, size_t bufSize);

private:
	static inline const WCHAR s_FilterExe[] = L"Executable (*.exe)\0*.exe\0All files\0*.*\0";
	static inline const WCHAR s_FilterDll[] = L"Dynamic Link Library (*.dll)\0*.dll\0All files\0*.*\0";
	static inline const WCHAR s_FilterExeDll[] = L"Executable (*.exe)\0*.exe\0Dynamic Link Library (*.dll)\0*.dll\0All files\0*.*\0";
	static inline const WCHAR s_FilterTxt[] = L"Text file (*.txt)\0*.txt\0All files\0*.*\0";
	static inline const WCHAR s_FilterXml[] = L"XML file (*.xml)\0*.xml\0All files\0*.*\0";
	static inline const WCHAR s_FilterMem[] = L"MEM file (*.mem)\0*.mem\0All files\0*.*\0";

	ProcessInfoEx& m_px;

	const WinSys::ProcessManager& m_pm;
	CStatusBarCtrl m_StatusBar;
	
	CListBox m_ListLog;
	CAccelerator m_Accelerators;
	CHexEdit _oepAddress;
	CHexEdit _iatAddress;
	CHexEdit _iatSize;
	CIcon m_Icon;
	DWORD _pid;
	CMenu _hMenuImports;

public:
	BEGIN_MSG_MAP_EX(CScyllaDlg)
		MSG_WM_SIZE(OnSize)
		MSG_WM_DESTROY(OnDestroy)
		MSG_WM_CONTEXTMENU(OnContextMenu)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)

		NOTIFY_HANDLER_EX(IDC_TREE_IMPORTS,NM_DBLCLK, OnTreeImportsDoubleClick)
		NOTIFY_HANDLER_EX(IDC_TREE_IMPORTS,TVN_KEYDOWN,OnTreeImportsKeyDown)

		COMMAND_ID_HANDLER_EX(IDC_BTN_AUTO_SEARCH, OnAutoSearch)
		COMMAND_ID_HANDLER_EX(IDC_BTN_GET_IMPORTS, OnGetImports)
		COMMAND_ID_HANDLER_EX(IDC_BTN_DUMP, OnDump)
		COMMAND_ID_HANDLER_EX(IDC_BTN_FIX_DUMP, OnFixDump)
		COMMAND_ID_HANDLER_EX(IDC_BTN_PE_REBUILD,OnPERebuild)
		COMMAND_ID_HANDLER_EX(ID_MISC_DISASSEMBLER,OnDisassembler)
		COMMAND_ID_HANDLER_EX(IDC_BTN_PICKDLL,OnPickDLL)

		COMMAND_ID_HANDLER_EX(IDC_BTN_CLEAR,OnClearImports)
		COMMAND_ID_HANDLER_EX(IDC_BTN_SHOW_INVALID,OnInvalidImports)
		COMMAND_ID_HANDLER_EX(IDC_BTN_SHOW_SUSPECT, OnSuspectImports)

		COMMAND_ID_HANDLER_EX(ID_IMPORTS_CLEAR_IMPORTS,OnClearImports)
		COMMAND_ID_HANDLER_EX(ID_IMPORTS_SHOW_INVALID,OnInvalidImports)
		COMMAND_ID_HANDLER_EX(ID_IMPORTS_SHOW_SUSPECT,OnSuspectImports)
		COMMAND_ID_HANDLER_EX(ID_IMPORTS_CUT_SELECTED,OnCutSelected)
		COMMAND_ID_HANDLER_EX(ID_IMPORTS_SAVE_TREE,OnSaveTree)
		COMMAND_ID_HANDLER_EX(ID_IMPORTS_LOAD_TREE,OnLoadTree)
	END_MSG_MAP()
};