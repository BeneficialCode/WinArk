#pragma once

#include "AppCommandBase.h"

class RenameValueCommand : public AppCommandBase {
public:
	RenameValueCommand(const CString& path, const CString& oldName, const CString& newName);

	bool Execute() override;
	bool Undo() override {
		return Execute();
	}

private:
	CString _path, _oldName, _newName;
};