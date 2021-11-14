#include "pch.h"
#include "ProcessProtect.h"
#include "AutoLock.h"


ProcessProtectGlobals g_ProtectData;

OB_PREOP_CALLBACK_STATUS
OnPreOpenProcess(PVOID, POB_PRE_OPERATION_INFORMATION Info) {
	if (Info->KernelHandle)
		return OB_PREOP_SUCCESS;

	auto process = (PEPROCESS)Info->Object;
	auto pid = HandleToUlong(PsGetProcessId(process));

	AutoLock locker(g_ProtectData.Lock);
	bool find = FindProcess(pid);
	if (g_ProtectData.Attack && find) {
		if (Info->Operation == OB_OPERATION_HANDLE_CREATE) {
			Info->Parameters->CreateHandleInformation.DesiredAccess |= (PROCESS_TERMINATE | PROCESS_VM_READ | PROCESS_VM_WRITE);
		}
		else {
			Info->Parameters->DuplicateHandleInformation.DesiredAccess |= (PROCESS_TERMINATE | PROCESS_VM_READ | PROCESS_VM_WRITE);
		}
	}
	else {
		if (Info->Operation == OB_OPERATION_HANDLE_CREATE) {
			// found in list,remove terminate, read, write accesses
			Info->Parameters->CreateHandleInformation.DesiredAccess &=
				~PROCESS_TERMINATE;
			Info->Parameters->CreateHandleInformation.DesiredAccess &=
				~PROCESS_VM_READ;
			Info->Parameters->CreateHandleInformation.DesiredAccess &=
				~PROCESS_VM_WRITE;
		}
		else {
			// found in list,remove terminate, read, write accesses
			Info->Parameters->DuplicateHandleInformation.DesiredAccess &=
				~PROCESS_TERMINATE;
			Info->Parameters->DuplicateHandleInformation.DesiredAccess &=
				~PROCESS_VM_READ;
			Info->Parameters->DuplicateHandleInformation.DesiredAccess &=
				~PROCESS_VM_WRITE;
		}
		
	}


	return OB_PREOP_SUCCESS;
}

bool FindProcess(ULONG pid) {
	if (g_ProtectData.PidsCount == 0)
		return false;

	for (int i = 0; i < MaxPids; i++)
		if (g_ProtectData.Pids[i] == pid)
			return true;
	return false;
}

bool AddProcess(ULONG pid) {
	for (int i = 0; i < MaxPids; i++)
		if (g_ProtectData.Pids[i] == 0) {
			// empty slot
			g_ProtectData.Pids[i] = pid;
			g_ProtectData.PidsCount++;
			return true;
		}
	return false;
}

bool RemoveProcess(ULONG pid) {
	for (int i = 0; i < MaxPids; i++)
		if (g_ProtectData.Pids[i] == pid) {
			g_ProtectData.Pids[i] = 0;
			g_ProtectData.PidsCount--;
			return true;
		}
	return false;
}