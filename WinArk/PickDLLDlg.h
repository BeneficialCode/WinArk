#pragma once
#include "resource.h"
#include "ProcessAccessHelper.h"

class CPickDLLDlg : public CDialogImpl<CPickDLLDlg>, public CWinDataExchange<CPickDLLDlg>,
	public CDialogResize<CPickDLLDlg>
{
public:
	enum { IDD = IDD_PICK_DLL };

	CPickDLLDlg(std::vector<ModuleInfo>& moduleList);

	ModuleInfo* GetSelectedModule() const { return _pSelectedModule; }

protected:

	// Variables

	std::vector<ModuleInfo>& _moduleList;
	ModuleInfo* _pSelectedModule = nullptr;

	// Controls

	CListViewCtrl _DLLsListView;

	enum ListColumns {
		Name,
		ImageBase,
		ImageSize,
		Path
	};

	int _prevColumn = -1;
	bool _ascending = true;

protected:

	// Message handlers

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);

	LRESULT OnListDllColumnClicked(NMHDR* pnmh);
	LRESULT OnListDllDoubleClick(NMHDR* pnmh);
	void OnOK(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);

	// GUI functions

	void AddColumnsToModuleList(CListViewCtrl& list);
	void DisplayModuleList(CListViewCtrl& list);

	static int CALLBACK ListViewCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

public:
	BEGIN_DDX_MAP(CPickDLLDlg)
		DDX_CONTROL_HANDLE(IDC_DLL_LIST, _DLLsListView)
	END_DDX_MAP()

	BEGIN_MSG_MAP(CPickDLLDlg)
		MSG_WM_INITDIALOG(OnInitDialog)

		NOTIFY_HANDLER_EX(IDC_DLL_LIST, LVN_COLUMNCLICK, OnListDllColumnClicked)
		NOTIFY_HANDLER_EX(IDC_DLL_LIST, NM_DBLCLK, OnListDllDoubleClick)

		COMMAND_ID_HANDLER_EX(IDOK, OnOK)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)

		CHAIN_MSG_MAP(CDialogResize<CPickDLLDlg>)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CPickDLLDlg)
		DLGRESIZE_CONTROL(IDC_DLL_LIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()
};