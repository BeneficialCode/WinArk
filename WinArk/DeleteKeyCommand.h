#pragma once

#include "AppCommandBase.h"

class DeleteKeyCommand : public AppCommandBase {
public:
	DeleteKeyCommand(const CString& path);

	bool Execute() override;
	bool Undo() override;

private:
	CString _path;
	CRegKey _deletedKey;
};