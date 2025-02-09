#include "stdafx.h"
#include "DisasmDlg.h"
#include "ProcessAccessHelper.h"
#include "Architecture.h"
#include <Psapi.h>


CDisasmDlg::CDisasmDlg(DWORD_PTR startAddress, ApiReader* pApiReader) {
	_addressHistories.push_back(startAddress);
	_hMenuDisasm.LoadMenu(IDR_DISASM);
	InitAddressCommentList();
}

BOOL CDisasmDlg::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	DoDataExchange();
	DlgResize_Init(true, true);

	AddColumnsToDisassembler(_ListDisassembler);
	DisplayDisassembly();

	_address.SetValue(_addressHistories[_addressHistoryIndex]);

	CenterWindow();

	return TRUE;
}

void CDisasmDlg::OnContextMenu(CWindow wnd, CPoint point)
{
	if (wnd.GetDlgCtrlID() == IDC_LIST_DISASSEMBLER) {
		int selection = _ListDisassembler.GetSelectionMark();
		if (selection == -1)
			return;

		if (point.x == -1 && point.y == -1) {
			_ListDisassembler.EnsureVisible(selection, TRUE);
			_ListDisassembler.GetItemPosition(selection, &point);
			_ListDisassembler.ClientToScreen(&point);
		}

		CMenuHandle hSub = _hMenuDisasm.GetSubMenu(0);
		BOOL menuItem = hSub.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, point.x, point.y, wnd);
		if (menuItem) {
			int column = -1;
			switch (menuItem)
			{
				case ID_DISASM_COPY_ADDRESS:
					column = Address;
					break;
				case ID_DISASM_COPY_OPCODES:
					column = Opcodes;
					break;
				case ID_DISASM_COPY_INSTRUCTIONS:
					column = Instruction;
					break;
				case ID_DISASM_FOLLOW:
					FollowInstruction(selection);
					break;
				case ID_DISASM_DISASSEMBLE_HERE:
					DisassembleNewAddress(ProcessAccessHelper::_insts[selection].addr);
					break;
				default:
					break;
			}
			if (column != -1) {
				_temp[0] = '\0';
				_ListDisassembler.GetItemText(selection, column, _temp, _countof(_temp));
				CopyToClipboard(_temp);
			}
		}
	}
}

LRESULT CDisasmDlg::OnNMCustomdraw(NMHDR* pnmh) {
	LRESULT result = 0;

	LPNMLVCUSTOMDRAW lpLVCustomDraw = (LPNMLVCUSTOMDRAW)pnmh;
	DWORD_PTR itemIndex = 0;

	switch (lpLVCustomDraw->nmcd.dwDrawStage)
	{
		case CDDS_ITEMPREPAINT:
		case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
		{
			itemIndex = lpLVCustomDraw->nmcd.dwItemSpec;
			if (lpLVCustomDraw->iSubItem == Instruction) {
				DoColorInstruction(lpLVCustomDraw, itemIndex);
			}
			else {
				lpLVCustomDraw->clrText = CLR_DEFAULT;
				lpLVCustomDraw->clrTextBk = CLR_DEFAULT;
			}
			break;
		}
		default:
			break;
	}

	result |= CDRF_NOTIFYPOSTPAINT;
	result |= CDRF_NOTIFYITEMDRAW;
	result |= CDRF_NOTIFYSUBITEMDRAW;

	return result;

}

void CDisasmDlg::OnExit(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	EndDialog(0);
}

void CDisasmDlg::OnDisassemble(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	DWORD_PTR address = _address.GetValue();
	if (address) {
		DisassembleNewAddress(address);
	}
}

void CDisasmDlg::OnDisassembleBack(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	if (_addressHistoryIndex != 0) {
		_addressHistoryIndex--;
		_address.SetValue(_addressHistories[_addressHistoryIndex]);
		if (!DisplayDisassembly()) {
			MessageBox(L"Cannot disassemble memory at this address", L"Error", MB_ICONERROR);
		}
	}
}

void CDisasmDlg::OnDisassembleForward(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	if (_addressHistoryIndex != (_addressHistories.size() - 1))
	{
		_addressHistoryIndex++;
		_address.SetValue(_addressHistories[_addressHistoryIndex]);
		if (!DisplayDisassembly())
		{
			MessageBox(L"Cannot disassemble memory at this address", L"Error", MB_ICONERROR);
		}
	}
}

void CDisasmDlg::AddColumnsToDisassembler(CListViewCtrl& list)
{
	list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	list.InsertColumn(Address, L"Address", LVCFMT_LEFT);
	list.InsertColumn(InstructionSize, L"Size", LVCFMT_CENTER);
	list.InsertColumn(Opcodes, L"Opcodes", LVCFMT_LEFT);
	list.InsertColumn(Instruction, L"Instructions", LVCFMT_LEFT);
	list.InsertColumn(Comment, L"Comment", LVCFMT_LEFT);
}

bool CDisasmDlg::DisplayDisassembly()
{
	_ListDisassembler.DeleteAllItems();
	if (!ProcessAccessHelper::ReadMemoryFromProcess(_addressHistories[_addressHistoryIndex],
		sizeof(_data), _data)) {
		return false;
	}
	if (!ProcessAccessHelper::DecomposeMemory(_data, sizeof(_data), _addressHistories[_addressHistoryIndex])) {
		return false;
	}
	if (!ProcessAccessHelper::DisassembleMemory(_data, sizeof(_data), _addressHistories[_addressHistoryIndex])) {
		return false;
	}

	for (unsigned int i = 0; i < ProcessAccessHelper::_instCount; i++) {
		swprintf_s(_temp, PRINTF_DWORD_PTR_FULL, ProcessAccessHelper::_decodedInsts[i].offset);
		_ListDisassembler.InsertItem(i, _temp);

		swprintf_s(_temp, L"%02d", ProcessAccessHelper::_decodedInsts[i].size);

		_ListDisassembler.SetItemText(i, InstructionSize, _temp);

		swprintf_s(_temp, L"%S", (char*)ProcessAccessHelper::_decodedInsts[i].instructionHex.p);

		ToUpperCase(_temp);
		_ListDisassembler.SetItemText(i, Opcodes, _temp);

		swprintf_s(_temp, L"%S%S%S", (char*)ProcessAccessHelper::_decodedInsts[i].mnemonic.p,
			ProcessAccessHelper::_decodedInsts[i].operands.length != 0 ? " " : "",
			(char*)ProcessAccessHelper::_decodedInsts[i].operands.p);

		ToUpperCase(_temp);
		_ListDisassembler.SetItemText(i, Instruction, _temp);

		_temp[0] = 0;
		if (GetDisassemblyComment(i))
		{
			_ListDisassembler.SetItemText(i, Comment, _temp);
		}
	}

	_ListDisassembler.SetColumnWidth(Address, LVSCW_AUTOSIZE_USEHEADER);
	_ListDisassembler.SetColumnWidth(InstructionSize, LVSCW_AUTOSIZE_USEHEADER);
	_ListDisassembler.SetColumnWidth(Opcodes, 140);
	_ListDisassembler.SetColumnWidth(Instruction, LVSCW_AUTOSIZE_USEHEADER);
	_ListDisassembler.SetColumnWidth(Comment, LVSCW_AUTOSIZE_USEHEADER);

	return true;
}

void CDisasmDlg::CopyToClipboard(const WCHAR* pText)
{
	if (OpenClipboard())
	{
		EmptyClipboard();
		size_t len = wcslen(pText);
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(WCHAR));
		if (hMem)
		{
			wcscpy_s(static_cast<WCHAR*>(GlobalLock(hMem)), len + 1, pText);
			GlobalUnlock(hMem);
			if (!SetClipboardData(CF_UNICODETEXT, hMem))
			{
				GlobalFree(hMem);
			}
		}
		CloseClipboard();
	}
}

void CDisasmDlg::ToUpperCase(WCHAR* pLowercase)
{
	for (size_t i = 0; i < wcslen(pLowercase); i++)
	{
		if (pLowercase[i] != L'x')
		{
			pLowercase[i] = towupper(pLowercase[i]);
		}
	}
}

void CDisasmDlg::DoColorInstruction(LPNMLVCUSTOMDRAW lpLVCustomDraw, DWORD_PTR itemIndex)
{
	if (ProcessAccessHelper::_insts[itemIndex].flags == FLAG_NOT_DECODABLE)
	{
		lpLVCustomDraw->clrText = RGB(255, 255, 255); // white text
		lpLVCustomDraw->clrTextBk = RGB(255, 0, 0); // red background
	}
	else if (META_GET_FC(ProcessAccessHelper::_insts[itemIndex].meta) == FC_RET)
	{
		lpLVCustomDraw->clrTextBk = RGB(0, 255, 255); // aqua
	}
	else if (META_GET_FC(ProcessAccessHelper::_insts[itemIndex].meta) == FC_CALL)
	{
		lpLVCustomDraw->clrTextBk = RGB(255, 255, 0); // yellow
	}
	else if (META_GET_FC(ProcessAccessHelper::_insts[itemIndex].meta) == FC_UNC_BRANCH)
	{
		lpLVCustomDraw->clrTextBk = RGB(0x32, 0xCD, 0x32); // limegreen
	}
	else if (META_GET_FC(ProcessAccessHelper::_insts[itemIndex].meta) == FC_CND_BRANCH)
	{
		lpLVCustomDraw->clrTextBk = RGB(0xAD, 0xFF, 0x2F); // greenyellow
	}
}

void CDisasmDlg::FollowInstruction(int index) {
	DWORD_PTR address = 0;
	DWORD_PTR temp = 0;
	DWORD fc = META_GET_FC(ProcessAccessHelper::_insts[index].meta);
	if (ProcessAccessHelper::_insts[index].flags != FLAG_NOT_DECODABLE) {
		if (fc == FC_CALL || fc == FC_UNC_BRANCH || fc == FC_CND_BRANCH) {
			BOOL isWow64 = FALSE;
			DWORD pointerSize = 4;
			::IsWow64Process(ProcessAccessHelper::_hProcess, &isWow64);
			if (!isWow64) {
				pointerSize = 8;
				if (ProcessAccessHelper::_insts[index].flags & FLAG_RIP_RELATIVE) {
					temp = INSTRUCTION_GET_RIP_TARGET(&ProcessAccessHelper::_insts[index]);
					if (!ProcessAccessHelper::ReadMemoryFromProcess(temp, pointerSize, &address)) {
						address = 0;
					}
				}
			}

			if (ProcessAccessHelper::_insts[index].ops[0].type == O_PC) {
				address = INSTRUCTION_GET_TARGET(&ProcessAccessHelper::_insts[index]);
			}
			if (ProcessAccessHelper::_insts[index].ops[0].type == O_DISP) {
				temp = ProcessAccessHelper::_insts[index].disp;
				if (!ProcessAccessHelper::ReadMemoryFromProcess(temp, pointerSize, &address)) {
					address = 0;
				}
			}
			if (address != 0) {
				DisassembleNewAddress(address);
			}
		}
	}
}

bool CDisasmDlg::GetDisassemblyComment(unsigned int index)
{
	DWORD_PTR address = 0;
	DWORD_PTR temp = 0;
	DWORD fc = META_GET_FC(ProcessAccessHelper::_insts[index].meta);
	if (ProcessAccessHelper::_insts[index].flags != FLAG_NOT_DECODABLE) {
		if (fc == FC_CALL || fc == FC_UNC_BRANCH || fc == FC_CND_BRANCH) {
			BOOL isWow64 = FALSE;
			DWORD pointerSize = 4;
			::IsWow64Process(ProcessAccessHelper::_hProcess, &isWow64);
			if (!isWow64) {
				pointerSize = 8;
				if (ProcessAccessHelper::_insts[index].flags & FLAG_RIP_RELATIVE) {
					temp = INSTRUCTION_GET_RIP_TARGET(&ProcessAccessHelper::_insts[index]);
					swprintf_s(_temp, L"-> " PRINTF_DWORD_PTR_FULL, temp);

					if (ProcessAccessHelper::ReadMemoryFromProcess(temp, pointerSize, &address)) {
						swprintf_s(_temp, L"%s -> " PRINTF_DWORD_PTR_FULL, _temp, address);
					}
				}
			}
			else if (ProcessAccessHelper::_insts[index].ops[0].type == O_PC) {
				address = INSTRUCTION_GET_TARGET(&ProcessAccessHelper::_insts[index]);
				swprintf_s(_temp, L"-> " PRINTF_DWORD_PTR_FULL, address);
			}
			else if (ProcessAccessHelper::_insts[index].ops[0].type == O_DISP) {
				temp = ProcessAccessHelper::_insts[index].disp;
				swprintf_s(_temp, L"-> " PRINTF_DWORD_PTR_FULL, temp);

				if (ProcessAccessHelper::ReadMemoryFromProcess(temp, pointerSize, &address)) {
					swprintf_s(_temp, L"%s -> " PRINTF_DWORD_PTR_FULL, _temp, address);
				}
			}
		}
	}

	if (address != 0) {
		AnalyzeAddress(address, _temp);
		return true;
	}

	return false;
}

void CDisasmDlg::DisassembleNewAddress(DWORD_PTR address)
{
	if (_addressHistories[_addressHistories.size() - 1] != address) {
		_addressHistories.push_back(address);
		_addressHistoryIndex = _addressHistories.size() - 1;
		_address.SetValue(_addressHistories[_addressHistoryIndex]);
		if (!DisplayDisassembly()) {
			MessageBox(L"Cannot disassemble memory at this address", L"Error", MB_ICONERROR);
		}
	}
}

void CDisasmDlg::InitAddressCommentList()
{
	HMODULE* phModules;
	WCHAR target[MAX_PATH];
	DWORD numHandles = ProcessAccessHelper::GetModuleHandlesFromProcess(ProcessAccessHelper::_hProcess, &phModules);
	if (numHandles == 0) {
		return;
	}
	for (DWORD i = 0; i < numHandles; i++) {
		if (ProcessAccessHelper::_targetImageBase != (DWORD_PTR)phModules[i]) {
			if (GetModuleFileNameEx(ProcessAccessHelper::_hProcess, phModules[i], target, _countof(target))) {
				AddModuleAddressCommentEntry((DWORD_PTR)phModules[i],
					ProcessAccessHelper::GetSizeOfImageProcess(ProcessAccessHelper::_hProcess, (DWORD_PTR)phModules[i]), target);
			}
		}
	}

	std::sort(_addressComments.begin(), _addressComments.end());
}

void CDisasmDlg::AddModuleAddressCommentEntry(DWORD_PTR address, DWORD moduleSize, const WCHAR* pModulePath) {
	DisasmAddressComment comment;
	const WCHAR* pSlash = wcsrchr(pModulePath, L'\\');
	if (pSlash) {
		pModulePath = pSlash + 1;
	}
	wcscpy_s(comment._comment, _countof(comment._comment), pModulePath);
	comment._address = address;
	comment._type = ADDRESS_TYPE_MODULE;
	comment._moduleSize = moduleSize;

	_addressComments.push_back(comment);
}

void CDisasmDlg::AnalyzeAddress(DWORD_PTR address, WCHAR* pComment) {
	if (_addressComments[0]._address > address) {
		return;
	}
	bool isSuspect = false;
	ApiInfo* pApi = _pApiReader->GetApiByVirtualAddress(address, &isSuspect);
	if (pApi != nullptr) {
		if (pApi->Name[0] == 0) {
			swprintf_s(_temp, L"%s = %s.%04X", pComment, pApi->pModule->GetFileName(), pApi->Oridinal);
		}
		else {
			swprintf_s(_temp, L"%s = %s.%S", pComment, pApi->pModule->GetFileName(), pApi->Name);
		}
	}
	else {
		for (size_t i = 0; i < _addressComments.size(); i++) {
			if (_addressComments[i]._type == ADDRESS_TYPE_MODULE) {
				if (address >= _addressComments[i]._address && address <
					(_addressComments[i]._address + _addressComments[i]._moduleSize))
				{
					swprintf_s(_temp, L"%s = %s", pComment, _addressComments[i]._comment);
					return;
				}
			}
		}
	}
}
