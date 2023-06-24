#pragma once
// 因为是模板类，所以直接在这里包含
#include "Table.h"
#include "resource.h"
#include "DriverHelper.h"

class CProcessTable 
	:public CTable<std::shared_ptr<WinSys::ProcessInfo>>,
	public CWindowImpl<CProcessTable> {
public:
	DECLARE_WND_CLASS_EX(NULL,CS_DBLCLKS|CS_VREDRAW|CS_HREDRAW, COLOR_WINDOW);

	BEGIN_MSG_MAP(CProcessTable)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_CREATE,OnCreate)
		MESSAGE_HANDLER(WM_PAINT,OnPaint)
		MESSAGE_HANDLER(WM_HSCROLL,OnHScroll)
		MESSAGE_HANDLER(WM_VSCROLL,OnVScroll)
		MESSAGE_HANDLER(WM_USER_VABS,OnUserVabs)
		MESSAGE_HANDLER(WM_USER_VREL,OnUserVrel)
		MESSAGE_HANDLER(WM_USER_CHGS, OnUserChgs)
		MESSAGE_HANDLER(WM_TIMER,OnTimer)
		MESSAGE_HANDLER(WM_MOUSEWHEEL,OnMouseWheel)
		MESSAGE_HANDLER(WM_MOUSEMOVE,OnMouseMove)
		MESSAGE_HANDLER(WM_LBUTTONDOWN,OnLBtnDown)
		MESSAGE_HANDLER(WM_LBUTTONUP,OnLBtnUp)
		MESSAGE_HANDLER(WM_RBUTTONDOWN,OnRBtnDown)
		MESSAGE_HANDLER(WM_USER_STS,OnUserSts)
		MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged)
		MESSAGE_HANDLER(WM_KEYDOWN,OnKeyDown)
		MESSAGE_HANDLER(WM_SYSKEYDOWN, OnSysKeyDown)
		MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
		//MESSAGE_HANDLER(WM_SIZE,OnSize)
		COMMAND_ID_HANDLER(ID_PROCESS_KILL,OnProcessKill)
		COMMAND_ID_HANDLER(ID_PROCESS_REFRESH, OnProcessRefresh)
		COMMAND_ID_HANDLER(ID_PROCESS_MODULES,OnProcessModules)
		COMMAND_ID_HANDLER(ID_PROCESS_PROPERTIES,OnProcessProperties)
		COMMAND_ID_HANDLER(ID_PROCESS_GOTOFILELOCATION,OnProcessGoToFileLocation)
		COMMAND_ID_HANDLER(ID_PROCESS_THREADS,OnProcessThreads)
		COMMAND_ID_HANDLER(ID_PROCESS_HANDLES,OnProcessHandles)
		COMMAND_ID_HANDLER(ID_PROCESS_MEMORY,OnProcessMemory)
		COMMAND_ID_HANDLER(ID_PROCESS_INLINEHOOKSCAN,OnProcessInlineHookScan)
		COMMAND_ID_HANDLER(ID_ADDRESS_TABLE_HOOKSCAN,OnProcessEATHookScan)
		COMMAND_ID_HANDLER(ID_PROCESS_VAD_INFO,OnProcessVadInfo)
		COMMAND_ID_HANDLER(ID_PROCESS_DUMP,OnProcessDump)
	END_MSG_MAP()

	CProcessTable(BarInfo& bars,TableInfo& table);

	int ParseTableEntry(CString& s, char& mask, int& select, std::shared_ptr<WinSys::ProcessInfo>& info, int column);
	bool CompareItems(const std::shared_ptr<WinSys::ProcessInfo>& p1, const std::shared_ptr<WinSys::ProcessInfo>& p2, int col, bool asc=true);

	//LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnHScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnVScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnUserVabs(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnUserVrel(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnUserChgs(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
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

	LRESULT OnProcessKill(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnProcessRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnProcessModules(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnProcessProperties(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnProcessGoToFileLocation(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnProcessThreads(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnProcessHandles(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnProcessMemory(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnProcessInlineHookScan(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnProcessSuspend(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnProcessEATHookScan(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnProcessVadInfo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnProcessDump(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	bool InitVadSymbols(VadData* pData);


	int GetRowImage(HWND, size_t row) const;
	void Refresh();
	ProcessInfoEx& GetProcessInfoEx(WinSys::ProcessInfo* pi) const;

private:
	enum class ProcessColumn {
		Name,Id,Session,UserName,Eprocess,Priority,Threads,Handles,Attributes,CreateTime,Description,CompanyName,Version,ExePath,CmdLine
	};
	//std::vector<std::shared_ptr<WinSys::ProcessInfo>> m_Processes;
	mutable std::unordered_map<WinSys::ProcessInfo*, ProcessInfoEx> m_ProcessesEx;
	WinSys::ProcessManager m_ProcMgr;
	int m_UpdateInterval = 1000;
};