#pragma once

#include "IHexControl.h"
#include "EditCommandBase.h"

class CHexControl :
	public CWindowImpl<CHexControl>,
	public IHexControl {
public:
	//	enum { uSCROLL_FLAGS = SW_INVALIDATE };

	DECLARE_WND_CLASS_EX(_T("WTLHexControl"), CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, NULL)

	CHexControl();

	void DoPaint(CDC& dc, RECT& rc);

	BEGIN_MSG_MAP(CHexControl)
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CHAR, OnChar)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
		MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
		MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_VSCROLL, OnVScroll)
		MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
		MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
		MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDialogCode)
		MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
	END_MSG_MAP()

	// message handlers
	LRESULT OnGetDialogCode(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnChar(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnMouseWheel(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnHScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnVScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	// Inherited via IHexControl
	void SetBufferManager(IBufferManager * mgr) override;
	void SetReadOnly(bool readonly) override;
	bool IsReadOnly() const override;
	void SetAllowExtension(bool allow) override;
	bool IsAllowExtension() const override;
	bool CanUndo() const override;
	bool CanRedo() const override;
	bool Undo() override;
	bool Redo() override;
	void SetSize(int64_t size) override;
	bool SetDataSize(int32_t size) override;
	int32_t GetDataSize() const override;
	bool SetBytesPerLine(int32_t bytesPerLine) override;
	int32_t GetBytesPerLine() const override;
	void SetOffsetColor(COLORREF color = CLR_INVALID);
	void SetDataColor(COLORREF color = CLR_INVALID);
	void SetAsciiColor(COLORREF color = CLR_INVALID);
	void SetBackColor(COLORREF color = CLR_INVALID);
	void Copy(int64_t offset = -1, int32_t size = 0);
	void Paste(int64_t offset = -1);
	COLORREF GetOffsetColor() const;
	COLORREF GetDataColor() const;
	COLORREF GetAsciiColor() const;
	COLORREF GetBackColor() const;

protected:
	void RecalcLayout();
	int64_t GetOffsetFromPoint(const POINT& pt);
	POINT GetPositionFromOffset(int64_t offset);
	virtual void Init();
	void DrawNumber(int64_t offset, uint64_t value, int digitsChanged);
	void CommitValue(int64_t offset, uint64_t value);
	void ResetInput();
	void UpdateData(EditCommandBase* cmd);
	void DrawOffset(int64_t offset);
	void DrawAsciiChar(int64_t offset, uint8_t ch);

	class CommandManager {
	public:
		bool CanUndo() const;
		bool CanRedo() const;

		bool AddCommand(std::shared_ptr<EditCommandBase> command, bool execute = true);
		bool Undo();
		bool Redo();
		void Clear();

		EditCommandBase* GetUndoCommand() const;
		EditCommandBase* GetRedoCommand() const;

	private:
		std::vector<std::shared_ptr<EditCommandBase>> _undoList;
		std::vector<std::shared_ptr<EditCommandBase>> _redoList;
	};

private:
	CommandManager m_CmdMgr;
	CFont m_Font;
	int m_FontPointSize{ 110 };
	CRect m_rcAddress, m_rcData, m_rcAscii;
	COLORREF m_clrAddress{ CLR_INVALID }, m_clrData{ CLR_INVALID }, m_clrAscii{ CLR_INVALID };
	COLORREF m_clrBack{ CLR_INVALID };
	int64_t m_SelStart{ -1 }, m_SelLength;
	int64_t m_StartOffset{ 0 }, m_EndOffset, m_CurrentOffset{ -1 };
	int32_t m_BytesPerLine{ 32 }, m_DataSize{ 1 };
	uint64_t m_OldValue;
	int m_Width, m_Height;
	IBufferManager* m_Buffer{ nullptr };
	int m_CharHeight, m_CharWidth;
	CSize m_Margin{ 2, 2 };
	int m_EditDigits{ 0 };
	uint64_t m_CurrentInput{ 0 };
	bool m_ReadOnly{ false };
	bool m_AllowExtension{ true };
	bool m_InsertMode{ false };
};

