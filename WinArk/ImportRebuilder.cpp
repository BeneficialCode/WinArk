#include "stdafx.h"
#include "ImportRebuilder.h"

bool ImportRebuilder::RebuildImportTable(const WCHAR* newFilePath, 
	std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap){
	bool ret = false;
	std::map<DWORD_PTR, ImportModuleThunk> copyModule;
	copyModule.insert(moduleThunkMap.begin(), moduleThunkMap.end());

	if (IsValid()) {
		SetDefaultFileAligment();

		ret = BuildNewImportTable(copyModule);
		if (ret) {
			AlignAllSectionHeaders();

		}
	}
	
	return ret;
}

bool ImportRebuilder::BuildNewImportTable(std::map<DWORD_PTR, ImportModuleThunk>& moduleThunkMap) {

	return true;
}