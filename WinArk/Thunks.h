#pragma once

#include "stdafx.h"


class ImportThunk
{
public:
	WCHAR m_ModuleName[MAX_PATH];
	char m_Name[MAX_PATH];
	DWORD_PTR m_VA;
	DWORD_PTR m_RVA;
	WORD m_Ordinal;
	DWORD_PTR m_ApiAddressVA;
	WORD m_Hint;
	bool m_Valid;
	bool m_Suspect;

	CTreeItem m_TreeItem;
	DWORD_PTR m_Key;

	void Invalidate() {
		m_Ordinal = 0;
		m_Hint = 0;
		m_Valid = false;
		m_Suspect = false;
		m_ModuleName[0] = 0;
		m_Name[0] = 0;
	}
};

class ImportModuleThunk
{
public:
	WCHAR m_ModuleName[MAX_PATH];
	std::map<DWORD_PTR, ImportThunk> m_ThunkMap;

	DWORD_PTR m_FirstThunk;

	CTreeItem m_TreeItem;
	DWORD_PTR m_Key;

	DWORD_PTR GetFirstThunk() const {
		if (m_ThunkMap.empty())
			return 0;
		else {
			const std::map<DWORD_PTR, ImportThunk>::const_iterator iterator = m_ThunkMap.begin();
			return iterator->first;
		}
	}
	bool IsValid() const {
		std::map<DWORD_PTR, ImportThunk>::const_iterator iterator = m_ThunkMap.begin();
		while (iterator != m_ThunkMap.end()) {
			if (iterator->second.m_Valid == false)
			{
				return false;
			}
			iterator++;
		}

		return true;
	}
};
