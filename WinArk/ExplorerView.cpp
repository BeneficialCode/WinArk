#include "stdafx.h"
#include "ExplorerView.h"


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
	HRESULT hr = lpsf->EnumObjects(m_hWnd, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &spIDList);
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