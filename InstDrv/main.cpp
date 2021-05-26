#include<stdio.h>
#include"InstDrv.h"
#include"../Anti-Rootkit/AntiRootkitCommon.h"

extern "C"
{
	extern VOID Entry();
}

void GetAppPath(char*szCurFile)
{
	GetModuleFileName(0, szCurFile, MAX_PATH);
	for (SIZE_T i = strlen(szCurFile) - 1;i >= 0; i--)
	{
		if (szCurFile[i] == '\\')
		{
			szCurFile[i + 1] = '\0';
			break;
		}
	}
}

int(__fastcall *p3)(__int64(__fastcall *)(PCWSTR), __int64);

int main()
{
	BOOL success;
	InstDrv inst;
	// 设置驱动名称
	//char szSysFile[MAX_PATH] = { 0 };
	//char szSvcLnkName[] = "hello123";
	//GetAppPath(szSysFile);
	//strcat(szSysFile, "Driver.drv");
	//// 安装并启动驱动
	//success = inst.Install(szSysFile, szSvcLnkName, szSvcLnkName);
	//success = inst.Start();
	//printf("LoadDriver=%d\n", success);
	// "打开"驱动的符号链接
	CHAR sym[] = "\\\\.\\hello123";
	inst.Open(sym);
	char x[16] = {0};
	INT_PTR Addr = (INT_PTR)Entry;
	memcpy(x, &Addr,8);
	DWORD y = 4;
	//  STATUS_INFO_LENGTH_MISMATCH
	inst.IoControl(0x800, &x, sizeof(x), &y, sizeof(y));
	printf("INPUT=%s\nOUTPUT=%p\n", x, y);
	/*inst.IoControl(0x801, 0, 0, 0, 0);*/
	CloseHandle(inst.m_hDevice);
	//success = inst.Stop();
	//success = inst.Remove();
	//printf("UnloadDriver=%d\n", success);
	getchar();
	return 0;
}