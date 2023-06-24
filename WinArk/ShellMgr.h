#pragma once

#include <ShlObj.h>
#include <atlctrls.h>

// https://learn.microsoft.com/en-us/windows/win32/shell/namespace-intro
class CShellItemIDList {
public:
	LPITEMIDLIST m_pidl;

	CShellItemIDList(LPITEMIDLIST pidl = nullptr) :m_pidl(pidl) {
	}

	~CShellItemIDList() {
		::CoTaskMemFree(m_pidl);
	}

	void Attach(LPITEMIDLIST pidl) {
		::CoTaskMemFree(m_pidl);
		m_pidl = pidl;
	}

	LPITEMIDLIST Detach() {
		LPITEMIDLIST pidl = m_pidl;
		m_pidl = nullptr;
		return pidl;
	}

	bool IsNull() const {
		return (m_pidl == nullptr);
	}

	CShellItemIDList& operator= (LPITEMIDLIST pidl) {
		Attach(pidl);
		return *this;
	}

	LPITEMIDLIST* operator &() {
		return &m_pidl;
	}

	operator LPITEMIDLIST() {
		return m_pidl;
	}

	operator LPCTSTR() {
		return (LPCTSTR)m_pidl;
	}

	operator LPTSTR()
	{
		return (LPTSTR)m_pidl;
	}

	void CreateEmpty(UINT cbSize) {
		::CoTaskMemFree(m_pidl);
		m_pidl = (LPITEMIDLIST)::CoTaskMemAlloc(cbSize);
		ATLASSERT(m_pidl != nullptr);
		if (m_pidl != nullptr)
			memset(m_pidl, 0, cbSize);
	}
};

typedef struct _LVItemData {
	_LVItemData() :Attribs(0) {
	}
	CComPtr<IShellFolder> spParentFolder;
	CShellItemIDList lpi;
	ULONG Attribs;
}LVITEMDATA,*LPLVITEMDATA;

typedef struct _TVItemData {
	_TVItemData(){}

	CComPtr<IShellFolder> spParentFolder;

	CShellItemIDList lpi;
	CShellItemIDList lpifq;
}TVITEMDATA,*LPTVITEMDATA;

class CShellMgr {
public:
	const int ID_DELETE_FILE = 32444;
	int GetIconIndex(LPITEMIDLIST lpi, UINT flags);
	void GetNormalAndSelectedIcons(LPITEMIDLIST lpifq, LPTVITEM lptvitem);
	LPITEMIDLIST ConcatPidls(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

	BOOL GetName(LPSHELLFOLDER lpsf, LPITEMIDLIST lpi, DWORD flags, LPTSTR lpFriendlyName);
	LPITEMIDLIST Next(LPCITEMIDLIST pidl);
	UINT GetSize(LPCITEMIDLIST pidl);
	LPITEMIDLIST CopyITEMID(LPITEMIDLIST lpi);

	LPITEMIDLIST GetFullyQualPidl(LPSHELLFOLDER lpsf, LPITEMIDLIST lpi);
	BOOL DoContextMenu(HWND hWnd, LPSHELLFOLDER lpsfParent, LPITEMIDLIST lpi,
		POINT point);
};