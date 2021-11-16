#pragma once

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
	static PULONG GetKiServiceTable();
	static PVOID GetSSDTApiAddress(ULONG number);
	static PVOID GetShadowSSDTApiAddress(ULONG number);
	static PULONG GetShadowServiceTable();
	static ULONG GetShadowServiceLimit();

private:
	static bool OpenDevice();

	static HANDLE _hDevice;
};