#include"ProcManager.h"

PETHREAD LookupThread(HANDLE Tid) {
	PETHREAD ethread;
	if (NT_SUCCESS(PsLookupThreadByThreadId(Tid, &ethread)))
		return ethread;
	else
		return NULL;
}

void EnumThread(PEPROCESS Process) {
	ULONG i = 0;
	PETHREAD ethrd = nullptr;
	PEPROCESS eproc = nullptr;
	for (i = 4; i < 262144; i = i + 4) {	// 这里的PID和TID范围有点随意，以后完善
		ethrd = LookupThread((HANDLE)i);
		if (ethrd != NULL) {
			eproc = IoThreadToProcess(ethrd);
			if (eproc == Process) {
				KdPrint(("[THREAD] ETHREAD=%p TID=%ld\n", 
					ethrd, PsGetThreadId(ethrd)));
			}
			ObDereferenceObject(ethrd);
		}
	}
}


void EnumModule(PEPROCESS Process) {
	UNREFERENCED_PARAMETER(Process);
}

/*
PsGetProcessId(eproc),
PsGetProcessInheritedFromUniqueProcessId(eproc),
PsGetProcessImageFileName(eproc)
*/

//常规方法结束进程
void ZwKillProcess(HANDLE Pid) {
	HANDLE hProcess = NULL;
	CLIENT_ID ClientId;
	OBJECT_ATTRIBUTES oa = { 0 };
	ClientId.UniqueProcess = Pid;
	ClientId.UniqueThread = 0;
	oa.Length = sizeof(oa);
	ZwOpenProcess(&hProcess, 1, &oa, &ClientId);
	if (hProcess) {
		ZwTerminateProcess(hProcess,0);
		ZwClose(hProcess);
	}
}

//常规方法结束线程
void ZwKillThread(HANDLE Tid) {
	HANDLE hThread = NULL;
	CLIENT_ID ClientId;
	OBJECT_ATTRIBUTES oa = { 0 };
	ClientId.UniqueProcess = 0;
	ClientId.UniqueThread = Tid;
	oa.Length = sizeof(oa);
	ZwOpenThread(&hThread, 1, &oa, &ClientId);
	if (hThread) {
		// 未导出函数
		//ZwTerminateThread(hThread, 0);
		ZwClose(hThread);
	}
}