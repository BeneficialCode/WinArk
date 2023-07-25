#include "stdafx.h"
#include "ColorBox.h"

LRESULT CColorBox::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return FALSE;
}

LRESULT CColorBox::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	return FALSE;
}

COLORREF CColorBox::MakeColorBrighter(COLORREF color, UCHAR increment) {
	UCHAR r, g, b;

	r = (UCHAR)color;
	g = (UCHAR)(color >> 8);
	b = (UCHAR)(color >> 16);

	if (r <= 255 - increment)
		r += increment;
	else
		r = 255;

	if (g <= 255 - increment)
		g += increment;
	else
		g = 255;

	if (b <= 255 - increment)
		b += increment;
	else
		b = 255;

	return RGB(r, g, b);
}

LRESULT CColorBox::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	CRect rc;
	GetClientRect(&rc);
	CPaintDC dc(m_hWnd);
	dc.SetDCPenColor(RGB(0x44, 0x44, 0x44));
	if (!_context.Hot && !_context.HasFocus)
		dc.SetDCBrushColor(_context.SelectedColor);
	else
		dc.SetDCBrushColor(MakeColorBrighter(_context.SelectedColor, 64));
	dc.SelectPen((HPEN)GetStockObject(DC_PEN));
	dc.SelectBrush((HBRUSH)GetStockObject(DC_BRUSH));
	dc.Rectangle(&rc);
	return TRUE;
}

LRESULT CColorBox::OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return TRUE;
}

LRESULT CColorBox::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if (!_context.Hot) {
		TRACKMOUSEEVENT trackMouseEvent = { sizeof(trackMouseEvent) };
		_context.Hot = TRUE;
		InvalidateRect(NULL);
		trackMouseEvent.dwFlags = TME_LEAVE;
		trackMouseEvent.hwndTrack = m_hWnd;
		TrackMouseEvent(&trackMouseEvent);
	}
	return FALSE;
}

LRESULT CColorBox::OnMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	_context.Hot = FALSE;
	InvalidateRect(NULL);
	return TRUE;
}

LRESULT CColorBox::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	SelectColor();
	return TRUE;
}

LRESULT CColorBox::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	_context.HasFocus = TRUE;
	InvalidateRect(NULL);
	return 0;
}

LRESULT CColorBox::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	_context.HasFocus = FALSE;
	InvalidateRect(NULL);
	return 0;
}

LRESULT CColorBox::OnGetDlgCode(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if (wParam == VK_RETURN)
		return DLGC_WANTMESSAGE;
	return 0;
}

LRESULT CColorBox::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	switch (wParam)
	{
	case VK_SPACE:
	case VK_RETURN:
		SelectColor();
		break;
	default:
		break;
	}
	return TRUE;
}

void CColorBox::SelectColor() {
	CColorDialog dlg(_context.SelectedColor, CC_FULLOPEN, *this);
	if (dlg.DoModal() == IDOK)
		_context.SelectedColor = dlg.GetColor();
	InvalidateRect(NULL);
}

void CColorBox::SetColor(COLORREF color) {
	_context.SelectedColor = color;
	if (IsWindow()) {
		InvalidateRect(NULL);
	}
}

COLORREF CColorBox::GetColor() {
	return _context.SelectedColor;
}

void CColorBox::EnableThemeSupport(bool support) {
	_context.EnableThemeSupport = support;
}
