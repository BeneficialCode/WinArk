#pragma once


class ListCtrlHelper {
public:
	template<typename T = int>
	static T FindColumn(CHeaderCtrl header, int id) {
		auto count = header.GetItemCount();
		HDITEM hdi;
		hdi.mask = HDI_LPARAM;
		for (int i = 0; i < count; i++) {
			header.GetItem(i, &hdi);
			if (hdi.lParam == id)
				return static_cast<T>(i);
		}
		return static_cast<T>(-1);
	}

	template<typename T = int>
	static T FindColumn(CListViewCtrl list, int id) {
		return FindColumn(list.GetHeader(), id);
	}
};