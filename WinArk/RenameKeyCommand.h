#pragma once

#include "AppCommandBase.h"

class RenameKeyCommand : public AppCommandBase {
public:
	RenameKeyCommand(const CString& path, const CString& newname);

	bool Execute() override;
	bool Undo() override {
		return Execute();
	}

private:
	CString _path, _name;
};
