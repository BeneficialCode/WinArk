#pragma once
#include "resource.h"
#include "HexEdit.h"
#include "ApiReader.h"

enum DisasmAddressType {
	ADDRESS_TYPE_MODULE,
	ADDRESS_TYPE_API,
	ADDRESS_TYPE_SPECIAL
};

class DisasmAddressComment {
public:
	DWORD_PTR _address;
	WCHAR _comment[512];
	DisasmAddressType _type;
	DWORD _moduleSize;

	bool operator<(const DisasmAddressComment& rhs) {
		return _address < rhs._address;
	}
};

class CDisasmDlg :public CDialogImpl<CDisasmDlg>,
	public CWinDataExchange<CDisasmDlg>, public CDialogResize<CDisasmDlg> {
public:
	enum {IDD = IDD_DISASSEMBLER};

	CDisasmDlg(DWORD_PTR startAddress, ApiReader* _pApiReader);

protected:
	static const size_t DISASM_SIZE = 0x1000;
	WCHAR _temp[512];
	int _addressHistoryIndex = 0;

	std::vector<DWORD_PTR> _addressHistories;
	std::vector<DisasmAddressComment> _addressComments;

	CListViewCtrl _ListDisassembler;
	CHexEdit _address;

	enum DisasmColumns {
		Address = 0,
		InstructionSize,
		Opcodes,
		Instruction,
		Comment
	};

	CMenu _hMenuDisasm;

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnContextMenu(CWindow wnd, CPoint point);
	void OnExit(UINT uNotifyCode, int nID, CWindow wndCtl);
	LRESULT OnNMCustomdraw(NMHDR* pnmh);
	void OnDisassemble(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnDisassembleBack(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnDisassembleForward(UINT uNotifyCode, int nID, CWindow wndCtl);

	void AddColumnsToDisassembler(CListViewCtrl& list);
	bool DisplayDisassembly();

	void CopyToClipboard(const WCHAR* pText);

private:
	ApiReader* _pApiReader = nullptr;
	BYTE _data[DISASM_SIZE];

	void ToUpperCase(WCHAR* pLowercase);
	void DoColorInstruction(LPNMLVCUSTOMDRAW lpLVCustomDraw, DWORD_PTR itemIndex);
	void FollowInstruction(int index);
	bool GetDisassemblyComment(unsigned int index);

	void DisassembleNewAddress(DWORD_PTR address);
	void InitAddressCommentList();
	void AddModuleAddressCommentEntry(DWORD_PTR address, DWORD moduleSize, const WCHAR* pModulePath);
	void AnalyzeAddress(DWORD_PTR address, WCHAR* pComment);

public:
	BEGIN_DDX_MAP(CDisasmDlg)
		DDX_CONTROL_HANDLE(IDC_LIST_DISASSEMBLER,_ListDisassembler)
		DDX_CONTROL(IDC_DISASM_ADDRESS,_address)
	END_DDX_MAP()

	BEGIN_MSG_MAP(CDisasmDlg)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_CONTEXTMENU(OnContextMenu)

		NOTIFY_HANDLER_EX(IDC_LIST_DISASSEMBLER,NM_CUSTOMDRAW,OnNMCustomdraw)

		COMMAND_ID_HANDLER_EX(IDC_DISASM,OnDisassemble)
		COMMAND_ID_HANDLER_EX(IDC_DISASM_BACK,OnDisassembleBack)
		COMMAND_ID_HANDLER_EX(IDC_DISASM_FORWARD,OnDisassembleForward)
		COMMAND_ID_HANDLER_EX(IDCANCEL,OnExit)
		COMMAND_ID_HANDLER_EX(IDOK,OnExit)
		CHAIN_MSG_MAP(CDialogResize<CDisasmDlg>)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CDisasmDlg)
		DLGRESIZE_CONTROL(IDC_LIST_DISASSEMBLER, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_DISASM, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_DISASM_BACK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_DISASM_FORWARD, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_DISASM_ADDRESS, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_DISASSEMBLE_ADDRESS, DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()
};