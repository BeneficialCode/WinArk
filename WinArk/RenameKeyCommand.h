#pragma once

#include "AppCommandBase.h"

class RenameKeyCommand : public RegAppCommandBase<RenameKeyCommand> {
public:
	RenameKeyCommand(PCWSTR path, PCWSTR name, PCWSTR newName, AppCommandCallback<RenameKeyCommand> cb = nullptr);

	bool Execute() override;
	bool Undo() override;

	const CString& GetNewName() const;

	CString GetCommandName() const override;

private:
	CString _newName;
};
