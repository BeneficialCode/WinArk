#pragma once

#include "stdafx.h"


class ImportThunk
{
public:
	WCHAR ModuleName[MAX_PATH];
	char Name[MAX_PATH];
	DWORD_PTR VA;
	DWORD_PTR RVA;
	WORD Ordinal;
	DWORD_PTR ApiAddressVA;
	WORD Hint;
	bool Valid;
	bool Suspect;

	CTreeItem TreeItem;
	DWORD_PTR Key;

	void Invalidate() {
		Ordinal = 0;
		Hint = 0;
		Valid = false;
		Suspect = false;
		ModuleName[0] = 0;
		Name[0] = 0;
	}
};

class ImportModuleThunk
{
public:
	WCHAR ModuleName[MAX_PATH];
	std::map<DWORD_PTR, ImportThunk> ThunkMap;

	DWORD_PTR FirstThunk;

	CTreeItem TreeItem;
	DWORD_PTR Key;

	DWORD_PTR GetFirstThunk() const {
		if (ThunkMap.empty())
			return 0;
		else {
			const std::map<DWORD_PTR, ImportThunk>::const_iterator iterator = ThunkMap.begin();
			return iterator->first;
		}
	}
	bool IsValid() const {
		std::map<DWORD_PTR, ImportThunk>::const_iterator iterator = ThunkMap.begin();
		while (iterator != ThunkMap.end()) {
			if (iterator->second.Valid == false)
			{
				return false;
			}
			iterator++;
		}

		return true;
	}
};
