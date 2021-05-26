#pragma once
#include "AppCommandBase.h"

class CreateNewKeyCommand : public AppCommandBase {
public:
	CreateNewKeyCommand(const CString& parentPath, PCWSTR keyName);

	bool Execute() override;
	bool Undo() override;

private:
	CString _keyName;
	CString _parentName;
};

