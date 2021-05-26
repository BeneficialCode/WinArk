#pragma once

#include "AppCommandBase.h"

struct ITreeOperations {
	virtual bool SelectNode(TreeNodeBase* parent, PCWSTR name) = 0;
};

struct IMainApp {
	virtual bool AddCommand(std::shared_ptr<AppCommandBase> cmd, bool execute = true) = 0;
	virtual void ShowCommandError(PCWSTR message = nullptr) = 0;
	virtual bool IsAllowModify() const = 0;
	virtual UINT TrackPopupMenu(CMenuHandle menu, int x, int y) = 0;
	virtual CUpdateUIBase* GetUIUpdate() = 0;
};