#pragma once

// Extended MultiSelectTree styles
static const DWORD MTVS_EX_NOMARQUEE = 0x00000001;

// New control notifications
static const UINT TVN_ITEMSELECTING = 0x0001;
static const UINT TVN_ITEMSELECTED = 0x0002;

static bool operator==(const CTreeItem& ti1, const CTreeItem& ti2)
{
	return ti1.m_hTreeItem == ti2.m_hTreeItem;
}

class CMultiSelectTreeViewCtrl :
	public CWindowImpl<CMultiSelectTreeViewCtrl, CTreeViewCtrlEx, CWinTraitsOR<TVS_SHOWSELALWAYS>>,
	public CCustomDraw<CMultiSelectTreeViewCtrl>
{
public:
	DECLARE_WND_SUPERCLASS(_T("WTL_MultiSelectTree"), GetWndClassName())


};