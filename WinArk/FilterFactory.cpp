#include "stdafx.h"
#include "FilterFactory.h"
#include "EventNameFilter.h"
#include "ProcessIdFilter.h"
#include "ProcessNameFilter.h"
#include "PropertyNameFilter.h"
#include "PropertyValueFilter.h"

static PCWSTR names[] = {
	L"Process Name",L"Process Id",L"Event Name",L"Property Value",L"Property Name"
};

std::vector<CString> FilterFactory::GetFilterNames() {
	return std::vector<CString>(std::begin(names), std::end(names));
}

std::shared_ptr<FilterBase> FilterFactory::CreateFilter(PCWSTR name, CompareType type, PCWSTR params,
	FilterAction action) {
	std::shared_ptr<FilterBase> filter;

	for (int i = 0; i < _countof(names); i++) {
		if (::wcscmp(name, names[i]) == 0) {
			switch (i)
			{
				case 0:
					filter = std::make_shared<ProcessNameFilter>(params, type, action);
					break;

				case 1:
					filter = std::make_shared<ProcessIdFilter>(_wtoi(params), type, action);
					break;

				case 2:
					filter = std::make_shared<EventNameFilter>(params, type, action);
					break;

				case 3:
					filter = std::make_shared<PropertyValueFilter>(params, type, action);
					break;

				case 4:
					filter = std::make_shared<PropertyNameFilter>(params, type, action);
					break;
			}
		}
	}
	return filter;
}