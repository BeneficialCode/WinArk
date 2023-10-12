#pragma once
#include "Table.h"
#include "resource.h"
#include "DriverHelper.h"


struct ObjectCallbackInfo {
	void* RegistrationHandle;
	void* CallbackEntry;
	ObjectCallbackType Type;
	bool Enabled;
	void* PreOperation;
	void* PostOperation;
	ULONG Operations;
	std::wstring Company;
	std::string Module;
};

class CObjectCallbackTable :
	public CTable<ObjectCallbackInfo>,
	public CWindowImpl<CObjectCallbackTable> {
public:
	DECLARE_WND_CLASS_EX(NULL, CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW, COLOR_WINDOW);

	CObjectCallbackTable(BarInfo& bars, TableInfo& table);
	int ParseTableEntry(CString& s, char& mask, int& select, ObjectCallbackInfo& info, int column);
	bool CompareItems(const ObjectCallbackInfo& s1, const ObjectCallbackInfo& s2, int col, bool asc);

	BEGIN_MSG_MAP(CKernelNotifyTable)
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
		COMMAND_ID_HANDLER(ID_OB_CALLBACK_REFRESH, OnRefresh)
		COMMAND_ID_HANDLER(ID_OB_CALLBACK_REMOVE, OnRemove)
		COMMAND_ID_HANDLER(ID_OB_CALLBACK_ENABLE,OnEnable)
		COMMAND_ID_HANDLER(ID_OB_CALLBACK_DISABLE,OnDisable)
		COMMAND_ID_HANDLER(ID_OB_CALLBACK_COPY, OnCopy)
		COMMAND_ID_HANDLER(ID_OB_CALLBACK_EXPORT, OnExport)
		COMMAND_ID_HANDLER(ID_OB_CALLBACK_REMOVE_BY_NAME,OnRemoveByCompanyName)
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
	LRESULT OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDisable(WORD, WORD, HWND, BOOL&);
	LRESULT OnEnable(WORD, WORD, HWND, BOOL&);
	LRESULT OnRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRemoveByCompanyName(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCopy(WORD, WORD, HWND, BOOL&);
	LRESULT OnExport(WORD, WORD, HWND, BOOL&);

	enum class ObCallbackColumn {
		CallbackEntry,RegisterarionHandle,Type,Enabled,PreOperation,PostOperation,Opeartions,Company,ModuleName
	};

	PCWSTR TypeToString(ObjectCallbackType type);
	CString DecodeOperations(ULONG operations);
private:
	void Refresh();

	std::wstring GetSingleInfo(ObjectCallbackInfo& info);
};