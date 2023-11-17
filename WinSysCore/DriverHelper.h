#pragma once
#include "..\Anti-Rootkit\AntiRootkit.h"


struct DriverHelper final {
	static bool LoadDriver(bool load = true);
	static bool InstallDriver(bool justCopy = false, void* pBuffer = nullptr,DWORD size = 0);
	static bool UpdateDriver(void* pBuffer,DWORD size);
	static bool IsDriverLoaded();
	static HANDLE OpenHandle(void* pObject, ACCESS_MASK);
	static HANDLE DupHandle(HANDLE hObject, ULONG pid, ACCESS_MASK access, DWORD flags = DUPLICATE_SAME_ACCESS);
	static HANDLE OpenProcess(DWORD pid, ACCESS_MASK access = PROCESS_QUERY_INFORMATION);
	//static PVOID GetObjectAddress(HANDLE hObject);
	static USHORT GetVersion();
	static USHORT GetCurrentVersion();
	static bool CloseDevice();
	static HANDLE OpenThread(DWORD tid, ACCESS_MASK access = THREAD_QUERY_INFORMATION);
	static HANDLE OpenKey(PCWSTR name, ACCESS_MASK access);
	static PULONG GetShadowServiceTable(PULONG* pServiceDescriptor);
	static PVOID GetSSDTApiAddress(ULONG number);
	static PVOID GetShadowSSDTApiAddress(ULONG number);
	static ULONG GetServiceLimit(PULONG* pTable);
	static bool InitNtServiceTable(PULONG* pTable);
	static ULONG GetProcessNotifyCount(ProcessNotifyCountData* pData);
	static ULONG GetThreadNotifyCount(ThreadNotifyCountData* pData);
	static bool EnumProcessNotify(NotifyInfo* pNotifyInfo, KernelCallbackInfo* pCallbackInfo);
	static bool EnumThreadNotify(NotifyInfo* pNotifyInfo, KernelCallbackInfo* pCallbackInfo);
	static bool EnumImageLoadNotify(NotifyInfo* pNotifyInfo, KernelCallbackInfo* pCallbackInfo);
	static ULONG GetImageNotifyCount(PULONG* pCount);
	static bool EnumPiDDBCacheTable(ULONG_PTR Address,PVOID buffer,ULONG size);
	static ULONG GetUnloadedDriverCount(PULONG* pCount);
	static bool EnumUnloadedDrivers(UnloadedDriversInfo* pInfo,PVOID buffer,ULONG size);
	static ULONG GetPiDDBCacheDataSize(ULONG_PTR Address);
	static ULONG GetUnloadedDriverDataSize(UnloadedDriversInfo* pInfo);
	static bool EnumObCallbackNotify(ULONG offset,ObCallbackInfo* pCallbackInfo,ULONG size);
	static LONG GetObCallbackCount(ULONG offset);
	static ULONG GetCmCallbackCount(PULONG* pCount);
	static ULONG GetIoTimerCount(PULONG* pCount);
	static bool EnumCmCallbackNotify(PVOID pHeadList, CmCallbackInfo* pCallbackInfo,ULONG size);
	static bool GetDriverObjectRoutines(PCWSTR name, PVOID pRoutines);
	static bool SetImageLoadNotify();
	static bool RemoveImageLoadNotify();
	static bool RemoveNotify(NotifyData* pData);
	static bool EnableDbgSys(DbgSysCoreInfo* pInfo);
	static bool DisableDbgSys();
	static bool EnumKernelTimer(KernelTimerData* pData,DpcTimerInfo* pInfo,SIZE_T size);
	static bool EnumIoTimer(IoTimerData* pData, IoTimerInfo* pInfo, SIZE_T size);
	static bool EnumMiniFilterOperations(MiniFilterData* pData, SIZE_T dataSize,OperationInfo* pInfo, SIZE_T size);
	static bool RemoveMiniFilter(MiniFilterData* pData, SIZE_T dataSize);

	static ULONG GetKernelTimerCount(KernelTimerData* pData);
	static ULONG GetVadCount(VadData* pData);

	static bool Bypass(DWORD flag);
	static bool Unbypass(DWORD flag);

	static PVOID GetEprocess(HANDLE pid);
	static bool DumpSysModule(PCWSTR sysPath, void* imageBase, ULONG imageSize);

	static bool DumpKernelMem(DumpMemData* pData, void* pInfo);

	static ULONG GetWinExtHostsCount(PVOID pList);
	static bool EnumWinExtHosts(PVOID pList, WinExtHostInfo* pInfo, ULONG size);

	static bool EnumExtTable(ExtHostData* pData, void* pInfo, ULONG size);

	static bool DetectInlineHook(KInlineData* pData,KernelInlineHookData* pInfo, ULONG size);
	static ULONG GetKernelInlineHookCount(ULONG_PTR base);

	static bool ForceDeleteFile(const wchar_t* fileName);

	static bool DisableDriverLoad(CiSymbols* pSym);
	static bool EnableDriverLoad();

	static bool StartLogDriverHash(CiSymbols* pSym);
	static bool StopLogDriverHash();

	static bool GetLegoNotifyRoutine(void* pPspLegoNotifyRoutine, void* pRoutine);
	static bool RemoveObCallback(ULONG_PTR addr);
	static bool DisableObCallback(ULONG_PTR addr);
	static bool EnableObCallback(ULONG_PTR addr);
private:
	static bool OpenDevice();

	static HANDLE _hDevice;
};