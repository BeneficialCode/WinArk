#include "pch.h"
#include "KernelTimer.h"
#include "Helpers.h"
#include "Logging.h"
#include "typesdefs.h"

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

void KernelTimer::EnumKernelTimer(KernelTimerData* pData, DpcTimerInfo* pInfo) {
	PULONG_PTR KiProcessorBlock = (PULONG_PTR)pData->pKiProcessorBlock;
	int k = 0;

#ifdef _WIN64
	Helpers::KiWaitAlways = *(ULONG_PTR*)pData->pKiWaitAlways;
	Helpers::KiWaitNever = *(ULONG_PTR*)pData->pKiWaitNever;
#endif // _WIN64
	ULONG maxCount = pData->maxEntryCount;
	ULONG tableOffset = pData->tableOffset;
	ULONG entriesOffset = pData->entriesOffset;

	for (KAFFINITY i = 0; i < KeNumberProcessors; i++) {
		ULONG_PTR kprcbAddr = KiProcessorBlock[i];
		ULONG_PTR tableAddr = kprcbAddr + tableOffset;
		ULONG_PTR entriesAddr = tableAddr + entriesOffset;
		PKTIMER_TABLE_ENTRY pTableEntry = (PKTIMER_TABLE_ENTRY)entriesAddr;
		if (!MmIsAddressValid(pTableEntry)) {
			continue;
		}
		for (int j = 0; j < maxCount; j++) {
			PLIST_ENTRY pListHead = &pTableEntry[j].Entry;

			for (PLIST_ENTRY pListEntry = pListHead->Flink; pListEntry != pListHead; pListEntry = pListEntry->Flink) {
				if (!MmIsAddressValid(pListEntry))
					break;
				PKTIMER pTimer = CONTAINING_RECORD(pListEntry, KTIMER, TimerListEntry);
#ifdef _WIN64
				ULONG_PTR salt = (ULONG_PTR)pTimer;
				PKDPC pKDpc = (PKDPC)Helpers::KiDecodePointer((ULONG_PTR)pTimer->Dpc, salt);
				if (!MmIsAddressValid(pKDpc))
					continue;
				LogInfo("KTIMER: 0x%p \t KDPC: 0x%p \t函数入口: 0x%p\t\n", pTimer, pKDpc,pKDpc->DeferredRoutine);
				pInfo[k].DueTime = pTimer->DueTime;
				pInfo[k].KDpc = pKDpc;
				pInfo[k].KTimer = pTimer;
				pInfo[k].Routine = pKDpc->DeferredRoutine;
				pInfo[k].Period = pTimer->Period;
				k++;
#else
				if (!MmIsAddressValid(pTimer->Dpc)) {
					continue;
				}
				LogInfo("KTIMER: 0x%p \t KDPC: 0x%p \t函数入口: 0x%p\t\n", pTimer, pTimer->Dpc, pTimer->Dpc->DeferredRoutine);
				pInfo[k].DueTime = pTimer->DueTime;
				pInfo[k].KDpc = pTimer->Dpc;
				pInfo[k].KTimer = pTimer;
				pInfo[k].Routine = pTimer->Dpc->DeferredRoutine;
				pInfo[k].Period = pTimer->Period;
				k++;
#endif // _WIN64
			}
		}
	}

	LogInfo("Total Kernel Timer: %d\n", k);
}

ULONG KernelTimer::GetKernelTimerCount(KernelTimerData* pData) {
	PULONG_PTR KiProcessorBlock = (PULONG_PTR)pData->pKiProcessorBlock;
	ULONG k = 0;

#ifdef _WIN64
	Helpers::KiWaitAlways = *(ULONG_PTR*)pData->pKiWaitAlways;
	Helpers::KiWaitNever = *(ULONG_PTR*)pData->pKiWaitNever;
#endif // _WIN64
	ULONG maxCount = pData->maxEntryCount;
	ULONG tableOffset = pData->tableOffset;
	ULONG entriesOffset = pData->entriesOffset;

	for (KAFFINITY i = 0; i < KeNumberProcessors; i++) {
		ULONG_PTR kprcbAddr = KiProcessorBlock[i];
		ULONG_PTR tableAddr = kprcbAddr + tableOffset;
		ULONG_PTR entriesAddr = tableAddr + entriesOffset;
		PKTIMER_TABLE_ENTRY pTableEntry = (PKTIMER_TABLE_ENTRY)entriesAddr;
		if (!MmIsAddressValid(pTableEntry)) {
			continue;
		}
		for (int j = 0; j < maxCount; j++) {
			PLIST_ENTRY pListHead = &pTableEntry[j].Entry;

			for (PLIST_ENTRY pListEntry = pListHead->Flink; pListEntry != pListHead; pListEntry = pListEntry->Flink) {
				if (!MmIsAddressValid(pListEntry))
					break;
				PKTIMER pTimer = CONTAINING_RECORD(pListEntry, KTIMER, TimerListEntry);
#ifdef _WIN64
				ULONG_PTR salt = (ULONG_PTR)pTimer;
				PKDPC pKDpc = (PKDPC)Helpers::KiDecodePointer((ULONG_PTR)pTimer->Dpc, salt);
				if (!MmIsAddressValid(pKDpc))
					continue;
				k++;
#else
				if (!MmIsAddressValid(pTimer->Dpc)) {
					continue;
				}
				k++;
#endif // _WIN64
			}
		}
	}

	LogInfo("Total Kernel Timer: %d\n", k);
	return k;
}