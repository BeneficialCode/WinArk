#pragma once
#include "resource.h"
#include "HexEdit.h"



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
	END_DDX_MAP()

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	CScyllaDlg(const WinSys::ProcessManager& pm, ProcessInfoEx& px) :m_pm(pm), m_px(px) {}

	BEGIN_MSG_MAP_EX(CScyllaDlg)
		MSG_WM_SIZE(OnSize)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()


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
	
	CListBox m_ListLog;
	CAccelerator m_Accelerators;
	CHexEdit _oepAddress;
	CHexEdit _iatAddress;
	CHexEdit _iatSize;
};