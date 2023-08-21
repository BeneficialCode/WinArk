#pragma once
/*
MIT License

Copyright (c) 2021 Pavel Yosifovich

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "TaskHelper.h"
#include "VirtualListView.h"
#include "CustomSplitterWindow.h"

class CTaskSchedView :
	public CWindowImpl<CTaskSchedView>,
	public CAutoUpdateUI<CTaskSchedView>,
	public CVirtualListView<CTaskSchedView> {
public:
	DECLARE_WND_CLASS(nullptr);

	const UINT WM_BUILD_TASK_SCHED_TREE = WM_APP + 14;

	CString GetColumnText(HWND, int row, int col) const;
	CString GetDetailsColumnText(HWND, int row, int col) const;

	int GetRowImage(HWND, int row, int col) const;
	void DoSort(const SortInfo* si);
	void DoSortDetails(const SortInfo* si);
	BOOL OnRightClickList(HWND, int row, int col, const POINT&);

	BEGIN_MSG_MAP(CTaskSchedView)
		NOTIFY_CODE_HANDLER(TVN_SELCHANGED,OnTreeSelChanged)
		NOTIFY_HANDLER(TaskListId,LVN_ITEMCHANGED,OnListItemChanged)
		MESSAGE_HANDLER(WM_CREATE,OnCreate)
		MESSAGE_HANDLER(WM_SIZE,OnSize)
		MESSAGE_HANDLER(WM_BUILD_TASK_SCHED_TREE,OnBuildTree)
		COMMAND_ID_HANDLER(ID_TASK_ENABLE,OnEnableDisableTask)
		COMMAND_ID_HANDLER(ID_TASK_RUN,OnRunTask)
		COMMAND_ID_HANDLER(ID_TASK_STOP,OnStopTask)
		COMMAND_ID_HANDLER(ID_TASK_DELETE,OnDeleteTask)
		COMMAND_ID_HANDLER(ID_VIEW_REFRESH,OnViewRefresh)
		CHAIN_MSG_MAP(CVirtualListView<CTaskSchedView>)
	END_MSG_MAP()

	enum {
		TaskListId = 129,
		DetailsListId
	};

	enum class IconIndex {
		Generic, Yes, No, Action, Trigger,
	};
	struct DetailsItem {
		CString Name;
		CString Details;
		IconIndex Image{ IconIndex::Generic };
	};

	LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnBuildTree(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnTreeSelChanged(int, LPNMHDR, BOOL&);
	LRESULT OnEnableDisableTask(WORD, WORD, HWND, BOOL&);
	LRESULT OnListItemChanged(int, LPNMHDR, BOOL&);
	LRESULT OnViewRefresh(WORD, WORD, HWND, BOOL&);
	LRESULT OnRunTask(WORD, WORD, HWND, BOOL&);
	LRESULT OnStopTask(WORD, WORD, HWND, BOOL&);
	LRESULT OnDeleteTask(WORD, WORD, HWND, BOOL&);
public:

	CCustomSplitterWindow m_MainSplitter;

private:
	enum class ColumnType {
		Name,Status,Triggers,Created,Path,
		Author,NextRun,LastRun,RunResults,
		RunningPid,InstanceGuid,CurrentAction
	};

	enum class NodeType {
		None,
		Folder,TaskSched,RunningTasks,AllTasks
	};
	struct TaskItem {
		CString Name;
		CString Path;
		mutable DATE NextRun{ 0 };
		CComPtr<IRegisteredTask> spTask;
		CComPtr<IRunningTask> spRunningTask;
	};

	HRESULT BuildTaskTree();
	HRESULT BuildTaskTree(ITaskFolder* folder, CTreeItem root);
	CString GetFolderPath(HTREEITEM hItem) const;
	TASK_STATE GetTaskState(const TaskItem& item) const;
	CString GetTaskAuthor(const TaskItem& item) const;
	CString GetTaskCreateDate(const TaskItem& item) const;
	void RefreshList();
	void UpdateDetails();
	int UpdateItemDetails(const TaskItem& item);

	void EnumTasks();
	void EnumAllTasks();
	void EnumRunningTasks();
	void AddTasks(ITaskFolder* folder);
	void SwitchColumns();
	void UpdateUI();


	CCustomHorSplitterWindow m_ListSplitter;
	CListViewCtrl m_List;
	CTreeViewCtrlEx m_Tree;
	CListViewCtrl m_Details;
	TaskSchedSvc m_TaskSvc;
	std::vector<TaskItem> m_Items;
	std::vector<DetailsItem> m_ItemDetails;
	CComPtr<ITaskFolder> m_spCurrentFolder;
	NodeType m_CurrentNodeType{ NodeType::None };
	ColumnsState m_TasksColumns, m_RunningTasksColumns;
	int m_CurrentSelectedItem{ -1 };
};