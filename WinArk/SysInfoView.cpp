#include "stdafx.h"
#include "SysInfoView.h"
#include "SortHelper.h"
#include "FormatHelper.h"
#include "Helpers.h"

using namespace WinSys;

CSysInfoView::CSysInfoView(IMainFrame* frame) :CViewBase(frame),
m_Items{
	ItemData(RowType::Version,L"Windows Version",ItemDataFlags::Const | ItemDataFlags::Special),
	ItemData(RowType::BootTime,L"Boot Time",ItemDataFlags::Const | ItemDataFlags::Time | ItemDataFlags::Special),
	ItemData(RowType::TotalRAM,L"Usable RAM",ItemDataFlags::Const | ItemDataFlags::Special),
	ItemData(RowType::TotalCPUs,L"Processor Count",&m_CpuCount,&m_CpuCount,ItemDataFlags::Bits32 | ItemDataFlags::Const),
	ItemData(RowType::ProcessorBrand,L"Processor Brand",ItemDataFlags::Const | ItemDataFlags::Special),
	ItemData(RowType::BootMode,L"Boot Mode",ItemDataFlags::Const | ItemDataFlags::Special),
	ItemData(RowType::SecureBoot,L"Secure Boot",ItemDataFlags::Const | ItemDataFlags::Special),
	ItemData(RowType::HVCIStatus,L"HVCI Status",ItemDataFlags::Const | ItemDataFlags::Special),
	ItemData(RowType::Vendor,L"Processor Vendor",ItemDataFlags::Const | ItemDataFlags::Special),
	ItemData(RowType::Hypervisor,L"Hypervisor",ItemDataFlags::Const | ItemDataFlags::Special),
	}
{
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	m_CpuCount = info.dwNumberOfProcessors;
}

LRESULT CSysInfoView::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	m_hWndClient = m_List.Create(*this, rcDefault, nullptr, ListViewDefaultStyle & ~LVS_SHAREIMAGELISTS);
	m_List.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_FULLROWSELECT);

	m_List.InsertColumn(0, L"Name", LVCFMT_LEFT, 150);
	m_List.InsertColumn(1, L"Value", LVCFMT_LEFT, 300);

	m_BasicalSysInfo = SystemInformation::GetBasicSystemInfo();
	int count = (int)m_Items.size();
	m_List.SetItemCount(count);
	
	Refresh();
	
	return 0;
}

CSysInfoView::ItemData::ItemData(RowType type, PCWSTR name, uint32_t* pCurrent, uint32_t* pPrevious, ItemDataFlags flags)
	: Type(type), Name(name), Flags(flags) {
	Current.Value32 = pCurrent;
	Previous.Value32 = pPrevious;
}

CSysInfoView::ItemData::ItemData(RowType type, PCWSTR name, DWORD* pCurrent, DWORD* pPrevious, ItemDataFlags flags)
	: ItemData(type, name, (uint32_t*)pCurrent, (uint32_t*)pPrevious, flags) {
}

CSysInfoView::ItemData::ItemData(RowType type, PCWSTR name, uint64_t* pCurrent, uint64_t* pPrevious, ItemDataFlags flags)
	: Type(type), Name(name), Flags(flags) {
	Current.Value64 = pCurrent;
	Previous.Value64 = pPrevious;
}

CSysInfoView::ItemData::ItemData(RowType type, PCWSTR name, int64_t* pCurrent, int64_t* pPrevious, ItemDataFlags flags)
	: ItemData(type, name, (uint64_t*)pCurrent, (uint64_t*)pPrevious, flags) {
}

CSysInfoView::ItemData::ItemData(RowType type, PCWSTR name, ItemDataFlags flags) : Type(type), Name(name), Flags(flags) {
	Current.Value64 = Previous.Value64 = nullptr;
}

bool CSysInfoView::IsSortable(HWND, int row) const {
	return false;
}

void CSysInfoView::OnUpdate() {
	Refresh();
	m_List.RedrawItems(m_List.GetTopIndex(), m_List.GetTopIndex() + m_List.GetCountPerPage());
}

void CSysInfoView::Refresh() {

}

void CSysInfoView::DoSort(const SortInfo* si) {
	std::sort(m_Items.begin(), m_Items.end(), [&](auto& i1, auto& i2) {
		return SortHelper::SortStrings(i1.Name, i2.Name, si->SortAscending);
		});
}

CString CSysInfoView::GetColumnText(HWND, int row, int col) const {
	auto& item = m_Items[row];
	if (col == 0) {
		return item.Name;
	}

	CString text;
	auto flags = item.Flags;

	if ((flags & ItemDataFlags::Special) == ItemDataFlags::None) {

		if ((flags & ItemDataFlags::MemoryInPages) == ItemDataFlags::MemoryInPages) {
			return FormatHelper::FormatWithCommas(*item.Current.Value32 >> 8) + L" MB";
		}
		if ((flags & ItemDataFlags::Bits32) == ItemDataFlags::Bits32)
			return FormatHelper::FormatWithCommas(*item.Current.Value32);
		if ((flags & ItemDataFlags::TimeSpan) == ItemDataFlags::TimeSpan)
			return FormatHelper::TimeSpanToString(*item.Current.Value64);
		if ((flags & ItemDataFlags::Bits32) == ItemDataFlags::None)
			return FormatHelper::FormatWithCommas(*item.Current.Value64);
	}

	switch (item.Type) {
	case RowType::BootTime: return FormatHelper::TimeToString(SystemInformation::GetBootTime());

	case RowType::TotalRAM:
		text = FormatHelper::FormatWithCommas(SystemInformation::GetBasicSystemInfo().TotalPhysicalInPages >> 8) + L" MB";
		text.Format(L"%s (%u GB)", text, (ULONG)((SystemInformation::GetBasicSystemInfo().TotalPhysicalInPages + (1 << 17)) >> 18));
		break;

	case RowType::Version:
	{
		auto& ver = SystemInformation::GetWindowsVersion();
		text.Format(L"%u.%u.%u", ver.Major, ver.Minor, ver.Build);
		break;
	}

	case RowType::ProcessorBrand:
	{
		std::string brand = SystemInformation::GetCpuBrand();
		text = Helpers::StringToWstring(brand).c_str();
		break;
	}

	case RowType::BootMode:
	{
		GetFirmwareEnvironmentVariable(L"",
			L"{00000000-0000-0000-0000-000000000000}",
			nullptr, 0);
		if (GetLastError() == ERROR_INVALID_FUNCTION) {
			text =  L"Legacy";
		}
		else {
			text = L"UEFI";
		}
		break;
	}

	case RowType::SecureBoot:
	{
		BOOL bSecureBoot = FALSE;
		GetFirmwareEnvironmentVariable(
			L"SecureBoot",
			L"{8be4df61-93ca-11d2-aa0d-00e098032b8c}",
			&bSecureBoot,
			sizeof(BOOLEAN));
		bSecureBoot ? text = L"Enable" : text = L"Disable";
		break;
	}

	case RowType::HVCIStatus:
	{
		BOOLEAN kmci = FALSE;
		BOOLEAN strict = FALSE;
		BOOLEAN ium = FALSE;
		SystemInformation::GetCodeIntegrityInformation(&kmci, &strict, &ium);
		if (kmci) {
			text += L"KMCI | ";
			if(strict)
				text += L"Strict Mode | ";
			if (ium)
				text += L"IUM";
		}
		else {
			text = L"Disable";
		}
		break;
	}

	case RowType::Vendor:
	{
		int cpuInfo[4];
		CpuIdEx(cpuInfo, 0, 0);
		std::string vendor;
		vendor += std::string((const char*)&cpuInfo[1], 4);
		vendor += std::string((const char*)&cpuInfo[3], 4);
		vendor += std::string((const char*)&cpuInfo[2], 4);
		text += Helpers::StringToWstring(vendor).c_str();
		break;
	}

	case RowType::Hypervisor:
	{
		int cpuInfo[4];
		CpuIdEx(cpuInfo, 1, 0);
		if (cpuInfo[2] & 0x80000000) {
			CpuIdEx(cpuInfo, 0x40000000, 0);
			std::string vm;
			vm += std::string((const char*)&cpuInfo[1], 4);
			vm += std::string((const char*)&cpuInfo[2], 4);
			vm += std::string((const char*)&cpuInfo[3], 4);
			text += Helpers::StringToWstring(vm).c_str();
		}
		else {
			text += "not presend ";
		}
		break;
	}

	default:
		ATLASSERT(false);
		break;
	}
	return text;
}