#include "pch.h"
#include "KernelTimer.h"
#include "Helpers.h"
#include "Logging.h"

struct KernelTimerData {
	ULONG tableOffset;
	ULONG entriesOffset;
	ULONG maxEntryCount;
	void* pKiWaitNever;
	void* pKiWaitAlways;
	void* pKiProcessorBlock;
};

void KernelTimer::Init() {
	KeInitializeTimer(&_Timer);
}

bool KernelTimer::SetOneShot(LARGE_INTEGER interval, PKDPC Dpc) {
	return KeSetTimer(&_Timer, interval, Dpc);
}

bool KernelTimer::SetPeriod(LARGE_INTEGER interval,ULONG period, PKDPC Dpc) {
	return KeSetTimerEx(&_Timer, interval, period, Dpc);
}

void KernelTimer::Cancel() {
	KeCancelTimer(&_Timer);
}

void KernelTimer::EnumKernelTimer(KernelTimerData* pData) {
	PULONG_PTR KiProcessorBlock = (PULONG_PTR)pData->pKiProcessorBlock;
	for (KAFFINITY i = 0; i < KeNumberProcessors; i++) {
		ULONG_PTR kprcbAddr = KiProcessorBlock[i];
		ULONG_PTR tableAddr = kprcbAddr + pData->tableOffset;
		ULONG_PTR entriesAddr = tableAddr + pData->entriesOffset;
		PKTIMER_TABLE_ENTRY pTableEntry = (PKTIMER_TABLE_ENTRY)entriesAddr;
		if (!MmIsAddressValid(pTableEntry)) {
			return;
		}
		for (int i = 0; i < pData->maxEntryCount; i++) {
			PLIST_ENTRY pListHead = &pTableEntry[i].Entry;

			for (PLIST_ENTRY pListEntry = pListHead->Flink; pListEntry != pListHead; pListEntry = pListEntry->Flink) {
				if (!MmIsAddressValid(pListEntry))
					continue;
				PKTIMER pTimer = CONTAINING_RECORD(pListEntry, KTIMER, TimerListEntry);
#ifdef _WIN64
				Helpers::KiWaitAlways = *(ULONG_PTR*)pData->pKiWaitAlways;
				Helpers::KiWaitNever = *(ULONG_PTR*)pData->pKiWaitNever;
				ULONG_PTR salt = (ULONG_PTR)pTimer;
				PKDPC pKDpc = (PKDPC)Helpers::KiDecodePointer((ULONG_PTR)pTimer->Dpc, salt);
				if (!MmIsAddressValid(pKDpc))
					continue;
				LogInfo("KTIMER: 0x%p \t KDPC: 0x%p \t函数入口: 0x%p\t\n", pTimer, pKDpc,pKDpc->DeferredRoutine);
#else
				LogInfo("KTIMER: 0x%p \t KDPC: 0x%p \t函数入口: 0x%p\t\n", pTimer, pTimer->Dpc, pTimer->Dpc->DeferredRoutine);
#endif // _WIN64
			}
		}
	}
}