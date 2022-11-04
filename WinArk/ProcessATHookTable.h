#pragma once
#include "Table.h"
#include <ProcessModuleTracker.h>
#include <PEParser.h>
#include <ApiSets.h>

enum class ATHookType {
	IAT, EAT
};

struct EATHookInfo {
	std::wstring Name;
	ULONG_PTR Address;
	ULONG_PTR TargetAddress;
	std::wstring TargetModule;
	ATHookType Type;
};



class CProcessATHookTable :
	public CTable<EATHookInfo>,
	public CWindowImpl<CProcessATHookTable> {
public:
	DECLARE_WND_CLASS_EX(NULL, CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW, COLOR_WINDOW);

	CProcessATHookTable(BarInfo& bars, TableInfo& table, DWORD pid, bool x64);
	int ParseTableEntry(CString& s, char& mask, int& select, EATHookInfo& info, int column);
	bool CompareItems(const EATHookInfo& s1, const EATHookInfo& s2, int col, bool asc);

	BEGIN_MSG_MAP(CProcessATHookTable)
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

	enum class Column {
		HookObject, HookType, Address, TargetAddress, Module
	};
	std::shared_ptr<WinSys::ModuleInfo> GetModuleByAddress(ULONG_PTR address);
	CString TypeToString(ATHookType type);
	void CheckEATHook(const std::shared_ptr<WinSys::ModuleInfo>& m);
	void CheckIATHook(const std::shared_ptr<WinSys::ModuleInfo>& m);
	std::shared_ptr<WinSys::ModuleInfo> GetModuleByName(std::wstring name);

	std::vector<ULONG_PTR> GetExportedProcAddr(std::wstring libName, std::string name, bool isPe64);

	std::string GetForwardName(std::wstring libName, std::string name, bool isPe64);
	
	std::vector<std::wstring> GetApiSetHostName(std::wstring apiset);

	struct Library {
		std::wstring Name;
		void* Base;
		bool isPe64;
		std::vector<ExportedSymbol> Symbols;
	};

private:
	WinSys::ProcessModuleTracker m_ModuleTracker;
	DWORD m_Pid;
	HANDLE m_hProcess;
	std::vector<std::shared_ptr<WinSys::ModuleInfo>> m_Modules;
	std::vector<Library> _libraries;
	ApiSets m_ApiSets;
	std::vector<ApiSetEntry> m_Entries;
};
