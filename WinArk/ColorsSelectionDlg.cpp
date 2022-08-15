#include "stdafx.h"
#include "ColorsSelectionDlg.h"

DWORD CColorsSelectionDlg::OnPrePaint(int, LPNMCUSTOMDRAW) {
    return CDRF_NOTIFYITEMDRAW;
}

DWORD CColorsSelectionDlg::OnPostErase(int, LPNMCUSTOMDRAW cd) {
    ::SetTextColor(cd->hdc, RGB(255, 255, 255));

    return CDRF_SKIPPOSTPAINT;
}

DWORD CColorsSelectionDlg::OnPreErase(int, LPNMCUSTOMDRAW cd) {
    auto id = (UINT)cd->hdr.idFrom;

    if (id == IDC_TEXT_COLOR) {
        CDCHandle dc(cd->hdc);
        CRect rc(cd->rc);
        dc.FillRect(&rc, ::CreateSolidBrush(g_myColor[g_myScheme[0].textcolor]));
    }

    if (id == IDC_HIT_TEXT_COLOR) {
        CDCHandle dc(cd->hdc);
        CRect rc(cd->rc);
        UINT i = id - IDC_TEXT_COLOR;
        dc.FillSolidRect(&rc, g_myColor[g_myScheme[0].hitextcolor]);
        dc.SetBkMode(TRANSPARENT);
    }

    if (id == IDC_LOW_COLOR) {
        CDCHandle dc(cd->hdc);
        CRect rc(cd->rc);
        UINT i = id - IDC_TEXT_COLOR;
        rc.InflateRect(-20, 10, -10, 10);
        dc.FillSolidRect(&rc, g_myColor[g_myScheme[0].lowcolor]);
        dc.SetBkMode(TRANSPARENT);
    }

    if (id == IDC_BK_COLOR) {
        CDCHandle dc(cd->hdc);
        CRect rc(cd->rc);
        UINT i = id - IDC_TEXT_COLOR;
        rc.InflateRect(-20, 10, -10, 10);
        dc.FillSolidRect(&rc, g_myColor[g_myScheme[0].bkcolor]);
        dc.SetBkMode(TRANSPARENT);
    }

    if (id == IDC_SEL_BK_COLOR) {
        CDCHandle dc(cd->hdc);
        CRect rc(cd->rc);
        UINT i = id - IDC_TEXT_COLOR;
        rc.InflateRect(-20, 10, -10, 10);
        dc.FillSolidRect(&rc, g_myColor[g_myScheme[0].selbkcolor]);
        dc.SetBkMode(TRANSPARENT);
    }

    if (id == IDC_LINE_COLOR) {
        CDCHandle dc(cd->hdc);
        CRect rc(cd->rc);
        UINT i = id - IDC_TEXT_COLOR;
        rc.InflateRect(-20, 10, -10, 10);
        dc.FillSolidRect(&rc, g_myColor[g_myScheme[0].linecolor]);
        dc.SetBkMode(TRANSPARENT);
    }

    if (id == IDC_AUX_COLOR) {
        CDCHandle dc(cd->hdc);
        CRect rc(cd->rc);
        UINT i = id - IDC_TEXT_COLOR;
        rc.InflateRect(-20, 10, -10, 10);
        dc.FillSolidRect(&rc, g_myColor[g_myScheme[0].auxcolor]);
        dc.SetBkMode(TRANSPARENT);
    }

    if (id == IDC_CONDBK_COLOR) {
        CDCHandle dc(cd->hdc);
        CRect rc(cd->rc);
        UINT i = id - IDC_TEXT_COLOR;
        rc.InflateRect(-20, 10, -10, 10);
        dc.FillSolidRect(&rc, g_myColor[g_myScheme[0].condbkcolor]);
        dc.SetBkMode(TRANSPARENT);
    }

    return CDRF_SKIPPOSTPAINT;
}

LRESULT CColorsSelectionDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
    _ScrollBar.Attach(GetDlgItem(IDC_OPACITY));
    _ScrollBar.SetScrollRange(180, 255, FALSE);
    _ScrollBar.SetScrollPos(255, FALSE);
    _cb[0].Attach(GetDlgItem(IDC_TEXT_COLOR));
    _cb[1].Attach(GetDlgItem(IDC_HIT_TEXT_COLOR));
    _cb[2].Attach(GetDlgItem(IDC_LOW_COLOR));
    _cb[3].Attach(GetDlgItem(IDC_BK_COLOR));
    _cb[4].Attach(GetDlgItem(IDC_SEL_BK_COLOR));
    _cb[5].Attach(GetDlgItem(IDC_LINE_COLOR));
    _cb[6].Attach(GetDlgItem(IDC_AUX_COLOR));
    _cb[7].Attach(GetDlgItem(IDC_CONDBK_COLOR));


	return 0;
}

LRESULT CColorsSelectionDlg::OnCloseCmd(WORD, WORD wID, HWND, BOOL&) {
    if (wID == IDOK) {
       
    }
    EndDialog(wID);
    return 0;
}

LRESULT CColorsSelectionDlg::OnHScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
    switch (LOWORD(wParam))
    {
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            _opacity = HIWORD(wParam);
            break;
        default:
            break;
    }
    _ScrollBar.SetScrollPos(_opacity);
    CString value;
    value.Format(L"%.2f", (float)_opacity / 255);
    SetDlgItemText(IDC_VALUE, value);
    SetLayeredWindowAttributes(GetParent().m_hWnd, 0xffffff,_opacity, LWA_ALPHA);
    return 0;
}