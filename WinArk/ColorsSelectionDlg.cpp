#include "stdafx.h"
#include "ColorsSelectionDlg.h"

LRESULT CColorsSelectionDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {

	return 0;
}

LRESULT CColorsSelectionDlg::OnCloseCmd(WORD, WORD wID, HWND, BOOL&) {
    if (wID == IDOK) {
       
    }
    EndDialog(wID);
    return 0;
}