#pragma once
#include "Table.h"
#include "resource.h"
#include <ProcessVMTracker.h>
#include <ProcessModuleTracker.h>
#include <capstone/capstone.h>

enum class HookType {
	x64HookType1,x64HookType2,x64HookType3,x64HookType4
};

struct InlineHookInfo {
	std::wstring Name;
	HookType Type;
	ULONG_PTR Address;
	ULONG_PTR TargetAddress;
	std::wstring TargetModule;
};

class CProcessInlineHookTable :
	public CTable<InlineHookInfo>,
	public CWindowImpl<CProcessInlineHookTable> {

public:
	DECLARE_WND_CLASS_EX(NULL, CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW, COLOR_WINDOW);

	CProcessInlineHookTable(BarInfo& bars, TableInfo& table,DWORD pid);
	int ParseTableEntry(CString& s, char& mask, int& select, InlineHookInfo& info, int column);
	bool CompareItems(const InlineHookInfo& s1, const InlineHookInfo& s2, int col, bool asc);

	BEGIN_MSG_MAP(CProcessInlineHookTable)
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
		MESSAGE_HANDLER(WM_USER_STS, OnUserSts)
		MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		MESSAGE_HANDLER(WM_SYSKEYDOWN, OnSysKeyDown)
	END_MSG_MAP()

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

	void Refresh();

private:

	const std::vector<std::shared_ptr<WinSys::MemoryRegionItem>>& GetItems();
	void CheckX64HookType1(cs_insn* insn, size_t j, size_t count, ULONG_PTR moduleBase, size_t moduleSize,
		ULONG_PTR base, size_t size);
	void CheckX64HookType2(cs_insn* insn, size_t j, size_t count);
	void CheckX64HookType4(cs_insn* insn, size_t j, size_t count, ULONG_PTR moduleBase, size_t moduleSize,
		ULONG_PTR base, size_t size);

	std::shared_ptr<WinSys::ModuleInfo> GetModuleByAddress(ULONG_PTR address);
	void CheckInlineHook(uint8_t* code, size_t codeSize, uint64_t address, ULONG_PTR moduleBase, SIZE_T moduleSize);

	CString TypeToString(HookType type);

	enum class Column {
		HookObject,HookType,Address,TargetAddress,Module
	};

private:
	DWORD m_Pid;
	HANDLE m_hProcess{ INVALID_HANDLE_VALUE };
	std::unique_ptr<WinSys::ProcessVMTracker> m_VMTracker{ nullptr };
	std::vector<std::shared_ptr<WinSys::MemoryRegionItem>> m_Items;
	std::vector<std::shared_ptr<WinSys::ModuleInfo>> m_Modules;
	std::vector<std::shared_ptr<WinSys::ModuleInfo>> m_Sys64Modules;
	WinSys::ProcessModuleTracker m_ModuleTracker;
	csh _x64handle;
};