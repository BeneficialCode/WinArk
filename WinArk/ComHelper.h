#pragma once

struct ComHelper {
	template<typename TCollection,typename TItem>
	static void DoForeach(TCollection* coll, std::function<void(TItem*)> f) {
		LONG count;
		coll->get_Count(&count);
		for (LONG i = 1; i <= count; i++) {
			CComPtr<TItem> spItem;
			coll->get_Item(CComVariant(i), &spItem);
			if (spItem)
				f(spItem.p);
		}
	}

	template<typename TCollection, typename TItem>
	static void DoForeachNoVariant(TCollection* coll, std::function<void(TItem*)> f) {
		LONG count;
		coll->get_Count(&count);
		for (LONG i = 1; i <= count; i++) {
			CComPtr<TItem> spItem;
			coll->get_Item(i, &spItem);
			if (spItem)
				f(spItem.p);
		}
	}
};