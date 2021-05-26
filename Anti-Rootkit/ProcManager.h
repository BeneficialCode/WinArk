#pragma once
#include<ntifs.h>

extern "C" {
	NTKERNELAPI UCHAR* PsGetProcessImageFileName(IN PEPROCESS Process);
	NTKERNELAPI HANDLE PsGetProcessInheritedFromUniqueProcessId(IN PEPROCESS Process);
	NTKERNELAPI PPEB PsGetProcessPeb(PEPROCESS Process);
	NTKERNELAPI NTSTATUS ZwOpenThread(
		_Out_  PHANDLE ThreadHandle,
		_In_   ACCESS_MASK DesiredAccess,
		_In_   POBJECT_ATTRIBUTES ObjectAttributes,
		_In_   PCLIENT_ID ClientId
	);
}

//根据进程ID返回进程EPROCESS，失败返回NULL
PEPROCESS LookupProcess(HANDLE Pid);

//根据线程ID返回线程ETHREAD，失败返回NULL
PETHREAD LookupThread(HANDLE Tid);

//枚举指定进程的线程
void EnumThread(PEPROCESS Process);

//枚举指定进程的模块
void EnumModule(PEPROCESS Process);

//枚举进程
void EnumProcess();

//常规方法结束进程
void ZwKillProcess(HANDLE Pid);

//常规方法结束线程
void ZwKillThread(HANDLE Tid);