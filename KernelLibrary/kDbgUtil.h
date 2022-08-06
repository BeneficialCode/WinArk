#pragma once

struct EProcessGlobalOffsets {
	ULONG RundownProtect;		// PEX_RUNDOWN_REF
	ULONG CrossThreadFlags;		// ULONG
	ULONG Peb;					// 
	ULONG DebugPort;
	ULONG Wow64Process;
	ULONG Flags;
	ULONG SectionBaseAddress;
	ULONG SectionObject;
	ULONG UniqueProcessId;
};

struct EThreadGlobalOffsets {
	ULONG CrossThreadFlags;
	ULONG SystemThread;
	ULONG Cid;
	ULONG ClonedThread;
	ULONG RundownProtect;
	ULONG ThreadInserted;
	ULONG Tcb;
	ULONG StartAddress;
};

struct TcbGlobalOffsets {
	ULONG Teb;
	ULONG ApcStateIndex;
};

class kDbgUtil final {
public:
	static PEX_RUNDOWN_REF GetProcessRundownProtect(PEPROCESS Process);
	/*static PEX_RUNDOWN_REF GetProcessCrossThreadFlags(PEPROCESS Process);
	static PEX_RUNDOWN_REF GetProcessPeb(PEPROCESS Process);
	static PEX_RUNDOWN_REF GetProcessDebugPort(PEPROCESS Process);
	static PEX_RUNDOWN_REF GetProcessWow64Process(PEPROCESS Process);
	static PEX_RUNDOWN_REF GetProcessFlags(PEPROCESS Process);
	static PEX_RUNDOWN_REF GetProcessSectionBaseAddress(PEPROCESS Process);
	static PEX_RUNDOWN_REF GetProcessSectionObject(PEPROCESS Process);
	static PEX_RUNDOWN_REF GetProcessUniqueProcessId(PEPROCESS Process);*/



	static inline TcbGlobalOffsets _tcbOffsets;
	static inline EThreadGlobalOffsets _ethreadOffsets;
	static inline EProcessGlobalOffsets _eprocessOffsets;
};