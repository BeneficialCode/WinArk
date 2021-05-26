#include "stdafx.h"
#include "resource.h"
#include "RenameKeyCommand.h"
#include "RegistryManager.h"
#include "RegHelper.h"

RenameKeyCommand::RenameKeyCommand(const CString& path, const CString& newname)
	: AppCommandBase(L"Rename Key"), _path(path), _name(newname) {
}

bool RenameKeyCommand::Execute() {
	auto index = _path.ReverseFind(L'\\');
	CString parent = _path.Left(index);
	auto success = RegistryManager::Get().RenameKey(_path, _name);
	if (success == ERROR_SUCCESS) {
		auto oldname = _name;
		_name = _path.Mid(index + 1);
		_path = parent + L"\\" + oldname;
	}
	return success == ERROR_SUCCESS;
}