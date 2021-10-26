#pragma once
#include "AppCommandBase.h"

class CreateKeyCommand : public RegAppCommandBase<CreateKeyCommand> {
public:
	CreateKeyCommand(PCWSTR path, PCWSTR name, AppCommandCallback<CreateKeyCommand> cb = nullptr);

	bool Execute() override;
	bool Undo() override;
};

