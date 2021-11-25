#include "pch.h"
#include "DriverHelper.h"
#include "SecurityHelper.h"

HANDLE DriverHelper::_hDevice;



bool DriverHelper::LoadDriver(bool load) {
	if (_hDevice) {
		::CloseHandle(_hDevice);
		_hDevice = INVALID_HANDLE_VALUE;
	}
	wil::unique_schandle hScm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
	if (!hScm)
		return false;

	wil::unique_schandle hService(::OpenService(hScm.get(), L"AntiRootkit", SERVICE_ALL_ACCESS));
	if (!hService)
		return false;

	SERVICE_STATUS status;
	bool success = true;
	DWORD targetState;
	::QueryServiceStatus(hService.get(), &status);
	if (load && status.dwCurrentState != (targetState = SERVICE_RUNNING))
		success = ::StartService(hService.get(), 0, nullptr);
	else if (!load && status.dwCurrentState != (targetState = SERVICE_STOPPED))
		success = ::ControlService(hService.get(), SERVICE_CONTROL_STOP, &status);
	else
		return true;

	if (!success)
		return false;

	for (int i = 0; i < 20; i++) {
		::QueryServiceStatus(hService.get(), &status);
		if (status.dwCurrentState == targetState)
			return true;
		::Sleep(200);
	}

	return false;
}

bool DriverHelper::InstallDriver(bool justCopy,void* pBuffer,DWORD size) {
	if (!SecurityHelper::IsRunningElevated())
		return false;
	
	WCHAR path[MAX_PATH];
	::GetSystemDirectory(path, MAX_PATH);
	::wcscat_s(path, L"\\Drivers\\AntiRootkit.sys");
	wil::unique_hfile hFile(::CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_SYSTEM, nullptr));
	if (!hFile)
		return false;

	DWORD bytes = 0;
	::WriteFile(hFile.get(), pBuffer, size, &bytes, nullptr);
	if (bytes != size)
		return false;

	if (justCopy)
		return true;

	wil::unique_schandle hScm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
	if (!hScm)
		return false;

	wil::unique_schandle hService(::CreateService(hScm.get(), L"AntiRootkit", nullptr, SERVICE_ALL_ACCESS, 
		SERVICE_KERNEL_DRIVER,
		SERVICE_AUTO_START, // driver is loaded automatically by the SCM when Windows subsystem is available
		SERVICE_ERROR_NORMAL, // the error is logged to the event log service
		path, // the full path to the executable to run for the service
		nullptr, nullptr, nullptr, nullptr, nullptr));
	if (ERROR_SERVICE_EXISTS == GetLastError()) {
		hService.reset(::OpenService(hScm.get(), L"AntiRootkit", SERVICE_ALL_ACCESS));
		if (!hService)
			return false;
		if (!DeleteService(hService.get()))
			return false;
		hService.reset(::CreateService(hScm.get(), L"AntiRootkit", nullptr, SERVICE_ALL_ACCESS,
			SERVICE_KERNEL_DRIVER,
			SERVICE_AUTO_START, // driver is loaded automatically by the SCM when Windows subsystem is available
			SERVICE_ERROR_NORMAL, // the error is logged to the event log service
			path, // the full path to the executable to run for the service
			nullptr, nullptr, nullptr, nullptr, nullptr));
	}
	auto success = hService != nullptr;
	return success;
}

bool DriverHelper::UpdateDriver(void* pBuffer,DWORD size) {
	if (!LoadDriver(false))
		return false;
	if (!InstallDriver(true,pBuffer,size))
		return false;
	if (!LoadDriver())
		return false;

	return true;
}

HANDLE DriverHelper::OpenHandle(void* pObject, ACCESS_MASK access) {
	if (!OpenDevice())
		return nullptr;

	OpenObjectData data;
	data.Access = access;
	data.Address = pObject;

	DWORD bytes;
	HANDLE hObject;
	return ::DeviceIoControl(_hDevice, IOCTL_ARK_OPEN_OBJECT, &data, sizeof(data), &hObject, sizeof(hObject), &bytes, nullptr) ? hObject : nullptr;
}

HANDLE DriverHelper::OpenProcess(DWORD pid, ACCESS_MASK access) {
	if (OpenDevice()) {
		OpenProcessThreadData data;
		data.AccessMask = access;
		data.Id = pid;
		HANDLE hProcess;
		DWORD bytes;

		return ::DeviceIoControl(_hDevice, IOCTL_ARK_OPEN_PROCESS, &data, sizeof(data),
			&hProcess, sizeof(hProcess), &bytes, nullptr) ? hProcess : nullptr;
	}

	return ::OpenProcess(access, FALSE, pid);
}

bool DriverHelper::OpenDevice() {
	if (!_hDevice) {
		_hDevice = ::CreateFile(L"\\\\.\\AntiRootkit", GENERIC_WRITE | GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
			OPEN_EXISTING, 0, nullptr);
		if (_hDevice == INVALID_HANDLE_VALUE) {
			_hDevice = nullptr;
			return false;
		}
	}
	return true;
}

bool DriverHelper::IsDriverLoaded() {
	wil::unique_schandle hScm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE));
	if (!hScm)
		return false;

	wil::unique_schandle hService(::OpenService(hScm.get(), L"AntiRootkit", SERVICE_QUERY_STATUS));
	if (!hService)
		return false;

	SERVICE_STATUS status;
	if (!::QueryServiceStatus(hService.get(), &status))
		return false;

	return status.dwCurrentState == SERVICE_RUNNING;
}

bool DriverHelper::CloseDevice() {
	if (_hDevice) {
		::CloseHandle(_hDevice);
		_hDevice = nullptr;
	}
	return true;
}

USHORT DriverHelper::GetVersion() {
	USHORT version = 0;
	if (!OpenDevice())
		return 0;

	DWORD bytes;
	::DeviceIoControl(_hDevice, IOCTL_ARK_GET_VERSION, nullptr, 0, &version, sizeof(version), &bytes, nullptr);
	return version;
}


USHORT DriverHelper::GetCurrentVersion() {
	return DRIVER_CURRENT_VERSION;
}

HANDLE DriverHelper::DupHandle(HANDLE hObject, ULONG pid, ACCESS_MASK access, DWORD flags) {
	HANDLE hTarget = nullptr;
	if (OpenDevice()) {
		DupHandleData data;
		data.AccessMask = access;
		data.Handle = HandleToUlong(hObject);
		data.SourcePid = pid;
		data.Flags = flags;

		DWORD bytes;
		::DeviceIoControl(_hDevice, IOCTL_ARK_DUP_HANDLE, &data, sizeof(data),
			&hTarget, sizeof(hTarget), &bytes, nullptr);
	}
	if (!hTarget) {
		wil::unique_handle hProcess(OpenProcess(pid, PROCESS_DUP_HANDLE));
		if (hProcess)
			::DuplicateHandle(hObject, hProcess.get(), ::GetCurrentProcess(), &hTarget, access, FALSE, flags);
	}
	return hTarget;
}

HANDLE DriverHelper::OpenThread(DWORD tid, ACCESS_MASK access) {
	if (OpenDevice()) {
		OpenProcessThreadData data;
		data.AccessMask = access;
		data.Id = tid;
		HANDLE hThread;
		DWORD bytes;

		return ::DeviceIoControl(_hDevice, IOCTL_ARK_OPEN_THREAD, &data, sizeof(data),
			&hThread, sizeof(hThread), &bytes, nullptr) ? hThread : nullptr;
	}
	return ::OpenThread(access, FALSE, tid);
}

HANDLE DriverHelper::OpenKey(PCWSTR name,ACCESS_MASK access) {
	if (!OpenDevice())
		return nullptr;

	auto len = (ULONG)::wcslen(name);
	DWORD size = len * sizeof(WCHAR) + sizeof(KeyData);
	auto buffer = std::make_unique<BYTE[]>(size);
	if (!buffer)
		return nullptr;

	auto data = reinterpret_cast<KeyData*>(buffer.get());
	data->Access = access;
	data->Length = len;
	::wcscpy_s(data->Name, len + 1, name);

	DWORD bytes;
	HANDLE hObject = nullptr;
	::DeviceIoControl(_hDevice, IOCTL_ARK_OPEN_KEY, data, size, &hObject, sizeof(hObject), &bytes, nullptr);

	return hObject;
}

PULONG DriverHelper::GetKiServiceTable() {
	PULONG address = 0;
	if (!OpenDevice())
		return 0;

	DWORD bytes;
	::DeviceIoControl(_hDevice, IOCTL_ARK_GET_SERVICE_TABLE, nullptr, 0, &address, sizeof(address), &bytes, nullptr);
	return address;
}

PULONG DriverHelper::GetShadowServiceTable() {
	PULONG address = 0;
	if (!OpenDevice())
		return 0;

	DWORD bytes;
	::DeviceIoControl(_hDevice, IOCTL_ARK_GET_SHADOW_SERVICE_TABLE, nullptr, 0, &address, sizeof(address), &bytes, nullptr);
	return address;
}

PVOID DriverHelper::GetSSDTApiAddress(ULONG number) {
	if (!OpenDevice())
		return nullptr;

	PVOID address = nullptr;
	DWORD bytes;
	::DeviceIoControl(_hDevice, IOCTL_ARK_GET_SSDT_API_ADDR, &number, sizeof(number),
		&address, sizeof(address), &bytes, nullptr);
	return address;
}

PVOID DriverHelper::GetShadowSSDTApiAddress(ULONG number) {
	if (!OpenDevice())
		return nullptr;

	PVOID address = nullptr;
	DWORD bytes;
	::DeviceIoControl(_hDevice, IOCTL_ARK_GET_SHADOW_SSDT_API_ADDR, &number, sizeof(number),
		&address, sizeof(address), &bytes, nullptr);
	return address;
}

ULONG DriverHelper::GetShadowServiceLimit() {
	if (!OpenDevice())
		return 0;

	ULONG limit = 0;
	DWORD bytes;
	::DeviceIoControl(_hDevice, IOCTL_ARK_GET_SHADOW_SERVICE_LIMIT, nullptr, 0, 
		&limit, sizeof(limit),&bytes, nullptr);
	return limit;
}

ULONG DriverHelper::GetProcessNotifyCount(ProcessNotifyCountData *pData) {
	if (!OpenDevice())
		return 0;

	ULONG count = 0;
	DWORD bytes;
	::DeviceIoControl(_hDevice, IOCTL_ARK_GET_PROCESS_NOTIFY_COUNT, pData, sizeof(ProcessNotifyCountData),
		&count, sizeof(count), &bytes, nullptr);
	return count;
}

bool DriverHelper::EnumProcessNotify(NotifyInfo* pNotifyInfo, KernelCallbackInfo* pCallbackInfo) {
	if (!OpenDevice())
		return false;

	DWORD bytes;
	DWORD size = pCallbackInfo->Count * sizeof(pCallbackInfo->Address);
	::DeviceIoControl(_hDevice, IOCTL_ARK_ENUM_PROCESS_NOTIFY, pNotifyInfo, sizeof(NotifyInfo),
		pCallbackInfo, size, &bytes, nullptr);
	return true;
}