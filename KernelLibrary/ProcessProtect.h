#pragma once

#include "FastMutex.h"

#define PROCTECT_PREFIX "ProcessProtect: "
const int MaxPids = 256;

#define PROCESS_TERMINATE                  (0x0001)  
#define PROCESS_VM_READ                    (0x0010)  
#define PROCESS_VM_WRITE                   (0x0020) 

struct ProcessProtectGlobals {
	int PidsCount;			// currently protected process count
	ULONG Pids[MaxPids];	// protected PIDs
	FastMutex Lock;
	PVOID RegHandle;		// object registration cookie
	BOOLEAN Attack;			// Attack other process

	void Init() {
		Lock.Init();
	}
};

extern ProcessProtectGlobals g_ProtectData;

OB_PREOP_CALLBACK_STATUS OnPreOpenProcess(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION Info);

bool FindProcess(ULONG pid);
bool AddProcess(ULONG pid);
bool RemoveProcess(ULONG pid);