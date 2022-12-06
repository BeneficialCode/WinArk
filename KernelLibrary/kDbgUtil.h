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
	static PVOID GetProcessWow64Process(PEPROCESS Process);
	static PULONG GetProcessFlags(PEPROCESS Process);
	static VOID* GetProcessSectionBaseAddress(PEPROCESS Process);
	static VOID* GetProcessSectionObject(PEPROCESS Process);
	static VOID* GetProcessUniqueProcessId(PEPROCESS Process);
	static PULONG GetThreadCrossThreadFlags(PETHREAD Ethread);
	static PEX_RUNDOWN_REF GetThreadRundownProtect(PETHREAD Thread);
	static PPEB_LDR_DATA GetPEBLdr(PPEB Peb);

	// 初始化调试函数指针
	static bool InitDbgSys(DbgSysCoreInfo* info);
	static bool ExitDbgSys();
	static bool HookDbgSys();
	static bool UnhookDbgSys();

	static inline TcbOffsets _tcbOffsets;
	static inline EThreadOffsets _ethreadOffsets;
	static inline EProcessOffsets _eprocessOffsets;
	static inline PebOffsets _pebOffsets;
	
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


	using PPsGetNextProcessThread = PETHREAD(NTAPI*)(PEPROCESS Process, PETHREAD Thread);
	static inline PPsGetNextProcessThread g_pPsGetNextProcessThread{ nullptr };

	using PDbgkpWakeTarget = decltype(&DbgkpWakeTarget);
	static inline PDbgkpWakeTarget g_pDbgkpWakeTarget{ nullptr };
	using PDbgkpMarkProcessPeb = decltype(&DbgkpMarkProcessPeb);
	static inline PDbgkpMarkProcessPeb g_pDbgkpMarkProcessPeb{ nullptr };

	using PMmGetFileNameForAddress = NTSTATUS(NTAPI*)(PVOID ProcessVa, PUNICODE_STRING FileName);
	static inline PMmGetFileNameForAddress g_pMmGetFileNameForAddress{ nullptr };

	using PDbgkpQueueMessage = decltype(&DbgkpQueueMessage);
	static inline PDbgkpQueueMessage g_pDbgkpQueueMessage{ nullptr };

	using PDbgkpSendApiMessage = decltype(&DbgkpSendApiMessage);
	static inline PDbgkpSendApiMessage g_pDbgkpSendApiMessage{ nullptr };

	static inline bool _first = true;
};