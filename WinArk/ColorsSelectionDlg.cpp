#include "stdafx.h"
#include "ColorsSelectionDlg.h"
#include "ThemeSystem.h"
#include "IniFile.h"

static WCHAR iniFileFilter[] = L"Ini files (*.ini)\0*.ini\0All File\0*.*\0";

CColorsSelectionDlg::CColorsSelectionDlg(ThemeColor* colors, int count,
    int opacity) 
    :m_Colors(colors,colors+count),m_CountColors(count),_opacity(opacity){
    ATLASSERT(colors);
}

LRESULT CColorsSelectionDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
    _ScrollBar.Attach(GetDlgItem(IDC_OPACITY));
    _ScrollBar.SetScrollRange(180, 255, FALSE);
    _ScrollBar.SetScrollPos(255, FALSE);
    CRect rc{ 120,60,216,80 };
    DWORD cbStyle = 0x54010000;

    for (UINT i = 0; i < m_CountColors; i++) {
        auto color = m_Colors.begin() + i;
        _cb[i].SetColor(color->Color);
        _cb[i].EnableThemeSupport(true);
        _cb[i].Create(*this, &rc, L"", cbStyle, 0, IDC_TEXT_COLOR + i);
        rc.OffsetRect(0, rc.Height() + 12);
    }
	return 0;
}


LRESULT CColorsSelectionDlg::OnCloseCmd(WORD, WORD wID, HWND, BOOL&) {
    if (wID == IDOK) {
       for (UINT i = 0; i < m_CountColors; i++) {
           m_Colors[i].Color = _cb[i].GetColor();
           SetColor(i);
       }
       InitPenSys();
       InitBrushSys();
    }
    KillTimer(100);
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

const ThemeColor* CColorsSelectionDlg::GetColors() const {
    return m_Colors.data();
}

void CColorsSelectionDlg::SetColor(int i) {
    switch (i) {
        case 0:
            g_myColor[g_myScheme[0].textcolor] = _cb[i].GetColor();
            break;

        case 1:
            g_myScheme[0].hitextcolor = _cb[i].GetColor();
            break;

        case 2:
            g_myColor[g_myScheme[0].lowcolor] = _cb[i].GetColor();
            break;

        case 3:
            g_myColor[g_myScheme[0].bkcolor] = _cb[i].GetColor();
            break;

        case 4:
            g_myColor[g_myScheme[0].selbkcolor] = _cb[i].GetColor();
            break;

        case 5:
            g_myColor[g_myScheme[0].linecolor] = _cb[i].GetColor();
            break;

        case 6:
            g_myColor[g_myScheme[0].auxcolor] = _cb[i].GetColor();
            break;

        case 7:
            g_myColor[g_myScheme[0].condbkcolor] = _cb[i].GetColor();
            break;
    }
}

LRESULT CColorsSelectionDlg::OnSave(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    CSimpleFileDialog dlg(FALSE, L"ini", nullptr, OFN_EXPLORER | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT,
        iniFileFilter, *this);
    if (dlg.DoModal() == IDOK) {
        SaveColors(dlg.m_szFileName, L"TableColor", m_Colors.data(), m_CountColors);
        IniFile file(dlg.m_szFileName);
        file.WriteInt(L"Opacity", L"Value", _opacity);
    }
    return 0;
}

void CColorsSelectionDlg::SetColor() {
    for (UINT i = 0; i < m_CountColors; i++) {
        switch (i) {
            case 0:
                g_myColor[g_myScheme[0].textcolor] = m_Colors[i].Color;
                break;

            case 1:
                g_myScheme[0].hitextcolor = m_Colors[i].Color;
                break;

            case 2:
                g_myColor[g_myScheme[0].lowcolor] = m_Colors[i].Color;
                break;

            case 3:
                g_myColor[g_myScheme[0].bkcolor] = m_Colors[i].Color;
                break;

            case 4:
                g_myColor[g_myScheme[0].selbkcolor] = m_Colors[i].Color;
                break;

            case 5:
                g_myColor[g_myScheme[0].linecolor] = m_Colors[i].Color;
                break;

            case 6:
                g_myColor[g_myScheme[0].auxcolor] = m_Colors[i].Color;
                break;

            case 7:
                g_myColor[g_myScheme[0].condbkcolor] = m_Colors[i].Color;
                break;
        }
        _cb[i].SetColor(m_Colors[i].Color);
    }
}

LRESULT CColorsSelectionDlg::OnLoad(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    CSimpleFileDialog dlg(TRUE, L"ini", nullptr, OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST,
        iniFileFilter, *this);
    if (dlg.DoModal() == IDOK) {
        if (LoadColors(dlg.m_szFileName, L"TableColor", m_Colors.data(), m_CountColors)) {
            SetColor();
        }
        IniFile file(dlg.m_szFileName);
        _opacity = file.ReadInt(L"Opacity", L"Value", 255);
    }
    return 0;
}

const int CColorsSelectionDlg::GetOpacity() const {
    return _opacity;
}