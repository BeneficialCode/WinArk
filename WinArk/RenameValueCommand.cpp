#include "stdafx.h"
#include "RenameValueCommand.h"
#include "RegistryManager.h"

RenameValueCommand::RenameValueCommand(const CString& path, const CString& oldName, const CString& newName)
	: AppCommandBase(L"Rename Value"), _path(path), _oldName(oldName), _newName(newName) {
}

bool RenameValueCommand::Execute() {
	auto status = RegistryManager::Get().RenameValue(_path, _oldName, _newName);
	if (status == ERROR_SUCCESS) {
		auto temp(_oldName);
		_oldName = _newName;
		_newName = temp;
		return true;
	}

	return false;
}