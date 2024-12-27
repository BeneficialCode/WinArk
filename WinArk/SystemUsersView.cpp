#include "stdafx.h"
#include "SystemUsersView.h"
#include <algorithm>
#include "SortHelper.h"
#include <lm.h>
#include <sddl.h>
#include <SecurityHelper.h>
#include "Helpers.h"


std::string CSystemUsersView::GetReadableTime(DWORD time) const {
	if (time == 0) {
		return std::string("Never");
	}

	// Convert DWORD time to std::chrono::system_clock::time_point
	std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(time);

	// Convert to time_t for localtime conversion
	std::time_t tt = std::chrono::system_clock::to_time_t(tp);

	// Convert to local time
	std::tm local_tm = *std::localtime(&tt);

	// Format the time as a string
	std::ostringstream oss;
	oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
	return oss.str();
}


CString CSystemUsersView::GetColumnText(HWND, int row, int col) const {
	const auto& item = m_Users[row];

	switch (col)
	{
		case 0:
			return std::format(L"{:2d}", item.UserId).c_str();
		case 1:
			return item.UserName.c_str();
		case 2:
			return std::format(L"{:2d}", item.NumLogons).c_str();
		case 3:
		{
			std::string time = GetReadableTime(item.LastLogon);
			return Helpers::StringToWstring(time).c_str();
			break;
		}
		case 4:
		{
			std::string time = GetReadableTime(item.LastLogoff);
			return Helpers::StringToWstring(time).c_str();
			break;
		}
		case 5:
		{
			return item.Sid.c_str();
		}
		default:
			break;
	}

	return L"";
}

bool CSystemUsersView::IsUpdating() const {
	return false;
}

void CSystemUsersView::DoSort(const SortInfo* si) {
	return;
}

bool CSystemUsersView::IsSortable(HWND, int col) const {
	return False;
}

LRESULT CSystemUsersView::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	m_hWndClient = m_List.Create(*this, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | LVS_SINGLESEL | LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS);
	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);

	auto cm = GetColumnManager(m_List);
	cm->AddColumn(L"User Id", LVCFMT_RIGHT, 60, ColumnFlags::Mandatory | ColumnFlags::Const | ColumnFlags::Visible);
	cm->AddColumn(L"User Name", LVCFMT_LEFT, 130, ColumnFlags::Mandatory | ColumnFlags::Const | ColumnFlags::Visible);
	cm->AddColumn(L"Num Logons", LVCFMT_LEFT, 140, ColumnFlags::Mandatory | ColumnFlags::Const | ColumnFlags::Visible);
	cm->AddColumn(L"Last Logon", LVCFMT_LEFT, 160, ColumnFlags::Mandatory | ColumnFlags::Const | ColumnFlags::Visible);
	cm->AddColumn(L"Last Logoff", LVCFMT_LEFT, 160, ColumnFlags::Mandatory | ColumnFlags::Const | ColumnFlags::Visible);
	cm->AddColumn(L"SID", LVCFMT_LEFT, 360, ColumnFlags::Mandatory | ColumnFlags::Const | ColumnFlags::Visible);

	cm->UpdateColumns();

	Refresh();

	return 0;
}

void CSystemUsersView::Refresh() {
	m_Users = EnumSystemUsers();
	m_List.SetItemCountEx(static_cast<int>(m_Users.size()), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
	m_List.RedrawItems(m_List.GetTopIndex(), m_List.GetTopIndex() + m_List.GetCountPerPage());
}

std::vector<CSystemUsersView::UserInfo> CSystemUsersView::EnumSystemUsers() {
	std::vector<UserInfo> infos;

	NET_API_STATUS nStatus = NERR_Success;

	do
	{
		LPUSER_INFO_3 pUserInfo = NULL;
		DWORD entriesRead, totalEntries, resumeHandle = 0;
		nStatus = NetUserEnum(NULL, 3, FILTER_NORMAL_ACCOUNT, (LPBYTE*)&pUserInfo,
			MAX_PREFERRED_LENGTH, &entriesRead, &totalEntries, &resumeHandle);

		if ((nStatus == NERR_Success) || (nStatus == ERROR_MORE_DATA)) {
			LPUSER_INFO_3 pTemp = pUserInfo;
			infos.reserve(entriesRead);
			for (int i = 0; i < entriesRead; i++) {
				if (pTemp == NULL)
					break;

				UserInfo info;
				info.UserName = pTemp->usri3_name;
				info.Comment = pTemp->usri3_comment;
				info.LastLogoff = pTemp->usri3_last_logoff;
				info.LastLogon = pTemp->usri3_last_logon;
				info.UserId = pTemp->usri3_user_id;
				info.NumLogons = pTemp->usri3_num_logons;
				info.Sid = SecurityHelper::GetSidFromUser(pTemp->usri3_name);

				infos.emplace_back(std::move(info));
				pTemp++;
			}
		}

		if (pUserInfo != NULL) {
			NetApiBufferFree(pUserInfo);
			pUserInfo = NULL;
		}
	} while (nStatus == ERROR_MORE_DATA);

	return infos;
}