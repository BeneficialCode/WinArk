#include "stdafx.h"
#include "CreateNewKeyCommand.h"
#include "RegistryManager.h"

CreateNewKeyCommand::CreateNewKeyCommand(const CString& parentPath, PCWSTR keyName)
	: AppCommandBase(L"New Key"), _parentName(parentPath), _keyName(keyName) {
}

bool CreateNewKeyCommand::Execute() {
	auto status = RegistryManager::Get().CreateKey(_parentName, _keyName);
	return status == ERROR_SUCCESS;
}

bool CreateNewKeyCommand::Undo() {
	return RegistryManager::Get().DeleteKey(_parentName, _keyName) == ERROR_SUCCESS;
}

