#pragma once
#include "resource.h"




class CScyllaDlg 
	:public CDialogImpl<CScyllaDlg>,
	public CDialogResize<CScyllaDlg>,
	public CWinDataExchange<CScyllaDlg>,
	public CMessageFilter{
public:
	enum {IDD = IDD_SCYLLA};

	// Dialog Data eXchange, attaches/subclasses child controls to wrappers
	// DDX_CONTROL : subclass
	// DDX_CONTROL_HANDLE : attach
	BEGIN_DDX_MAP(CScyllaDlg)
		DDX_CONTROL(IDC_TREE_IMPORTS, m_ImportsTree)
		DDX_CONTROL_HANDLE(IDC_LIST_LOG,m_ListLog)
	END_DDX_MAP()

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	CScyllaDlg(const WinSys::ProcessManager& pm, ProcessInfoEx& px) :m_pm(pm), m_px(px) {}

	BEGIN_MSG_MAP_EX(CScyllaDlg)
		MSG_WM_SIZE(OnSize)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		CHAIN_MSG_MAP(CDialogResize<CScyllaDlg>)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CScyllaDlg)
		DLGRESIZE_CONTROL(IDC_BTN_PICKDLL, DLSZ_MOVE_X)

		DLGRESIZE_CONTROL(IDC_TREE_IMPORTS,DLSZ_SIZE_X|DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_GROUP_IMPORTS,DLSZ_SIZE_X|DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_BTN_SHOW_INVALID, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_BTN_SHOW_SUSPECT, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_BTN_CLEAR, DLSZ_MOVE_X | DLSZ_MOVE_Y)

		DLGRESIZE_CONTROL(IDC_GROUP_IAT_INFO, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_STATIC_OEPADDRESS, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_STATIC_IATADDRESS, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_STATIC_IATSIZE, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_OEP_ADDRESS, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_IAT_ADDRESS, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_IAT_SIZE, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_BTN_AUTO_SEARCH, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_BTN_GET_IMPORTS, DLSZ_MOVE_Y)

		DLGRESIZE_CONTROL(IDC_GROUP_DUMP, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_BTN_DUMP, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_BTN_PE_REBUILD, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_BTN_FIX_DUMP, DLSZ_MOVE_Y)

		DLGRESIZE_CONTROL(IDC_GROUP_LOG, DLSZ_MOVE_Y | DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_LIST_LOG, DLSZ_MOVE_Y | DLSZ_SIZE_X)
	END_DLGRESIZE_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	void OnSize(UINT nType, CSize size);

	void ProcessHandler();

protected:
	void SetupStatusBar();
	void UpdateStatusBar();

	enum StatusParts {
		Count = 0,
		Invalid,
		ImageBase,
	};

private:
	ProcessInfoEx& m_px;
	const WinSys::ProcessManager& m_pm;
	CStatusBarCtrl m_StatusBar;
	CContainedWindow m_ImportsTree;
	CListBox m_ListLog;
	CAccelerator m_Accelerators;
};