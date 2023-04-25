#include "stdafx.h"
#include "WFPFilterTable.h"
#include <WFPHelpers.h>
#include "SortHelper.h"

using namespace WinSys;

LRESULT CWFPFilterTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return 0;
}

LRESULT CWFPFilterTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	return 0;
}

LRESULT CWFPFilterTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CWFPFilterTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CWFPFilterTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CWFPFilterTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CWFPFilterTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CWFPFilterTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CWFPFilterTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}

LRESULT CWFPFilterTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CWFPFilterTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CWFPFilterTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CWFPFilterTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	CMenu menu;
	CMenuHandle hSubMenu;
	menu.LoadMenuW(IDR_KERNEL_HOOK_CONTEXT);
	hSubMenu = menu.GetSubMenu(1);
	POINT pt;
	::GetCursorPos(&pt);
	bool show = Tablefunction(m_hWnd, uMsg, wParam, lParam);
	if (show) {
		auto id = (UINT)TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, nullptr);
		if (id) {
			PostMessage(WM_COMMAND, id);
		}
	}
	return 0;
}

LRESULT CWFPFilterTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CWFPFilterTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CWFPFilterTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CWFPFilterTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

CWFPFilterTable::CWFPFilterTable(BarInfo& bars, TableInfo& table) :CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);
	Refresh();
}

int CWFPFilterTable::ParseTableEntry(CString& s, char& mask, int& select, WFPFilterInfo& info, int column) {
	switch (static_cast<TableColumn>(column))
	{
		case TableColumn::Name:
			s = info.Name.c_str();
			break;

		case TableColumn::FilterId:
			s.Format(L"%lu", info.FilterId);
			break;

		case TableColumn::Description:
			s = info.Description.c_str();
			break;

		case TableColumn::Flags:
			s = FlagToString(info.Flags);
			break;

		case TableColumn::Mode:
			info.IsUserMode ? s = L"User Mode" : s = L"Kernel Mode";
			break;

		case TableColumn::LayerName:
			s = info.LayerName.c_str();
			break;

		case TableColumn::ActionType:
			s = ActionTypeToString(info.ActionType);
			break;
	}
	return s.GetLength();
}

bool CWFPFilterTable::CompareItems(const WFPFilterInfo& s1, const WFPFilterInfo& s2, int col, bool asc) {
	switch (col)
	{
		case 0:
			return SortHelper::SortNumbers(s1.FilterId, s2.FilterId, asc);
			break;
		default:
			break;
	}
	return false;
}

void CWFPFilterTable::Refresh() {
	m_Table.data.info.clear();

	UINT32 status = NO_ERROR;
	
	HANDLE engineHandle = nullptr;
	HANDLE enumHandle = nullptr;
	FWPM_FILTER** ppFilters = nullptr;
	FWPM_FILTER_ENUM_TEMPLATE* pFilterEnumTemplate = nullptr;
	UINT32 numEntries = 0;
	do
	{
		//
		// open a handle to the WFP engine
		//
		status = WFPHelpers::HlprFwpmEngineOpen(&engineHandle);
		if (status != NO_ERROR)
			break;

		//
		// create an enumeration handle
		//
		status = WFPHelpers::HlprFwpmFilterCreateEnumHandle(engineHandle,
			pFilterEnumTemplate, &enumHandle);

		if (status != NO_ERROR)
			break;

		//
		// enumerate filters
		//
		status = WFPHelpers::HlprFwpmFilterEnum(engineHandle,
			enumHandle, INFINITE, &ppFilters, &numEntries);

		WFPFilterInfo info;
		if (status == NO_ERROR &&
			ppFilters && numEntries) {
			for (UINT32 filterIndex = 0; filterIndex < numEntries; filterIndex++) {
				info.FilterId = ppFilters[filterIndex]->filterId;
				info.Flags = ppFilters[filterIndex]->flags;
				info.Name = ppFilters[filterIndex]->displayData.name == nullptr ? L"" : ppFilters[filterIndex]->displayData.name;
				info.Description = ppFilters[filterIndex]->displayData.description == nullptr ? L"" :
					ppFilters[filterIndex]->displayData.description;
				info.IsUserMode = WFPHelpers::HlprFwpmLayerIsUserMode(&ppFilters[filterIndex]->layerKey);
				FWPM_LAYER* layer = nullptr;
				FwpmLayerGetByKey(engineHandle, &ppFilters[filterIndex]->layerKey, &layer);
				info.LayerName = layer->displayData.name;
				FwpmFreeMemory((void**)&layer);
				info.ActionType = ppFilters[filterIndex]->action.type;
				m_Table.data.info.push_back(std::move(info));
			}

			numEntries = 0;
		}
		//
		// free memory allocated by FwpmFilterEnum
		//
		FwpmFreeMemory((void**)&ppFilters);

		//
		// close enumeration handle
		//
		WFPHelpers::HlprFwpmFilterDestroyEnumHandle(engineHandle, &enumHandle);
	} while (false);
	
	if (engineHandle) {
		//
		// close engine handle
		//
		WFPHelpers::HlprFwpmEngineClose(&engineHandle);
	}

	m_Table.data.n = m_Table.data.info.size();
}

std::wstring CWFPFilterTable::GetSingleWFPInfo(WFPFilterInfo& info) {
	CString text;
	CString s;



	text += L"\r\n";

	return text.GetString();
}

LRESULT CWFPFilterTable::OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Refresh();

	return TRUE;
}

CString CWFPFilterTable::FlagToString(UINT32 flags) {
	static const struct {
		PCWSTR Text;
		DWORD Value;
	}flag[] = {
		{L"NONE",FWPM_FILTER_FLAG_NONE},
		{L"PERSISTENT",FWPM_FILTER_FLAG_PERSISTENT},
		{L"BOOTTIME",FWPM_FILTER_FLAG_BOOTTIME},
		{L"HAS_PROVIDER_CONTEXT",FWPM_FILTER_FLAG_HAS_PROVIDER_CONTEXT},
		{L"CLEAR_ACTION_RIGHT",FWPM_FILTER_FLAG_CLEAR_ACTION_RIGHT},
		{L"PERMIT_IF_CALLOUT_UNREGISTERED",FWPM_FILTER_FLAG_PERMIT_IF_CALLOUT_UNREGISTERED},
		{L"DISABLED",FWPM_FILTER_FLAG_DISABLED},
		{L"INDEXED",FWPM_FILTER_FLAG_INDEXED},
	};

	CString text;
	if (FWPM_FILTER_FLAG_NONE == flags)
		text = L"NONE | ";

	std::for_each(std::begin(flag), std::end(flag), [&text, flags](auto& p) {
		if (p.Value & flags)
			text += p.Text + CString(L" | ");
		});

	if (!text.IsEmpty())
		text = text.Left(text.GetLength() - 3);
	return text;
}

LRESULT CWFPFilterTable::OnDelete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);

	auto& info = m_Table.data.info[selected];

	HANDLE engineHandle = nullptr;
	UINT32 status = NO_ERROR;
	bool ok = false;
	do
	{
		status = WFPHelpers::HlprFwpmEngineOpen(&engineHandle);
		if (status != NO_ERROR)
			break;

		status = WFPHelpers::HlprFwpmFilterDeleteById(engineHandle, info.FilterId);
		if (status == ERROR_SUCCESS)
			ok = true;
	} while (false);
	
	if (engineHandle)
		WFPHelpers::HlprFwpmEngineClose(&engineHandle);

	if (!ok) {
		AtlMessageBox(m_hWnd, L"Delete failed!", L"Error", MB_ICONERROR);
	}
	if (ok) {
		Refresh();
		Invalidate();
	}

	return TRUE;
}

CString CWFPFilterTable::ActionTypeToString(UINT32 type) {
	switch (type) {
		case FWP_ACTION_BLOCK: return L"Block";
		case FWP_ACTION_PERMIT: return L"Permit";
		case FWP_ACTION_CALLOUT_TERMINATING: return L"Callout Terminating";
		case FWP_ACTION_CALLOUT_INSPECTION: return L"Callout Inspection";
		case FWP_ACTION_CALLOUT_UNKNOWN: return L"Callout Unknown";
	}
	return L"";
}