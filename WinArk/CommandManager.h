#pragma once
struct AppCommandBase;

class CommandManager {
public:
	bool CanUndo() const;
	bool CanRedo() const;

	bool AddCommand(std::shared_ptr<AppCommandBase> command, bool execute = true);
	bool Undo();
	bool Redo();
	void Clear();

	AppCommandBase* GetUndoCommand() const;
	AppCommandBase* GetRedoCommand() const;

private:
	std::vector<std::shared_ptr<AppCommandBase>> _undoList;
	std::vector<std::shared_ptr<AppCommandBase>> _redoList;
};