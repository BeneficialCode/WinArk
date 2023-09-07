#include "stdafx.h"
#include "MiniFilterOperationTable.h"
#include "FormatHelper.h"
#include "ClipboardHelper.h"
#include "DriverHelper.h"
#include "SymbolHelper.h"
#include "FileVersionInfoHelper.h"

#define IRP_MJ_CREATE                   0x00
#define IRP_MJ_CREATE_NAMED_PIPE        0x01
#define IRP_MJ_CLOSE                    0x02
#define IRP_MJ_READ                     0x03
#define IRP_MJ_WRITE                    0x04
#define IRP_MJ_QUERY_INFORMATION        0x05
#define IRP_MJ_SET_INFORMATION          0x06
#define IRP_MJ_QUERY_EA                 0x07
#define IRP_MJ_SET_EA                   0x08
#define IRP_MJ_FLUSH_BUFFERS            0x09
#define IRP_MJ_QUERY_VOLUME_INFORMATION 0x0a
#define IRP_MJ_SET_VOLUME_INFORMATION   0x0b
#define IRP_MJ_DIRECTORY_CONTROL        0x0c
#define IRP_MJ_FILE_SYSTEM_CONTROL      0x0d
#define IRP_MJ_DEVICE_CONTROL           0x0e
#define IRP_MJ_INTERNAL_DEVICE_CONTROL  0x0f
#define IRP_MJ_SHUTDOWN                 0x10
#define IRP_MJ_LOCK_CONTROL             0x11
#define IRP_MJ_CLEANUP                  0x12
#define IRP_MJ_CREATE_MAILSLOT          0x13
#define IRP_MJ_QUERY_SECURITY           0x14
#define IRP_MJ_SET_SECURITY             0x15
#define IRP_MJ_POWER                    0x16
#define IRP_MJ_SYSTEM_CONTROL           0x17
#define IRP_MJ_DEVICE_CHANGE            0x18
#define IRP_MJ_QUERY_QUOTA              0x19
#define IRP_MJ_SET_QUOTA                0x1a
#define IRP_MJ_PNP                      0x1b
#define IRP_MJ_PNP_POWER                IRP_MJ_PNP      // Obsolete....
#define IRP_MJ_MAXIMUM_FUNCTION         0x1b

//
//  Along with the existing IRP_MJ_xxxx definitions (0 - 0x1b) in NTIFS.H,
//  this defines all of the operation IDs that can be sent to a mini-filter.
//

#define IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION   ((UCHAR)-1)
#define IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION   ((UCHAR)-2)
#define IRP_MJ_ACQUIRE_FOR_MOD_WRITE                 ((UCHAR)-3)
#define IRP_MJ_RELEASE_FOR_MOD_WRITE                 ((UCHAR)-4)
#define IRP_MJ_ACQUIRE_FOR_CC_FLUSH                  ((UCHAR)-5)
#define IRP_MJ_RELEASE_FOR_CC_FLUSH                  ((UCHAR)-6)
#define IRP_MJ_QUERY_OPEN                            ((UCHAR)-7)


//
//  Leave space for additional FS_FILTER codes here
//

#define IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE             ((UCHAR)-13)
#define IRP_MJ_NETWORK_QUERY_OPEN                    ((UCHAR)-14)
#define IRP_MJ_MDL_READ                              ((UCHAR)-15)
#define IRP_MJ_MDL_READ_COMPLETE                     ((UCHAR)-16)
#define IRP_MJ_PREPARE_MDL_WRITE                     ((UCHAR)-17)
#define IRP_MJ_MDL_WRITE_COMPLETE                    ((UCHAR)-18)
#define IRP_MJ_VOLUME_MOUNT                          ((UCHAR)-19)
#define IRP_MJ_VOLUME_DISMOUNT                       ((UCHAR)-20)


#define FLT_INTERNAL_OPERATION_COUNT                 22

//
//  Not currently implemented
//

/*
#define IRP_MJ_READ_COMPRESSED                      ((UCHAR)-xx)
#define IRP_MJ_WRITE_COMPRESSED                     ((UCHAR)-xx)
#define IRP_MJ_MDL_READ_COMPLETE_REQUEST            ((UCHAR)-xx)
#define IRP_MJ_MDL_WRITE_COMPLETE_COMPRESSED        ((UCHAR)-xx)
*/

//
//  Marks the end of the operation list for registration
//

#define IRP_MJ_OPERATION_END                        ((UCHAR)0x80)

#define FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO     0x00000001

//
//  If set read/write operations that are not non-cached will be skipped:
//  i.e. the mini-filters callback for this operation will be bypassed.
//  This flag is relevant only for IRP_MJ_READ & IRP_MJ_WRITE
//  This of course implies that fast i/o reads and writes will be skipped,
//  since those imply cached i/o by default
//

#define FLTFL_OPERATION_REGISTRATION_SKIP_CACHED_IO     0x00000002

//
//  If set all operations that are not issued on a DASD (volume) handle will be skipped:
//  i.e. the mini-filters callback for this operation will be bypassed.
//  This flag is relevant for all operations
//

#define FLTFL_OPERATION_REGISTRATION_SKIP_NON_DASD_IO   0x00000004

#define FLTFL_OPERATION_REGISTRATION_SKIP_NON_CACHED_NON_PAGING_IO     0x00000008



LRESULT COperationTable::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return 0;
}

LRESULT COperationTable::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lparam, BOOL& /*bHandled*/) {
	return 0;
}
LRESULT COperationTable::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	PaintTable(m_hWnd);
	return 0;
}

LRESULT COperationTable::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT COperationTable::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT COperationTable::OnUserVabs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT COperationTable::OnUserVrel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT COperationTable::OnUserChgs(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	Tablefunction(m_hWnd, uMsg, wParam, lParam);
	return 0;
}

LRESULT COperationTable::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	return Tablefunction(m_hWnd, WM_VSCROLL, zDelta >= 0 ? 0 : 1, wParam);
}
LRESULT COperationTable::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT COperationTable::OnLBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT COperationTable::OnLBtnUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT COperationTable::OnRBtnDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	/*CMenu menu;
	CMenuHandle hSubMenu;
	menu.LoadMenu(IDR_KERNEL_HOOK_CONTEXT);
	hSubMenu = menu.GetSubMenu(0);
	POINT pt;
	::GetCursorPos(&pt);
	bool show = Tablefunction(m_hWnd, uMsg, wParam, lParam);
	if (show) {
		auto id = (UINT)TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, nullptr);
		if (id) {
			PostMessage(WM_COMMAND, id);
		}
	}*/

	return 0;
}
LRESULT COperationTable::OnUserSts(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT COperationTable::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT COperationTable::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}
LRESULT COperationTable::OnSysKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	return Tablefunction(m_hWnd, uMsg, wParam, lParam);
}

COperationTable::COperationTable(BarInfo& bars, TableInfo& table,std::wstring filterName) 
	:CTable(bars, table),m_Name(filterName) {
	SetTableWindowInfo(bars.nbar);
	Refresh();
}

int COperationTable::ParseTableEntry(CString& s, char& mask, int& select, OperationCallbackInfo& info, int column) {
	// FilterHandle,MajorCode,OperationType,Flag,Address,CallbackType,Company,Module
	switch (static_cast<Column>(column))
	{
		case Column::Address:
			s.Format(L"0x%p", info.Routine);
			break;

		case Column::FilterHandle:
			s.Format(L"0x%p", info.FilterHandle);
			break;

		case Column::CallbackType:
		{
			switch (info.Type)
			{
				case FilterType::PostOperation:
					s = L"PostOperation";
					break;

				case FilterType::PreOperation:
					s = L"PreOperation";
					break;
			}
			break;
		}
		case Column::OperationType:
			s = OperationTypeToString(info.MajorCode);
			break;

		case Column::Flag:
			s = FlagToString(info.Flags);
			break;

		case Column::Company:
			s = info.Company.c_str();
			break;

		case Column::Module:
			s = Helpers::StringToWstring(info.Module).c_str();
			break;

		case Column::MajorCode:
			s.Format(L"%d <0x%02x>", info.MajorCode, info.MajorCode);
			break;
	}
	return s.GetLength();
}

bool COperationTable::CompareItems(const OperationCallbackInfo& s1, const OperationCallbackInfo& s2, int col, bool asc) {
	switch (col)
	{
		case 0:

			break;
		default:
			break;
	}
	return false;
}



void COperationTable::Refresh() {
	m_Table.data.n = 0;
	m_Table.data.info.clear();

	ULONG offset = SymbolHelper::GetFltmgrStructMemberOffset("_FLT_FILTER", 
		"Operations");

	auto len = (ULONG)m_Name.length();
	DWORD size = len * sizeof(WCHAR) + sizeof(MiniFilterData);

	auto pData = std::make_unique<BYTE[]>(size);
	if (!pData)
		return;

	auto data = reinterpret_cast<MiniFilterData*>(pData.get());
	data->OperationsOffset = offset;
	data->Length = len;
	::wcscpy_s(data->Name, len + 1, m_Name.c_str());

	DWORD dataSize = size;

	int maxCount = IRP_MJ_MAXIMUM_FUNCTION + FLT_INTERNAL_OPERATION_COUNT;
	size = maxCount * sizeof(OperationInfo);
	wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_READWRITE));
	OperationInfo* p = (OperationInfo*)buffer.get();

	if (p != nullptr) {
		memset(p, 0, size);
		DriverHelper::EnumMiniFilterOperations(data, dataSize,p, size);
		for (int i = 0; (p->MajorFunction != IRP_MJ_OPERATION_END&& i<maxCount); i++) {
			OperationCallbackInfo info;
			info.Module = Helpers::GetKernelModuleByAddress((ULONG_PTR)p->PostOperation);
			std::wstring path = Helpers::StringToWstring(info.Module);
			info.Company = FileVersionInfoHelpers::GetCompanyName(path);
			info.Routine = p->PostOperation;
			info.FilterHandle = p->FilterHandle;
			info.Flags = p->Flags;
			info.MajorCode = p->MajorFunction;
			info.Type = FilterType::PostOperation;
			m_Table.data.info.push_back(info);

			info.Module = Helpers::GetKernelModuleByAddress((ULONG_PTR)p->PreOperation);
			path = Helpers::StringToWstring(info.Module);
			info.Company = FileVersionInfoHelpers::GetCompanyName(path);
			info.Routine = p->PreOperation;
			info.FilterHandle = p->FilterHandle;
			info.Flags = p->Flags;
			info.MajorCode = p->MajorFunction;
			info.Type = FilterType::PreOperation;
			m_Table.data.info.push_back(info);
			p++;
		}
	}


	m_Table.data.n = m_Table.data.info.size();
}

std::wstring COperationTable::GetSingleOperationInfo(OperationCallbackInfo& info) {
	CString text;
	CString s;



	text += L"\r\n";

	return text.GetString();
}

LRESULT COperationTable::OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Refresh();

	return TRUE;
}

PCWSTR COperationTable::OperationTypeToString(UCHAR type) {
	switch (type)
	{
		case IRP_MJ_CREATE:  return L"Create";
		case IRP_MJ_CREATE_NAMED_PIPE: return L"CreatePipe";
		case IRP_MJ_CREATE_MAILSLOT: return L"CreateMailsolt";
		case IRP_MJ_READ: return L"Read";
		case IRP_MJ_WRITE: return L"Write";
		case IRP_MJ_QUERY_INFORMATION: return L"QueryFileInformation";
		case IRP_MJ_SET_INFORMATION: return L"SetFileInformation";
		case IRP_MJ_QUERY_EA: return L"QueryEa";
		case IRP_MJ_SET_EA: return L"SetEa";
		case IRP_MJ_QUERY_VOLUME_INFORMATION: return L"QueryVolumeInformation";
		case IRP_MJ_SET_VOLUME_INFORMATION: return L"SetVolumeInformation";
		case IRP_MJ_DIRECTORY_CONTROL: return L"DirectoryControl";
		case IRP_MJ_FILE_SYSTEM_CONTROL: return L"FileSystemControl";
		case IRP_MJ_INTERNAL_DEVICE_CONTROL: return L"DeviceInternalIoControl";
		case IRP_MJ_DEVICE_CONTROL: return L"DeviceIoControl";
		case IRP_MJ_LOCK_CONTROL: return L"LockControl";
		case IRP_MJ_QUERY_SECURITY: return L"QuerySecurity";
		case IRP_MJ_SET_SECURITY: return L"SetSecurity";
		case IRP_MJ_QUERY_QUOTA: return L"QueryQuota";
		case IRP_MJ_SET_QUOTA: return L"SetQuota";
		case IRP_MJ_PNP: return L"Pnp";
		case IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION: return L"AcquireForSectionSynchronization";
		case IRP_MJ_ACQUIRE_FOR_MOD_WRITE: return L"AcquireForModifiedPageWriter";
		case IRP_MJ_RELEASE_FOR_MOD_WRITE: return L"ReleaseForModifiedPageWriter";
		case IRP_MJ_QUERY_OPEN: return L"QueryOpen";
		case IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE: return L"FastIoCheckIfPossible";
		case IRP_MJ_NETWORK_QUERY_OPEN: return L"NetworkQueryOpen";
		case IRP_MJ_MDL_READ: return L"MdlRead";
		case IRP_MJ_MDL_READ_COMPLETE: return L"MdlReadComplete";
		case IRP_MJ_PREPARE_MDL_WRITE: return L"PrepareMdlWrite";
		case IRP_MJ_MDL_WRITE_COMPLETE: return L"MdlWriteComplete";
		case IRP_MJ_VOLUME_MOUNT: return L"MountVolume";
		case IRP_MJ_ACQUIRE_FOR_CC_FLUSH: return L"AcquireForCacheFlash";
		case IRP_MJ_CLEANUP: return L"Cleanup";
		case IRP_MJ_CLOSE: return L"Close";
		case IRP_MJ_FLUSH_BUFFERS: return L"FlushBuffers";
		case IRP_MJ_RELEASE_FOR_CC_FLUSH: return L"ReleaseForCacheFlush";
		case IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION: return L"ReleaseForSectionSynchronization";
		case IRP_MJ_SHUTDOWN: return L"Shutdown";
		case IRP_MJ_VOLUME_DISMOUNT: return L"DismountVolume";
		case IRP_MJ_POWER: return L"Power";
		case IRP_MJ_SYSTEM_CONTROL: return L"SystemControl";
		case IRP_MJ_DEVICE_CHANGE: return L"DeviceChange";
		case (UCHAR)-8: return L"Reserved-8";
		case (UCHAR)-9: return L"Reserved-9";
		case (UCHAR)-10: return L"Reserved-10";
		case (UCHAR)-11: return L"Reserved-11";
		case (UCHAR)-12: return L"Reserved-12";
	}
	return L"";
}

CString COperationTable::FlagToString(DWORD flag) {
	static const struct {
		PCWSTR Text;
		DWORD Value;
	} operations[] = {
		{ L"", 0 },
		{ L"Skip Paging I/O", FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO  },
		{ L"Skip Cache I/O", FLTFL_OPERATION_REGISTRATION_SKIP_CACHED_IO  },
		{ L"Skip Non-DASD I/O", FLTFL_OPERATION_REGISTRATION_SKIP_NON_DASD_IO  },
		{ L"Skip Non-Cached Non-Paging I/Oy", FLTFL_OPERATION_REGISTRATION_SKIP_NON_CACHED_NON_PAGING_IO  },
	};

	CString text = L"";

	auto it = std::find_if(std::begin(operations), std::end(operations), [flag](auto& p) {
		return (p.Value & flag) != 0;
		});
	if (it != std::end(operations)) {
		text = it->Text;
	}
	else {
		text = L"";
	}

	return text;
}