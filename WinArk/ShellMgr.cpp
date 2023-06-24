#include "stdafx.h"
#include "ShellMgr.h"

int CShellMgr::GetIconIndex(LPITEMIDLIST lpi, UINT flags){
	SHFILEINFO sfi = { 0 };
	DWORD_PTR ret = ::SHGetFileInfo((LPCTSTR)lpi, 0, &sfi, 
		sizeof(SHFILEINFO), flags);
	return (ret != 0) ? sfi.iIcon : -1;
}

void CShellMgr::GetNormalAndSelectedIcons(LPITEMIDLIST lpifq, LPTVITEM lptvitem)
{
	int ret = lptvitem->iImage = GetIconIndex(lpifq, SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	ATLASSERT(ret >= 0);
	ret = lptvitem->iSelectedImage = GetIconIndex(lpifq, 
		SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_OPENICON);
	ATLASSERT(ret >= 0);
}

LPITEMIDLIST CShellMgr::ConcatPidls(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2){
	UINT cb1 = 0;
	if (pidl1 != nullptr)
		cb1 = GetSize(pidl1) - sizeof(pidl1->mkid.cb);

	UINT cb2 = GetSize(pidl2);
	
	LPITEMIDLIST pidlNew = (LPITEMIDLIST)::CoTaskMemAlloc(cb1 + cb2);
	if (pidlNew != nullptr) {
		if (pidl1 != nullptr)
			memcpy(pidlNew, pidl1, cb1);

		memcpy((LPSTR)pidlNew + cb1, pidl2, cb2);
	}

	return pidlNew;
}

BOOL CShellMgr::GetName(LPSHELLFOLDER lpsf, LPITEMIDLIST lpi, DWORD flags, LPTSTR lpFriendlyName){
	BOOL success = TRUE;
	STRRET str = { STRRET_CSTR };
	
	if (lpsf->GetDisplayNameOf(lpi, flags, &str) == NOERROR) {
		// https://blog.csdn.net/allway2/article/details/123548084
		CComHeapPtr<wchar_t> spszName;
		StrRetToStr(&str, lpi, &spszName);
		lstrcpy(lpFriendlyName, spszName);
	}
	else {
		success = FALSE;
	}

	return success;
}

LPITEMIDLIST CShellMgr::Next(LPCITEMIDLIST pidl){
	LPSTR lpMem = (LPSTR)pidl;
	lpMem += pidl->mkid.cb;
	return (LPITEMIDLIST)lpMem;
}

UINT CShellMgr::GetSize(LPCITEMIDLIST pidl){
	UINT totalSize = 0;
	if (pidl != nullptr) {
		totalSize += sizeof(pidl->mkid.cb); // Null terminator
		while (pidl->mkid.cb != 0) {
			totalSize += pidl->mkid.cb;
			pidl = Next(pidl);
		}
	}

	return totalSize;
}

LPITEMIDLIST CShellMgr::CopyITEMID(LPITEMIDLIST lpi){
	LPITEMIDLIST lpiTemp = (LPITEMIDLIST)::CoTaskMemAlloc(lpi->mkid.cb + sizeof(lpi->mkid.cb));
	::CopyMemory(lpiTemp, lpi, lpi->mkid.cb + sizeof(lpi->mkid.cb));
	return lpiTemp;
}

LPITEMIDLIST CShellMgr::GetFullyQualPidl(LPSHELLFOLDER lpsf, LPITEMIDLIST lpi){
	WCHAR szBuff[MAX_PATH] = { 0 };

	if (!GetName(lpsf, lpi, SHGDN_FORPARSING, szBuff))
		return nullptr;

	CComPtr<IShellFolder> spDesktop;
	HRESULT hr = ::SHGetDesktopFolder(&spDesktop);
	if (FAILED(hr))
		return nullptr;

	LPITEMIDLIST lpifq = nullptr;
	ULONG attribs = 0;
	hr = spDesktop->ParseDisplayName(0, nullptr, szBuff, nullptr, &lpifq, &attribs);
	if (FAILED(hr))
		return nullptr;

	return lpifq;
}

BOOL CShellMgr::DoContextMenu(HWND hWnd, LPSHELLFOLDER lpsfParent, LPITEMIDLIST lpi, POINT point){
	CComPtr<IContextMenu> spContextMenu;
	HRESULT hr = lpsfParent->GetUIObjectOf(hWnd, 1,
		(const struct _ITEMIDLIST**)&lpi, IID_IContextMenu, 0,
		(LPVOID*)&spContextMenu);
	if (FAILED(hr))
		return FALSE;

	HMENU hMenu = ::CreatePopupMenu();
	if (hMenu == nullptr)
		return FALSE;

	// Get the context menu for the item
	hr = spContextMenu->QueryContextMenu(hMenu, 0, 1, 0x7FFF, CMF_EXPLORE);
	if (FAILED(hr))
		return FALSE;

	CMenuHandle menu(hMenu);
	menu.AppendMenu(MF_STRING, ID_DELETE_FILE, L"Force Delete");

	int idCmd = ::TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON,
		point.x, point.y, 0, hWnd, nullptr);

	if (idCmd == ID_DELETE_FILE) {
		SendMessage(hWnd, WM_COMMAND, idCmd, 0);
		return TRUE;
	}

	if (idCmd != 0) {
		// Execute the command that was selected.
		CMINVOKECOMMANDINFO cmi = { 0 };
		cmi.cbSize = sizeof(CMINVOKECOMMANDINFO);
		cmi.fMask = 0;
		cmi.hwnd = hWnd;
		cmi.lpVerb = MAKEINTRESOURCEA(idCmd - 1);
		cmi.lpParameters = NULL;
		cmi.lpDirectory = NULL;
		cmi.nShow = SW_SHOWNORMAL;
		cmi.dwHotKey = 0;
		cmi.hIcon = NULL;
		hr = spContextMenu->InvokeCommand(&cmi);
	}

	::DestroyMenu(hMenu);
	return TRUE;
}