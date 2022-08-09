#include "stdafx.h"
#include "KernelNotifyTable.h"
#include "DriverHelper.h"
#include <SymbolManager.h>
#include "Helpers.h"
#include <filesystem>
#include "SymbolHelper.h"

CKernelNotifyTable::CKernelNotifyTable(BarInfo& bars, TableInfo& table)
	:CTable(bars, table) {
	SetTableWindowInfo(bars.nbar);
	Refresh();
}

LRESULT CKernelNotifyTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return 0;
}

LRESULT CKernelNotifyTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	
	return 0;
}
LRESULT CKernelNotifyTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT CKernelNotifyTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelNotifyTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelNotifyTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelNotifyTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelNotifyTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT CKernelNotifyTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}
LRESULT CKernelNotifyTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelNotifyTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelNotifyTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelNotifyTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	CMenu menu;
	CMenuHandle hSubMenu;
	menu.LoadMenu(IDR_CONTEXT);
	hSubMenu = menu.GetSubMenu(8);
	POINT pt;
	::GetCursorPos(&pt);
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);

	bool show = Tablefunction(m_hWnd, uMsg, wParam, lParam);
	if (show) {
		auto id = (UINT)TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, nullptr);
		if (id) {
			PostMessage(WM_COMMAND, id);
		}
	}
	return 0;
}
LRESULT CKernelNotifyTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelNotifyTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelNotifyTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT CKernelNotifyTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

int CKernelNotifyTable::ParseTableEntry(CString& s, char& mask, int& select, CallbackInfo& info, int column) {
	// 回调函数地址 回调类型 所在模块 文件厂商
	switch (column)
	{
		case 0:
			s.Format(L"0x%p", info.Routine);
			break;
		case 1:
		{
			switch (info.Type)
			{		
				case CallbackType::CreateProcessNotify:
					s = L"CreateProcess";
					break;
				case CallbackType::CreateThreadNotify:
					s = L"CreateThread";
					break;

				case CallbackType::LoadImageNotify:
					s = L"LoadImage";
					break;

				case CallbackType::ProcessObPostOperationNotify:
					s = L"Process ObPostOperation";
					break;

				case CallbackType::ProcessObPreOperationNotify:
					s = L"Process ObPreOperation";
					break;

				case CallbackType::ThreadObPostOperationNotify:
					s = L"Thread ObPostOperation";
					break;

				case CallbackType::ThreadObPreOperationNotify:
					s = L"Thread ObPreOperation";
					break;

				case CallbackType::RegistryNotify:
					s = L"Registry Notify";
					break;
			}
			break;
		}
		case 2:
			s = info.Company.c_str();
			break;
		case 3:
			s = Helpers::StringToWstring(info.Module).c_str();
			break;
	}

	return s.GetLength();
}

bool CKernelNotifyTable::CompareItems(const CallbackInfo& s1, const CallbackInfo& s2, int col, bool asc) {

	return false;
}

void CKernelNotifyTable::Refresh() {
	auto& helper = SymbolHelper::Get();
#ifdef _WIN64
	static ULONG Max = 64;
#else
	static ULONG Max = 8;
#endif

	ULONG count;
	ProcessNotifyCountData data;

	data.pCount = (PULONG)helper.GetKernelSymbolAddressFromName("PspCreateProcessNotifyRoutineCount");
	data.pExCount = (PULONG)helper.GetKernelSymbolAddressFromName("PspCreateProcessNotifyRoutineExCount");
	count = DriverHelper::GetProcessNotifyCount(&data);
	NotifyInfo info;
	info.Count = count;
	info.pRoutine = (void*)helper.GetKernelSymbolAddressFromName("PspCreateProcessNotifyRoutine");

	m_Table.data.n = 0;
	m_Table.data.info.clear();

	// Enum CreateProcessNotify
	if (count > 0) {
		SIZE_T size = Max * sizeof(void*) + sizeof(ULONG);
		wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_READWRITE));

		KernelCallbackInfo* p = (KernelCallbackInfo*)buffer.get();
		if (p != nullptr) {
			p->Count = count + 1;
			DriverHelper::EnumProcessNotify(&info, p);
			for (int i = 0; i < count; i++) {
				CallbackInfo info;
				info.Routine = p->Address[i];
				info.Type = CallbackType::CreateProcessNotify;
				info.Module = Helpers::GetKernelModuleByAddress((ULONG_PTR)info.Routine);
				std::wstring path = Helpers::StringToWstring(info.Module);
				info.Company = GetCompanyName(path);
				m_Table.data.info.push_back(std::move(info));
			}
		}
	}
	
	ULONG offset = helper.GetKernelStructMemberOffset("_OBJECT_TYPE", "CallbackList");
	KernelNotifyInfo notifyInfo;
	notifyInfo.Type = NotifyType::ProcessObjectNotify;
	notifyInfo.Offset = offset;
	count = DriverHelper::GetObCallbackCount(&notifyInfo);
	if (count > 0) {
		SIZE_T size = Max * sizeof(ObCallbackInfo);
		wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_READWRITE));

		ObCallbackInfo* p = (ObCallbackInfo*)buffer.get();
		if (p != nullptr) {
			DriverHelper::EnumObCallbackNotify(&notifyInfo, p, size);
			for (int i = 0; i < count; i++) {
				if (p[i].PostOperation) {
					CallbackInfo info;
					info.Routine = p[i].PostOperation;
					info.Type = CallbackType::ProcessObPostOperationNotify;
					info.Module = Helpers::GetKernelModuleByAddress((ULONG_PTR)info.Routine);
					std::wstring path = Helpers::StringToWstring(info.Module);
					info.Company = GetCompanyName(path);
					info.Address = p[i].RegistrationHandle;
					m_Table.data.info.push_back(std::move(info));
				}
				if (p[i].PreOperation) {
					CallbackInfo info;
					info.Routine = p[i].PreOperation;
					info.Type = CallbackType::ProcessObPreOperationNotify;
					info.Module = Helpers::GetKernelModuleByAddress((ULONG_PTR)info.Routine);
					std::wstring path = Helpers::StringToWstring(info.Module);
					info.Company = GetCompanyName(path);
					info.Address = p[i].RegistrationHandle;
					m_Table.data.info.push_back(std::move(info));
				}
			}
		}
	}


	notifyInfo.Type = NotifyType::ThreadObjectNotify;
	count = DriverHelper::GetObCallbackCount(&notifyInfo);
	if (count > 0) {
		SIZE_T size = Max * sizeof(ObCallbackInfo);
		wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_READWRITE));
		ObCallbackInfo* p = (ObCallbackInfo*)buffer.get();
		if (p != nullptr) {
			DriverHelper::EnumObCallbackNotify(&notifyInfo, p, size);
			for (int i = 0; i < count; i++) {
				if (p[i].PostOperation) {
					CallbackInfo info;
					info.Routine = p[i].PostOperation;
					info.Type = CallbackType::ThreadObPostOperationNotify;
					info.Module = Helpers::GetKernelModuleByAddress((ULONG_PTR)info.Routine);
					std::wstring path = Helpers::StringToWstring(info.Module);
					info.Company = GetCompanyName(path);
					info.Address = p[i].RegistrationHandle;
					m_Table.data.info.push_back(std::move(info));
				}
				if (p[i].PreOperation) {
					CallbackInfo info;
					info.Routine = p[i].PreOperation;
					info.Type = CallbackType::ThreadObPreOperationNotify;
					info.Module = Helpers::GetKernelModuleByAddress((ULONG_PTR)info.Routine);
					std::wstring path = Helpers::StringToWstring(info.Module);
					info.Company = GetCompanyName(path);
					info.Address = p[i].RegistrationHandle;
					m_Table.data.info.push_back(std::move(info));
				}
			}
		}

	}

	count = 0;
	ThreadNotifyCountData threadData;
	threadData.pCount = 0;
	threadData.pNonSystemCount = 0;
	threadData.pCount = (PULONG)helper.GetKernelSymbolAddressFromName("PspCreateThreadNotifyRoutineCount");
	threadData.pNonSystemCount = (PULONG)helper.GetKernelSymbolAddressFromName("PspCreateThreadNotifyRoutineNonSystemCount");

	count = DriverHelper::GetThreadNotifyCount(&threadData);
	if (count > 0) {
		info.Count = count;
		info.pRoutine = (void*)helper.GetKernelSymbolAddressFromName("PspCreateThreadNotifyRoutine");
		SIZE_T size = Max * sizeof(void*) + sizeof(ULONG);
		wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_READWRITE));
		KernelCallbackInfo* p = (KernelCallbackInfo*)buffer.get();
		if (p != nullptr) {
			p->Count = count + 1;
			DriverHelper::EnumThreadNotify(&info, p);
			for (int i = 0; i < count; i++) {
				CallbackInfo item;
				item.Routine = p->Address[i];
				item.Type = CallbackType::CreateThreadNotify;
				item.Module = Helpers::GetKernelModuleByAddress((ULONG_PTR)item.Routine);
				std::wstring path = Helpers::StringToWstring(item.Module);
				item.Company = GetCompanyName(path);
				m_Table.data.info.push_back(std::move(item));
			}
		}
	}

	count = 0;
	PULONG pCount = (PULONG)helper.GetKernelSymbolAddressFromName("PspLoadImageNotifyRoutineCount");

	count = DriverHelper::GetImageNotifyCount(&pCount);
	if (count > 0) {
		info.Count = count;
		info.pRoutine = (void*)helper.GetKernelSymbolAddressFromName("PspLoadImageNotifyRoutine");

		SIZE_T size = Max * sizeof(void*) + sizeof(ULONG);
		wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_READWRITE));
		KernelCallbackInfo* p = (KernelCallbackInfo*)buffer.get();
		if (p != nullptr) {
			p->Count = count + 1;
			DriverHelper::EnumImageLoadNotify(&info, p);
			for (int i = 0; i < count; i++) {
				CallbackInfo info;
				info.Routine = p->Address[i];
				info.Type = CallbackType::LoadImageNotify;
				info.Module = Helpers::GetKernelModuleByAddress((ULONG_PTR)info.Routine);
				std::wstring path = Helpers::StringToWstring(info.Module);
				info.Company = GetCompanyName(path);
				m_Table.data.info.push_back(std::move(info));
			}
		}
	}

	pCount = (PULONG)helper.GetKernelSymbolAddressFromName("CmpCallbackCount");
	count = DriverHelper::GetCmCallbackCount(&pCount);
	if (count > 0) {
		SIZE_T size = Max * sizeof(CmCallbackInfo);
		wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_READWRITE));
		CmCallbackInfo* p = (CmCallbackInfo*)buffer.get();
		if (p != nullptr) {
			PVOID callbackListHead = (void*)helper.GetKernelSymbolAddressFromName("CallbackListHead");

			DriverHelper::EnumCmCallbackNotify(callbackListHead, p, size);
			/*symbol = handler.GetSymbolFromName("CmpPreloadedHivesList");
			PVOID pCmpPreloadedHivesList = (PVOID)symbol->GetSymbolInfo()->Address;*/
			for (int i = 0; i < count; ++i) {
				CallbackInfo info;
				info.Routine = p[i].Address;
				info.Module = Helpers::GetKernelModuleByAddress((ULONG_PTR)info.Routine);
				info.Type = CallbackType::RegistryNotify;
				std::wstring path = Helpers::StringToWstring(info.Module);
				info.Company = GetCompanyName(path);
				info.Cookie = p[i].Cookie;
				m_Table.data.info.push_back(std::move(info));
			}
		}
	}

	m_Table.data.n = m_Table.data.info.size();
}

std::wstring CKernelNotifyTable::GetCompanyName(std::wstring path) { 
	BYTE buffer[1 << 12];
	WCHAR* companyName = nullptr;
	CString filePath = path.c_str();
	if (filePath.Mid(1, 2) == "??")
		filePath = filePath.Mid(4);
	if (::GetFileVersionInfo(filePath, 0, sizeof(buffer), buffer)) {
		WORD* langAndCodePage;
		UINT len;
		if (::VerQueryValue(buffer, L"\\VarFileInfo\\Translation", (void**)&langAndCodePage, &len)) {
			CString text;
			text.Format(L"\\StringFileInfo\\%04x%04x\\CompanyName", langAndCodePage[0], langAndCodePage[1]);
			
			if (::VerQueryValue(buffer, text, (void**)&companyName, &len))
				return companyName;
		}
	}
	return L"";
}

LRESULT CKernelNotifyTable::OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Refresh();
	Invalidate(True);
	return TRUE;
}

LRESULT CKernelNotifyTable::OnRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int selected = m_Table.data.selected;
	ATLASSERT(selected >= 0);
	auto& p = m_Table.data.info[selected];

	CString text;
	text.Format(L"移除回调：%p?",p.Routine);
	if (AtlMessageBox(*this, (PCWSTR)text, IDS_TITLE, MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON2) == IDCANCEL)
		return 0;

	NotifyData data;
	data.Address = p.Routine;

	switch (p.Type)
	{
		case CallbackType::CreateProcessNotify:
			data.Type = NotifyType::CreateProcessNotify;
			break;

		case CallbackType::CreateThreadNotify:
			data.Type = NotifyType::CreateThreadNotify;
			break;

		case CallbackType::LoadImageNotify:
			data.Type = NotifyType::LoadImageNotify;
			break;

		case CallbackType::ProcessObPostOperationNotify:
		case CallbackType::ProcessObPreOperationNotify:
			data.Type = NotifyType::ProcessObjectNotify;
			data.Address = p.Address;
			break;
		case CallbackType::ThreadObPostOperationNotify:
		case CallbackType::ThreadObPreOperationNotify:
			data.Type = NotifyType::ThreadObjectNotify;
			data.Address = p.Address;
			break;

		case CallbackType::RegistryNotify:
			data.Type = NotifyType::RegistryNotify;
			data.Cookie = p.Cookie;
			break;
		default:
			break;
	}


	BOOL ok = false;

	ok = DriverHelper::RemoveNotify(&data);
	if (!ok)
		AtlMessageBox(*this, L"Failed to remove notify", IDS_TITLE, MB_ICONERROR);
	else
		Refresh();
	Invalidate(True);
	return TRUE;
}