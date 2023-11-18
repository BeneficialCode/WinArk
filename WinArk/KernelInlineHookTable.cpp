#include "stdafx.h"
#include "KernelInlineHookTable.h"
#include "DriverHelper.h"
#include "SymbolHelper.h"
#include "SortHelper.h"
#include "ClipboardHelper.h"
#include "PEParser.h"


CKernelInlineHookTable::CKernelInlineHookTable(BarInfo& bars, TableInfo& table, ULONG_PTR base)
	:CTable(bars, table), _base(base) {
	SetTableWindowInfo(bars.nbar);
}

CString CKernelInlineHookTable::TypeToString(KernelHookType type) {
	switch (type)
	{
		case KernelHookType::x64HookType1:
			return L"x64HookType1";
		case KernelHookType::x64HookType2:
			return L"x64HookType2";
		case KernelHookType::x64HookType3:
			return L"x64HookType3";
		default:
			return L"Unknown Type";
	}
}

int CKernelInlineHookTable::ParseTableEntry(CString& s, char& mask, int& select, KernelInlineHookInfo& info, int column) {
	switch (static_cast<Column>(column))
	{
		case Column::HookObject:
			s = info.ModuleName.c_str();
			break;

		case Column::HookType:
			s = TypeToString(info.Type);
			break;

		case Column::Address:
		{
			s.Format(L"0x%p  ", info.Address);
			DWORD64 offset = 0;
			auto symbol = SymbolHelper::GetSymbolFromAddress(info.Address, &offset);
			if (symbol) {
				std::string name = symbol->GetSymbolInfo()->Name;
				s += Helpers::StringToWstring(name).c_str();
				if (offset != 0) {
					CString temp;
					temp.Format(L"+ 0x%x", offset);
					s += temp;
				}

			}
			break;
		}

		case Column::Module:
			s = info.TargetModule.c_str();
			break;

		case Column::TargetAddress:
		{
			s.Format(L"0x%p", info.TargetAddress);
			break;
		}

		default:
			break;
	}
	return s.GetLength();
}

bool CKernelInlineHookTable::CompareItems(const KernelInlineHookInfo& s1, const KernelInlineHookInfo& s2, int col, bool asc) {
	switch (static_cast<Column>(col))
	{
		case Column::Module:
			return SortHelper::SortStrings(s1.TargetModule, s2.TargetModule, asc);
			break;
		case Column::TargetAddress:
			return SortHelper::SortNumbers(s1.TargetAddress, s2.TargetAddress, asc);
			break;
		default:
			break;
	}
	return false;
}

// we have to specify the architectures explicitly when install capstone by vcpkg
LRESULT CKernelInlineHookTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Refresh();

	return 0;
}

LRESULT CKernelInlineHookTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL&) {

	return 0;
}

LRESULT CKernelInlineHookTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CKernelInlineHookTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelInlineHookTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelInlineHookTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelInlineHookTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelInlineHookTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelInlineHookTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}

LRESULT CKernelInlineHookTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CKernelInlineHookTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CKernelInlineHookTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CKernelInlineHookTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	CMenu menu;
	CMenuHandle hSubMenu;
	menu.LoadMenu(IDR_KERNEL_HOOK_CONTEXT);
	hSubMenu = menu.GetSubMenu(2);
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

LRESULT CKernelInlineHookTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CKernelInlineHookTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CKernelInlineHookTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

LRESULT CKernelInlineHookTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

void CKernelInlineHookTable::Refresh() {
	m_Table.data.info.clear();
	ULONG count = DriverHelper::GetKernelInlineHookCount(_base);
	if (count == 0) {
		return;
	}
	DWORD size = (count + 10) * sizeof(KernelInlineHookData);
	wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size,
		MEM_COMMIT, PAGE_READWRITE));
	KernelInlineHookData* pData = (KernelInlineHookData*)buffer.get();
	KInlineData info;
	info.Count = count;
	info.ImageBase = _base;
	bool success = DriverHelper::DetectInlineHook(&info, pData, size);
	if (success) {
		KernelInlineHookInfo info;
		for (ULONG i = 0; i < count; i++) {
			info.Address = pData[i].Address;
			std::string name = Helpers::GetKernelModuleNameByAddress(
				pData[i].Address);
			info.ModuleName = Helpers::StringToWstring(name);
			info.TargetAddress = pData[i].TargetAddress;
			info.Type = pData[i].Type;
			if (!CheckIsHooked(info.Address, info.TargetAddress, info.Type)) {
				continue;
			}
			std::string path = Helpers::GetKernelModuleByAddress(info.TargetAddress);
			info.TargetModule = Helpers::StringToWstring(path);
			m_Table.data.info.push_back(info);
		}
	}
	m_Table.data.n = m_Table.data.info.size();
}

LRESULT CKernelInlineHookTable::OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Refresh();
	Invalidate(True);
	return TRUE;
}

LRESULT CKernelInlineHookTable::OnHookCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& info = m_Table.data.info[selected];

	std::wstring text = GetSingleHookInfo(info);
	ClipboardHelper::CopyText(m_hWnd, text.c_str());
	return 0;
}

LRESULT CKernelInlineHookTable::OnHookExport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CSimpleFileDialog dlg(FALSE, nullptr, L"*.txt",
		OFN_EXPLORER | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		L"文本文档 (*.txt)\0*.txt\0所有文件\0*.*\0", m_hWnd);
	if (dlg.DoModal() == IDOK) {
		auto hFile = ::CreateFile(dlg.m_szFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
			return FALSE;
		for (int i = 0; i < m_Table.data.n; ++i) {
			auto& info = m_Table.data.info[i];
			std::wstring text = GetSingleHookInfo(info);
			Helpers::WriteString(hFile, text);
		}
		::CloseHandle(hFile);
	}
	return TRUE;
}

std::wstring CKernelInlineHookTable::GetSingleHookInfo(KernelInlineHookInfo& info) {
	CString text;
	CString s;


	s = info.ModuleName.c_str();
	s += L"\t";
	text += s;


	s = TypeToString(info.Type);
	s += L"\t";
	text += s;


	s.Format(L"0x%p  ", info.Address);
	DWORD64 offset = 0;
	auto symbol = SymbolHelper::GetSymbolFromAddress(info.Address, &offset);
	if (symbol) {
		std::string name = symbol->GetSymbolInfo()->Name;
		s += Helpers::StringToWstring(name).c_str();
		if (offset != 0) {
			CString temp;
			temp.Format(L"+ 0x%x", offset);
			s += temp;
		}
	}
	s += L"\t";
	text += s;


	s = info.TargetModule.c_str();
	s += L"\t";
	text += s;


	s.Format(L"0x%p", info.TargetAddress);
	text += L"\r\n";

	return text.GetString();
}


bool CKernelInlineHookTable::CheckIsHooked(ULONG_PTR address, ULONG_PTR targetAddress, KernelHookType type) {
	std::string path = Helpers::GetKernelModuleByAddress(address);
	std::wstring wpath = Helpers::StringToWstring(path);

	bool isX64Module = true;
	bool isCheckCode = true;
	void* local_image_base = nullptr;

	PEParser parser(wpath.c_str());
	isX64Module = parser.IsPe64();

	uint32_t image_size = parser.GetImageSize();
	local_image_base = ::VirtualAlloc(nullptr, image_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!local_image_base)
		return true;

	bool bHooked = true;

	if (local_image_base) {
		BYTE* data = (BYTE*)parser.GetBaseAddress();
		LARGE_INTEGER fileSize = parser.GetFileSize();

		uint64_t real_image_base = _base;

		// Copy image headers
		memcpy(local_image_base, data, parser.GetHeadersSize());

		// Copy image sections
		for (auto i = 0; i < parser.GetSectionCount(); ++i) {
			auto section = parser.GetSectionHeader(i);
			if ((section[i].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) > 0)
				continue;
			auto local_section = reinterpret_cast<void*>(reinterpret_cast<uint64_t>(local_image_base)
				+ section[i].VirtualAddress);
			if (section[i].PointerToRawData + section[i].SizeOfRawData > fileSize.QuadPart) {
				continue;
			}
			memcpy(local_section, reinterpret_cast<void*>(reinterpret_cast<uint64_t>(data)
				+ section[i].PointerToRawData), section[i].SizeOfRawData);
		}

		std::vector<RelocInfo> relocs = parser.GetRelocs(local_image_base);
		PEParser::RelocateImageByDelta(relocs, real_image_base - parser.GetImageBase());
		ULONG_PTR originalAddr = 0;
		ULONG rva = address - _base;
		switch (type)
		{
			case KernelHookType::x64HookType1:
			{
				PULONG_PTR pAddr = reinterpret_cast<PULONG_PTR>((PUCHAR)local_image_base + rva + 2);
				originalAddr = *pAddr;
				break;
			}
			case KernelHookType::x64HookType2:
			{
				ULONG lowAddr = *(PULONG)((PUCHAR)local_image_base + rva + 1);
				ULONG highAddr = *(PULONG)((PUCHAR)local_image_base + rva + 9);
				originalAddr = ((ULONG_PTR)highAddr << 32) | lowAddr;
				break;
			}
			case KernelHookType::x64HookType3:
			{
				ULONG imm = *(PULONG)((PUCHAR)local_image_base + rva + 1);
				originalAddr = imm + address + 5;
				break;
			}
			default:
				break;
		}

		if (local_image_base != nullptr) {
			VirtualFree(local_image_base, 0, MEM_RELEASE);
		}

		if (originalAddr == targetAddress) {
			return false;
		}
	}

	return bHooked;
}










