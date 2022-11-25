#pragma once
#include "Common.h"
#include "khook.h"
#include "kDbgSys.h"
#include "Teb.h"




class kDbgUtil final {
public:
	static PEX_RUNDOWN_REF GetProcessRundownProtect(PEPROCESS Process);
	static PULONG GetProcessCrossThreadFlags(PEPROCESS Process);
	static PPEB GetProcessPeb(PEPROCESS Process);
	static PDEBUG_OBJECT* GetProcessDebugPort(PEPROCESS Process);
	static VOID* GetProcessWow64Process(PEPROCESS Process);
	static PULONG GetProcessFlags(PEPROCESS Process);
	static VOID* GetProcessSectionBaseAddress(PEPROCESS Process);
	static VOID* GetProcessSectionObject(PEPROCESS Process);
	static VOID* GetProcessUniqueProcessId(PEPROCESS Process);
	static PULONG GetThreadCrossThreadFlags(PETHREAD Ethread);
	static PEX_RUNDOWN_REF GetThreadRundownProtect(PETHREAD Thread);

	// 初始化调试函数指针
	static bool InitDbgSys(DbgSysCoreInfo* info);
	static bool ExitDbgSys();
	static bool HookDbgSys();
	static bool UnhookDbgSys();

	static inline TcbGlobalOffsets _tcbOffsets;
	static inline EThreadGlobalOffsets _ethreadOffsets;
	static inline EProcessGlobalOffsets _eprocessOffsets;
	
	using PNtCreateDebugObject = decltype(&NtCreateDebugObject);
	static inline PNtCreateDebugObject g_pNtCreateDebugObject{ nullptr };
	static inline khook _hookNtCreateDebugObject;

	using PNtDebugActiveProcess = decltype(&NtDebugActiveProcess);
	static inline PNtDebugActiveProcess g_pNtDebugActiveProcess{ nullptr };

	using PDbgkpPostFakeProcessCreateMessages = decltype(&DbgkpPostFakeProcessCreateMessages);
	static inline PDbgkpPostFakeProcessCreateMessages g_pDbgkpPostFakeProcessCreateMessages{ nullptr };

	using PDbgkpSetProcessDebugObject = decltype(&DbgkpSetProcessDebugObject);
	static inline PDbgkpSetProcessDebugObject g_pDbgkpSetProcessDebugObject{ nullptr };

	using PDbgkpPostFakeThreadMessages = decltype(&DbgkpPostFakeThreadMessages);
	static inline PDbgkpPostFakeThreadMessages g_pDbgkpPostFakeThreadMessages{ nullptr };

	using PDbgkpPostModuleMessages = decltype(&DbgkpPostModuleMessages);
	static inline PDbgkpPostModuleMessages g_pDbgkpPostModuleMessages{ nullptr };


	using PPsGetNextProcessThread = PETHREAD(*)(PEPROCESS Process, PETHREAD Thread);
	static inline PPsGetNextProcessThread g_pPsGetNextProcessThread{ nullptr };

	using PDbgkpWakeTarget = decltype(&DbgkpWakeTarget);
	static inline PDbgkpWakeTarget g_pDbgkpWakeTarget{ nullptr };
	using PDbgkpMarkProcessPeb = decltype(&DbgkpMarkProcessPeb);
	static inline PDbgkpMarkProcessPeb g_pDbgkpMarkProcessPeb{ nullptr };

	static inline bool _first = true;
};