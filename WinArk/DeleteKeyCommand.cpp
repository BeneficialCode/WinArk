#include "stdafx.h"
#include "DeleteKeyCommand.h"
#include "RegistryManager.h"

DeleteKeyCommand::DeleteKeyCommand(const CString& path)
	: AppCommandBase(L"Delete Key"), _path(path) {
}

bool DeleteKeyCommand::Execute() {
	GUID guid;
	if (FAILED(::CoCreateGuid(&guid)))
		return false;

	WCHAR delname[64];
	::StringFromGUID2(guid, delname, 64);

	CString realpath;
	auto root = RegistryManager::Get().GetRoot(_path, realpath);
	if (!root)
		return false;

	if (!_deletedKey)
		_deletedKey.Create(RegistryManager::Get().GetDeletedKey(), delname,
			nullptr, 0, KEY_CREATE_SUB_KEY, nullptr, nullptr);

	if (!_deletedKey)
		return false;

	auto status = ::RegCopyTree(*root->GetKey(), realpath, _deletedKey);
	if (status != ERROR_SUCCESS)
		return false;

	auto name = _path.Mid(_path.ReverseFind(L'\\') + 1);
	auto parent = _path.Left(_path.GetLength() - name.GetLength());

	status = RegistryManager::Get().DeleteKey(parent, name);
	return status == ERROR_SUCCESS;
}

bool DeleteKeyCommand::Undo() {
	return false;
}