#pragma once


template<typename T>
class CTreeViewManager {
public:
	BEGIN_MSG_MAP(CTreeViewManager)
		NOTIFY_CODE_HANDLER(TVN_SELCHANGED, OnTreeSelectionChanged)
	END_MSG_MAP()

	LRESULT OnTreeSelectionChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
		return 0;
	}
};