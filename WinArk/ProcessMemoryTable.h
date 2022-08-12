#pragma once

#include "Table.h"
#include "resource.h"
#include "ProcessVMTracker.h"
#include <ThreadInfo.h>
#include <ProcessManager.h>

class CProcessMemoryTable :
	public CTable<std::shared_ptr<WinSys::MemoryRegionItem>>,
	public CWindowImpl<CProcessMemoryTable> {
public:
	DECLARE_WND_CLASS_EX(NULL, CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW, COLOR_WINDOW);

	BEGIN_MSG_MAP(CProcessMemoryTable)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
		MESSAGE_HANDLER(WM_VSCROLL, OnVScroll)
		MESSAGE_HANDLER(WM_USER_VABS, OnUserVabs)
		MESSAGE_HANDLER(WM_USER_VREL, OnUserVrel)
		MESSAGE_HANDLER(WM_USER_CHGS, OnUserChgs)
		MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLBtnDown)
		MESSAGE_HANDLER(WM_LBUTTONUP, OnLBtnUp)
		MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRBtnDown)
		MESSAGE_HANDLER(WM_USER_STS, OnUserSts)
		MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		MESSAGE_HANDLER(WM_SYSKEYDOWN, OnSysKeyDown)
		MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
		COMMAND_ID_HANDLER(ID_MEMORY_DUMP,OnMemoryDump)
	END_MSG_MAP()

	CProcessMemoryTable(BarInfo& bars, TableInfo& table, DWORD pid);

	int ParseTableEntry(CString& s, char& mask, int& select, std::shared_ptr<WinSys::MemoryRegionItem>& info, int column);

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/);
	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnGetDlgCode(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT OnMemoryDump(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	
	void Refresh();
private:
	enum class MemoryUsage {
		PrivateData,
		Heap,
		Image,
		Mapped,
		ThreadStack,
		Unusable,
		Unknown = 99
	};

	struct ItemDetails {
		CString Details;
		MemoryUsage Usage;
	};

	struct HeapInfo {
		DWORD_PTR Id;
		DWORD_PTR Address;
		DWORD_PTR Size;
		DWORD Flags;
	};

	static PCWSTR StateToString(DWORD state);
	static CString ProtectionToString(DWORD state);
	static PCWSTR TypeToString(DWORD type);
	static CString HeapFlagsToString(DWORD flags);

	//bool CompareItems(WinSys::MemoryRegionItem& m1, WinSys::MemoryRegionItem& m2, int col, bool asc);
	ItemDetails GetDetails(const std::shared_ptr<WinSys::MemoryRegionItem>& mi) const;
	PCWSTR UsageToString(const std::shared_ptr<WinSys::MemoryRegionItem>& item) const;
	static CString FormatWithCommas(long long size);

	DWORD m_Pid;
	std::vector<std::shared_ptr<WinSys::ThreadInfo>> m_Threads;
	mutable std::unordered_map<PVOID, ItemDetails> m_Details;
	std::vector<HeapInfo> m_Heaps;
	HANDLE m_hProcess;
	wil::unique_handle m_hReadProcess;
	std::unique_ptr<WinSys::ProcessVMTracker> m_Tracker;
	WinSys::ProcessManager m_ProcMgr;
};