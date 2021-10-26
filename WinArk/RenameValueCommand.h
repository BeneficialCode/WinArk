#pragma once

#include "AppCommandBase.h"

class RenameValueCommand : public RegAppCommandBase<RenameValueCommand> {
public:
	RenameValueCommand(PCWSTR path, PCWSTR name, PCWSTR newName, AppCommandCallback<RenameValueCommand> cb = nullptr);

	bool Execute() override;
	bool Undo() override;

	const CString& GetNewName() const;

	CString GetCommandName() const;

private:
	CString _path, _oldName, _newName;
};