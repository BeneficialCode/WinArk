#pragma once
#include "resource.h"
#include "ProcessInfoEx.h"
#include "ProcessManager.h"

class CProcessPropertiesDlg :
	public CDialogImpl<CProcessPropertiesDlg>,
	public CDynamicDialogLayout<CProcessPropertiesDlg> {
public:
	enum {IDD = IDD_PROCESS_PROPERTIES};

	CProcessPropertiesDlg(const WinSys::ProcessManager& pm,ProcessInfoEx& px):m_pm(pm),m_px(px){}
	void SetModal(bool modal) {
		m_Modal = modal;
	}

	void OnFinalMessage(HWND) override;

	BEGIN_MSG_MAP(CProcessPropertiesDlg)
		MESSAGE_HANDLER(WM_GETMINMAXINFO,OnGetMinMaxInfo)
		MESSAGE_HANDLER(WM_INITDIALOG,OnInitDialog)
		COMMAND_ID_HANDLER(IDCANCEL,OnCloseCmd)
		COMMAND_ID_HANDLER(IDOK,OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_JOB,OnShowJob)
		COMMAND_ID_HANDLER(IDC_TOKEN,OnShowToken)
		COMMAND_ID_HANDLER(IDC_ENV,OnShowEnvironment)
	END_MSG_MAP()
private:
	void InitProcess();

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnGetMinMaxInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnShowToken(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnShowEnvironment(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnShowJob(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	ProcessInfoEx& m_px;
	const WinSys::ProcessManager& m_pm;
	bool m_Modal{ false };
};