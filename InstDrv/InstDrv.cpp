#include "InstDrv.h"


BOOL InstDrv::GetSvcHandle(PCHAR pServiceName)
{
	m_pServiceName = pServiceName;
	m_hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == m_hSCManager)
	{
		m_dwLastError = GetLastError();
		return FALSE;
	}
	m_hService = OpenService(m_hSCManager, m_pServiceName, SERVICE_ALL_ACCESS);
	if (NULL == m_hService)
	{
		CloseServiceHandle(m_hSCManager);
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

// 安装服务
BOOL InstDrv::Install(PCHAR pSysPath, PCHAR pServiceName, PCHAR pDisplayName)
{
	m_pSysPath = pSysPath;
	m_pServiceName = pServiceName;
	m_pDisplayName = pDisplayName;
	m_hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == m_hSCManager)
	{
		m_dwLastError = GetLastError();
		return FALSE;
	}
	m_hService = CreateServiceA(m_hSCManager, m_pServiceName, m_pDisplayName,
		SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
		m_pSysPath, NULL, NULL, NULL, NULL, NULL);

	if (NULL == m_hService)
	{
		m_dwLastError = GetLastError();
		if (ERROR_SERVICE_EXISTS == m_dwLastError)
		{
			m_hService = OpenService(m_hSCManager, m_pServiceName, SERVICE_ALL_ACCESS);
			if (NULL == m_hService)
			{
				CloseServiceHandle(m_hSCManager);
				return FALSE;
			}
		}
		else
		{
			CloseServiceHandle(m_hSCManager);
			return FALSE;
		}
	}
	return TRUE;
}


// 启动服务
BOOL InstDrv::Start()
{
	if (!StartService(m_hService, NULL, NULL))
	{
		m_dwLastError = GetLastError();
		return FALSE;
	}
	return TRUE;
}

// 停止服务
BOOL InstDrv::Stop()
{
	SERVICE_STATUS ss;
	if (!ControlService(m_hService, SERVICE_CONTROL_STOP, &ss))
	{
		m_dwLastError = GetLastError();
		return FALSE;
	}
	return TRUE;
}

// 移除服务
BOOL InstDrv::Remove()
{
	if (!DeleteService(m_hService))
	{
		m_dwLastError = GetLastError();
		return FALSE;
	}
	return TRUE;
}

// 打开驱动的符号链接
BOOL InstDrv::Open(PCHAR pLinkName)
{
	if (m_hDevice != INVALID_HANDLE_VALUE)
		return TRUE;
	m_hDevice = CreateFile(pLinkName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, 0);
	if (m_hDevice != INVALID_HANDLE_VALUE)
		return TRUE;
	else
		return FALSE;
}

DWORD InstDrv::CTL_CODE_GEN(DWORD lngFunction)
{
	return (FILE_DEVICE_UNKNOWN * 65536) | (FILE_ANY_ACCESS * 16384) | (lngFunction * 4) | METHOD_BUFFERED;
}

//和驱动实现通信的核心函数
BOOL InstDrv::IoControl(DWORD dwIoCode, PVOID InBuff, DWORD InBuffLen, PVOID OutBuff, DWORD OutBuffLen)
{
	DWORD returned;
	BOOL success = DeviceIoControl(m_hDevice, CTL_CODE_GEN(dwIoCode), InBuff, InBuffLen, OutBuff, OutBuffLen, &returned, nullptr);
	return success;
}