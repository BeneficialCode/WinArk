#pragma once
#include "Table.h"
#include "resource.h"
#include <KernelModuleTracker.h>

#define SystemModuleInformation 11

struct SystemServiceInfo {
	DWORD ServiceNumber;
	std::string ServiceFunctionName;
	uintptr_t OriginalAddress;
	std::string HookType;
	uintptr_t CurrentAddress;
	std::string TargetModule;
	bool Hooked;
};

class CSSDTHookTable :
	public CTable<SystemServiceInfo>,
	public CWindowImpl<CSSDTHookTable>{

public:
	DECLARE_WND_CLASS_EX(NULL, CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW, COLOR_WINDOW);

	CSSDTHookTable(BarInfo& bars, TableInfo& table);
	int ParseTableEntry(CString& s, char& mask, int& select, SystemServiceInfo& info, int column);
	bool CompareItems(const SystemServiceInfo& s1, const SystemServiceInfo& s2, int col, bool asc);

	BEGIN_MSG_MAP(CServiceTable)
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

	ULONG_PTR GetOrignalAddress(DWORD number);
	
	struct SSDTEntry {
		int Number;
		std::string Name;
		void* Original;
		void* Current;
	};

	void GetSSDTEntry();

private:
	enum class HookColumn {
		Number, Name, OrgAddress, HookType, CurAddress, TargetModule
	};

	PVOID _kernelBase;
	PVOID _fileMapVA;
	PULONG _KiServiceTable;
	ULONGLONG _imageBase;
	std::vector<SSDTEntry> _entries;
	int _total = 0;
};