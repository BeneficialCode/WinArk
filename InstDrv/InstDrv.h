#pragma once
#include<Windows.h>

class InstDrv
{
public:
	InstDrv()
	{
		m_pSysPath = NULL;
		m_pServiceName = NULL;
		m_pDisplayName = NULL;
		m_hSCManager = NULL;
		m_hService = NULL;
		m_hDevice = INVALID_HANDLE_VALUE;
	}
	~InstDrv()
	{
		if (m_hService != NULL)
			CloseServiceHandle(m_hService);
		if (m_hSCManager != NULL)
			CloseServiceHandle(m_hSCManager);
		if (m_hDevice != INVALID_HANDLE_VALUE)
			CloseHandle(m_hDevice);
	}
public:
	DWORD m_dwLastError;
	PCHAR m_pSysPath;
	PCHAR m_pServiceName;
	PCHAR m_pDisplayName;
	HANDLE m_hDevice;
	SC_HANDLE m_hSCManager;
	SC_HANDLE m_hService;
	BOOL Install(PCHAR pSysPath,PCHAR pServiceName,PCHAR pDisplayName);
	BOOL Start();
	BOOL Stop();
	BOOL Remove();
	BOOL Open(PCHAR pLinkName);
	BOOL IoControl(DWORD dwIoCode, PVOID InBuff, DWORD InBuffLen, PVOID OutBuff, DWORD OutBufLen);
private:
	BOOL GetSvcHandle(PCHAR pServiceName);
	DWORD CTL_CODE_GEN(DWORD lngFunction);
};

