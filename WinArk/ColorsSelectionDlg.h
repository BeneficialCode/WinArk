#pragma once
#include "resource.h"
#include "ColorBox.h"
#include "ThemeColor.h"

class CColorsSelectionDlg :
	public CDialogImpl<CColorsSelectionDlg>{
public:
	enum {IDD = IDD_COLORS};

	CColorsSelectionDlg(ThemeColor* colors, int count,int opacity);

	const ThemeColor* GetColors() const;

	const int GetOpacity() const;

	BEGIN_MSG_MAP(CColorsSelectionDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_HSCROLL,OnHScroll)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_LOAD,OnLoad)
		COMMAND_ID_HANDLER(IDC_SAVE,OnSave)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnHScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSave(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnLoad(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	void SetColor(int i);
	void SetColor();
private:
	CScrollBar _ScrollBar;
	int _opacity;

	CColorBox _cb[8];
	std::vector<ThemeColor> m_Colors;
	UINT m_CountColors;
};