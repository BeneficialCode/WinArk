#include "stdafx.h"
#include "ExplorerView.h"
#include "DriverHelper.h"

LRESULT CExplorerView::OnCreate(UINT, WPARAM, LPARAM, BOOL&){
	m_WndSplitter.Create(m_hWnd, rcDefault, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0);

	m_WndTreeView.Create(m_WndSplitter, rcDefault, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
		WS_EX_CLIENTEDGE);

	m_WndListView.Create(m_WndSplitter, rcDefault, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		LVS_REPORT | LVS_AUTOARRANGE | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS,
		WS_EX_CLIENTEDGE);
	m_WndListView.SetExtendedListViewStyle(LVS_EX_TRACKSELECT | LVS_EX_ONECLICKACTIVATE);
		
	InitViews();

	m_WndSplitter.SetSplitterPanes(m_WndTreeView, m_WndListView);
	m_WndSplitter.SetSplitterPos(25);
	m_WndSplitter.UpdateSplitterLayout();

	CComPtr<IShellFolder> spFolder;
	HRESULT hr = ::SHGetDesktopFolder(&spFolder);
	if (SUCCEEDED(hr)) {
		CWaitCursor wait;

		FillTreeView(spFolder, nullptr, TVI_ROOT);
		m_WndTreeView.Expand(m_WndTreeView.GetRootItem());
		m_WndTreeView.SelectItem(m_WndTreeView.GetRootItem());
	}

	return 0;
}

void CExplorerView::InitViews(){
	// Get Desktop folder
	CShellItemIDList spidl;
	HRESULT hRet = ::SHGetSpecialFolderLocation(m_hWnd, CSIDL_DESKTOP, &spidl);
	ATLASSERT(SUCCEEDED(hRet));

	// Get system image lists
	SHFILEINFO sfi = { 0 };
	HIMAGELIST hImageList = (HIMAGELIST)::SHGetFileInfo(spidl, 0, &sfi, sizeof(sfi),
		SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_ICON);
	ATLASSERT(hImageList != nullptr);

	memset(&sfi, 0, sizeof(SHFILEINFO));
	HIMAGELIST hImageListSmall = (HIMAGELIST)::SHGetFileInfo(spidl, 0, &sfi, sizeof(sfi),
		SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	ATLASSERT(hImageListSmall != nullptr);

	// Set tree view image list
	m_WndTreeView.SetImageList(hImageListSmall, 0);

	// Create list view columns
	m_WndListView.InsertColumn(0, L"Name", LVCFMT_LEFT, 200, 0);
	m_WndListView.InsertColumn(1, _T("Size"), LVCFMT_RIGHT, 100, 1);
	m_WndListView.InsertColumn(2, _T("Type"), LVCFMT_LEFT, 100, 2);
	m_WndListView.InsertColumn(3, _T("Modified"), LVCFMT_LEFT, 100, 3);
	m_WndListView.InsertColumn(4, _T("Attributes"), LVCFMT_RIGHT, 100, 4);

	// Set list view image lists
	m_WndListView.SetImageList(hImageList, LVSIL_NORMAL);
	m_WndListView.SetImageList(hImageListSmall, LVSIL_SMALL);
}

LRESULT CExplorerView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
	int cx = GET_X_LPARAM(lParam), cy = GET_Y_LPARAM(lParam);
	m_WndSplitter.MoveWindow(0, 0, cx, cy);
	RECT rect;
	GetClientRect(&rect);
	m_WndSplitter.SetSplitterPos((rect.right - rect.left) / 4);
	m_WndSplitter.UpdateSplitterLayout();
	return 0;
}

HRESULT CExplorerView::FillTreeView(LPSHELLFOLDER lpsf, LPITEMIDLIST lpifq, HTREEITEM hParent)
{
	if (lpsf == nullptr)
		return E_POINTER;

	CComPtr<IEnumIDList> spIDList;
	HRESULT hr = lpsf->EnumObjects(m_hWnd, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS| SHCONTF_INCLUDEHIDDEN, &spIDList);
	if (FAILED(hr))
		return hr;

	CShellItemIDList lpi;
	CShellItemIDList lpifqThisItem;
	LPTVITEMDATA lptvid = nullptr;
	ULONG fetched = 0;
	WCHAR szBuff[MAX_PATH] = { 0 };

	TVITEM tvi = { 0 };				// TreeView Item
	TVINSERTSTRUCT tvins = { 0 };	// TreeView Insert Struct
	HTREEITEM hPrev = nullptr;		// Previous Item Add
	
	// Hourglass on
	CWaitCursor wait;

	int cnt = 0;
	while (spIDList->Next(1, &lpi, &fetched) == S_OK) {
		// Create a fully qualified path to the current item
		// The SH* shell api's take a fully qualified path pidl,
		// (see GetIcon above where I call SHGetFileInfo) whereas the
		// interface methods take a relative path pidl.
		ULONG attrs = SFGAO_HASSUBFOLDER | SFGAO_FOLDER;
		lpsf->GetAttributesOf(1, (LPCITEMIDLIST*)&lpi, &attrs);

		if ((attrs & (SFGAO_HASSUBFOLDER | SFGAO_FOLDER)) != 0) {
			// We need this next if statement so that we don't add things like
			// the MSN to our tree. MSN is not a folder, but according to the
			// shell is has subfolders....
			// Shell File Get Attributes Options
			if (attrs & SFGAO_FOLDER) {
				tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
				
				if (attrs & SFGAO_HASSUBFOLDER) {
					// This item has sub-folders, so let's put the + in the TreeView.
					// The first time the user clicks on the item, we'll populate the
					// sub-folders then.
					tvi.cChildren = 1;
					tvi.mask |= TVIF_CHILDREN;
				}

				// OK, let's get some memory for our ITEMDATA struct
				lptvid = new TVITEMDATA;
				if (lptvid == NULL)
					return E_FAIL;

				// Now get the friendly name that we'll put in the treeview...
				if (!m_ShellMgr.GetName(lpsf, lpi, SHGDN_NORMAL, szBuff))
					return E_FAIL;

				tvi.pszText = szBuff;
				tvi.cchTextMax = MAX_PATH;


				lpifqThisItem = m_ShellMgr.ConcatPidls(lpifq, lpi);

				// Now, make a copy of the ITEMIDLIST
				lptvid->lpi = m_ShellMgr.CopyITEMID(lpi);

				m_ShellMgr.GetNormalAndSelectedIcons(lpifqThisItem, &tvi);

				lptvid->spParentFolder.p = lpsf;    // Store the parent folders SF
				lpsf->AddRef();

				lptvid->lpifq = m_ShellMgr.ConcatPidls(lpifq, lpi);

				tvi.lParam = (LPARAM)lptvid;

				tvins.item = tvi;
				tvins.hInsertAfter = hPrev;
				tvins.hParent = hParent;

				// Add the item to the tree and combo
				hPrev = TreeView_InsertItem(m_WndTreeView.m_hWnd, &tvins);
				int nIndent = 0;
				while (NULL != (hPrev = (HTREEITEM)m_WndTreeView.SendMessage(TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM)hPrev)))
				{
					nIndent++;
				}
			}

			lpifqThisItem = nullptr;
		}

		lpi = nullptr; // Finally, free the pidl that the shell gave us...
	}

	return hr;
}

LRESULT CExplorerView::OnTVSelChanged(int, LPNMHDR pnmh, BOOL&) {
	LPNMTREEVIEW lpTV = (LPNMTREEVIEW)pnmh;
	LPTVITEMDATA lptvid = (LPTVITEMDATA)lpTV->itemNew.lParam;
	if (lptvid != nullptr) {
		CComPtr<IShellFolder> spFolder;
		HRESULT hr = lptvid->spParentFolder->BindToObject(lptvid->lpi, 0,
			IID_IShellFolder, (LPVOID*)&spFolder);
		if (FAILED(hr))
			return hr;

		if (m_WndListView.GetItemCount() > 0)
			m_WndListView.DeleteAllItems();
		
		FillListView(lptvid, spFolder);

		WCHAR psz[MAX_PATH] = { 0 };
		m_ShellMgr.GetName(lptvid->spParentFolder, lptvid->lpi, SHGDN_FORPARSING, psz);

		if (StrChr(psz, L'{')) // special case
			m_ShellMgr.GetName(lptvid->spParentFolder, lptvid->lpi, SHGDN_NORMAL, psz);

		int nImage = 0;
		int nSelectedImage = 0;
		m_WndTreeView.GetItemImage(lpTV->itemNew.hItem, nImage, nSelectedImage);
	}

	return 0;
}

BOOL CExplorerView::FillListView(LPTVITEMDATA lptvid, LPSHELLFOLDER pShellFolder) {
	ATLASSERT(pShellFolder != nullptr);

	CComPtr<IEnumIDList> spEnumIDList;
	HRESULT hr = pShellFolder->EnumObjects(m_WndListView.GetParent(),
		SHCONTF_FOLDERS | SHCONTF_NONFOLDERS| SHCONTF_INCLUDEHIDDEN, &spEnumIDList);
	if (FAILED(hr))
		return FALSE;

	CShellItemIDList lpifqThisItem;
	CShellItemIDList lpi;
	ULONG fetched = 0;
	UINT flags = 0;
	LVITEM lvi = { 0 };
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	int ctr = 0;

	while (spEnumIDList->Next(1, &lpi, &fetched) == S_OK) {
		// Get some memory for the ITEMDATA structure
		LPLVITEMDATA lplvid = new LVITEMDATA;
		if (lplvid == nullptr)
			return FALSE;

		// Since you are interested in the display attributes as well as other attributes
		// you need to set attrs to SFGAO_DISPLAYATTRMASK before calling GetAttributesOf
		ULONG attrs = SFGAO_DISPLAYATTRMASK;
		hr = pShellFolder->GetAttributesOf(1, (const struct _ITEMIDLIST**)&lpi, &attrs);
		if (FAILED(hr))
			return FALSE;

		lplvid->Attribs = attrs;

		lpifqThisItem = m_ShellMgr.ConcatPidls(lptvid->lpifq, lpi);

		lvi.iItem = ctr;
		lvi.iSubItem = 0;
		lvi.pszText = LPSTR_TEXTCALLBACK;
		lvi.cchTextMax = MAX_PATH;
		flags = SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON;
		lvi.iImage = I_IMAGECALLBACK;

		lplvid->spParentFolder.p = pShellFolder;
		pShellFolder->AddRef();

		// Now make a copy of the ITEMIDLIST
		lplvid->lpi = m_ShellMgr.CopyITEMID(lpi);

		lvi.lParam = (LPARAM)lplvid;

		// Add the item to the list view control
		int n = m_WndListView.InsertItem(&lvi);
		m_WndListView.AddItem(n, 1, LPSTR_TEXTCALLBACK, I_IMAGECALLBACK);
		m_WndListView.AddItem(n, 2, LPSTR_TEXTCALLBACK, I_IMAGECALLBACK);
		m_WndListView.AddItem(n, 3, LPSTR_TEXTCALLBACK, I_IMAGECALLBACK);
		m_WndListView.AddItem(n, 4, LPSTR_TEXTCALLBACK, I_IMAGECALLBACK);
		
		ctr++;
		lpifqThisItem = nullptr;
		lpi = nullptr; // free PIDL the shell gave you
	}

	return TRUE;
}

LRESULT CExplorerView::OnLVGetDispInfo(int, LPNMHDR pnmh, BOOL&){
	NMLVDISPINFO* plvdi = (NMLVDISPINFO*)pnmh;
	if (plvdi == nullptr)
		return 0;

	LPLVITEMDATA lplvid = (LPLVITEMDATA)plvdi->item.lParam;

	HTREEITEM hti = m_WndTreeView.GetSelectedItem();
	TVITEM tvi = { 0 };
	tvi.mask = TVIF_PARAM;
	tvi.hItem = hti;

	m_WndTreeView.GetItem(&tvi);
	if (tvi.lParam <= 0)
		return 0L;

	LPTVITEMDATA lptvid = (LPTVITEMDATA)tvi.lParam;
	if (lptvid == nullptr)
		return 0L;

	CShellItemIDList pidlTemp = m_ShellMgr.ConcatPidls(lptvid->lpifq, lplvid->lpi);
	plvdi->item.iImage = m_ShellMgr.GetIconIndex(pidlTemp, 
		SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	if (plvdi->item.iSubItem == 0 && (plvdi->item.mask & LVIF_TEXT)) {
		// File Name
		// SHGetDisplayNameFlags
		m_ShellMgr.GetName(lplvid->spParentFolder, lplvid->lpi, SHGDN_NORMAL, plvdi->item.pszText);
	}
	else {
		CComPtr<IShellFolder2> spFolder2;
		HRESULT hr = lptvid->spParentFolder->QueryInterface(IID_IShellFolder2, (void**)&spFolder2);
		if (FAILED(hr))
			return hr;

		SHELLDETAILS sd = { 0 };
		sd.fmt = LVCFMT_CENTER;
		sd.cxChar = 15;

		hr = spFolder2->GetDetailsOf(lplvid->lpi, plvdi->item.iSubItem, &sd);
		if (FAILED(hr))
			return hr;

		CComHeapPtr<wchar_t> spszName;
		StrRetToStr(&sd.str, lplvid->lpi.m_pidl, &spszName);
		lstrcpy(m_szListViewBuffer, spszName);
		plvdi->item.pszText = m_szListViewBuffer;
	}

	plvdi->item.mask |= LVIF_DI_SETITEM;

	return 0;
}

LRESULT CExplorerView::OnTVItemExpanding(int, LPNMHDR pnmh, BOOL&){
	CWaitCursor wait;
	LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)pnmh;
	if ((pnmtv->itemNew.state & TVIS_EXPANDEDONCE))
		return 0L;

	LPTVITEMDATA lptvid = (LPTVITEMDATA)pnmtv->itemNew.lParam;

	CComPtr<IShellFolder> spFolder;
	HRESULT hr = lptvid->spParentFolder->BindToObject(lptvid->lpi, 0, IID_IShellFolder,
		(LPVOID*)&spFolder);

	if (FAILED(hr))
		return hr;

	FillTreeView(spFolder, lptvid->lpifq, pnmtv->itemNew.hItem);

	
	return 0;
}

LRESULT CExplorerView::OnTVDeleteItem(int, LPNMHDR pnmh, BOOL&){
	LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)pnmh;
	LPTVITEMDATA lptvid = (LPTVITEMDATA)pnmtv->itemOld.lParam;
	delete lptvid;

	return 0;
}

LRESULT CExplorerView::OnLVDeleteItem(int, LPNMHDR pnmh, BOOL&)
{
	LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)pnmh;

	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iItem = pnmlv->iItem;
	lvi.iSubItem = 0;

	if (!m_WndListView.GetItem(&lvi))
		return 0;

	LPLVITEMDATA lplvid = (LPLVITEMDATA)lvi.lParam;
	delete lplvid;

	return 0;
}

LRESULT CExplorerView::OnNMRClick(int, LPNMHDR pnmh, BOOL&){
	POINT pt = { 0,0 };
	::GetCursorPos(&pt);
	POINT ptClient = pt;
	if (pnmh->hwndFrom != NULL)
		::ScreenToClient(pnmh->hwndFrom, &ptClient);

	if (pnmh->hwndFrom == m_WndTreeView.m_hWnd)
	{
		TVHITTESTINFO tvhti = { 0 };
		tvhti.pt = ptClient;
		m_WndTreeView.HitTest(&tvhti);
		if ((tvhti.flags & TVHT_ONITEMLABEL) != 0)
		{
			TVITEM tvi = { 0 };
			tvi.mask = TVIF_PARAM;
			tvi.hItem = tvhti.hItem;
			if (m_WndTreeView.GetItem(&tvi) != FALSE)
			{
				LPTVITEMDATA lptvid = (LPTVITEMDATA)tvi.lParam;
				if (lptvid != NULL) {
					m_ShellMgr.DoContextMenu(::GetParent(m_WndTreeView.m_hWnd), lptvid->spParentFolder, lptvid->lpi, pt);
					RefreshTreeView();
				}
			}
		}
	}
	else if (pnmh->hwndFrom == m_WndListView.m_hWnd)
	{
		LVHITTESTINFO lvhti = { 0 };
		lvhti.pt = ptClient;
		m_WndListView.HitTest(&lvhti);
		if ((lvhti.flags & LVHT_ONITEMLABEL) != 0)
		{
			LVITEM lvi = { 0 };
			lvi.mask = LVIF_PARAM;
			lvi.iItem = lvhti.iItem;
			if (m_WndListView.GetItem(&lvi) != FALSE)
			{
				LPLVITEMDATA lptvid = (LPLVITEMDATA)lvi.lParam;
				if (lptvid != NULL) {
					LPITEMIDLIST lpifq = m_ShellMgr.GetFullyQualPidl(lptvid->spParentFolder, lptvid->lpi);
					SHGetPathFromIDList(lpifq, m_Path);
					CoTaskMemFree(lpifq);
					m_ShellMgr.DoContextMenu(::GetParent(m_WndListView.m_hWnd), lptvid->spParentFolder, lptvid->lpi, pt);
					RefreshTreeView();
				}
			}
		}
	}

	return 0;
}

void CExplorerView::RefreshTreeView(){
	CComPtr<IShellFolder> spFolder;
	HRESULT hr = ::SHGetDesktopFolder(&spFolder);
	if (SUCCEEDED(hr)) {
		CWaitCursor wait;
		m_WndTreeView.DeleteAllItems();
		FillTreeView(spFolder, nullptr, TVI_ROOT);
		m_WndTreeView.Expand(m_WndTreeView.GetRootItem());
		m_WndTreeView.SelectItem(m_WndTreeView.GetRootItem());
	}
}

LRESULT CExplorerView::OnForceDeleteFile(WORD, WORD, HWND, BOOL&){
	DWORD attr = GetFileAttributes(m_Path);
	if (attr == INVALID_FILE_ATTRIBUTES) {
		AtlMessageBox(nullptr, L"Invalid file!!!", L"Error", MB_ICONERROR);
		return 0;
	}
	CString msg;
	msg.Format(L"Are you sure to delete the file: %s ?", m_Path);
	int ret = AtlMessageBox(nullptr,msg.GetString(), L"Confirm Infomration", MB_OKCANCEL);
	if (ret == IDCANCEL) {
		return 0;
	}
	if (attr & FILE_ATTRIBUTE_DIRECTORY) {
		ForceDeleteDir(m_Path);
	}
	else
		DriverHelper::ForceDeleteFile(m_Path);
	return 0;
}

void CExplorerView::ForceDeleteDir(std::wstring dir) {
	WIN32_FIND_DATA ffd;
	std::wstring fileName;
	std::wstring wildcards = L"\\*";
	std::wstring searchDir = dir + wildcards;

	HANDLE hFind = FindFirstFile(searchDir.c_str(), &ffd);
	if (hFind == INVALID_HANDLE_VALUE) {
		return;
	}

	LARGE_INTEGER fileSize;
	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (_wcsicmp(ffd.cFileName, L".") == 0 || _wcsicmp(ffd.cFileName, L"..") == 0)
				continue;
			std::wstring subDir = dir + L"\\" + ffd.cFileName;
			ForceDeleteDir(subDir);
		}
		else
		{
			fileSize.LowPart = ffd.nFileSizeLow;
			fileSize.HighPart = ffd.nFileSizeHigh;
			std::wstring file = dir + L"\\" + ffd.cFileName;
			DriverHelper::ForceDeleteFile(file.c_str());
		}
	} while (FindNextFile(hFind, &ffd));

	FindClose(hFind);
	DriverHelper::ForceDeleteFile(dir.c_str());
}
