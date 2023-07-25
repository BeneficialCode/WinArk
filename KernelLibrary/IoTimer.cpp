#include "pch.h"
#include "IoTimer.h"
#include "Logging.h"
#include "typesdefs.h"

NTSTATUS IoTimer::Init(PDEVICE_OBJECT DeviceObject, PIO_TIMER_ROUTINE TimerRoutine,
	PVOID Context) {
	return IoInitializeTimer(DeviceObject, TimerRoutine, Context);
}

void IoTimer::Start(PDEVICE_OBJECT DeviceObject) {
	IoStartTimer(DeviceObject);
}

void IoTimer::Stop(PDEVICE_OBJECT DeviceObject) {
	IoStopTimer(DeviceObject);
}

void IoTimer::EnumIoTimer(IoTimerData* pData, IoTimerInfo* pInfo) {
	PLIST_ENTRY pIopTimerQueueHead = *(PLIST_ENTRY*)pData->pIopTimerQueueHead;
	LIST_ENTRY IopTimerQueueHead = *pIopTimerQueueHead;
	PLIST_ENTRY timerEntry;
	PIO_TIMER timer;
	KIRQL irql;
	PKSPIN_LOCK pIopTimerLock = (PKSPIN_LOCK)pData->pIopTimerLock;


	KeAcquireSpinLock(pIopTimerLock, &irql);
	ULONG count = *pData->pIopTimerCount;
	int j = 0;
	for (timerEntry = IopTimerQueueHead.Flink;
		(timerEntry != &IopTimerQueueHead) && count;
		timerEntry = timerEntry->Flink) {

		timer = CONTAINING_RECORD(timerEntry, IO_TIMER, TimerList);
		if (timer->TimerFlag) {
			LogInfo("Type: %d,TimerRoutine: 0x%p,DeviceObject: 0x%p,TimerFlag: 0x%d",
				timer->Type,
				timer->TimerRoutine,
				timer->DeviceObject,
				timer->TimerFlag);

			pInfo[j].DeviceObject = timer->DeviceObject;
			pInfo[j].TimerFlag = timer->TimerFlag;
			pInfo[j].TimerRoutine = timer->TimerRoutine;
			pInfo[j].Type = timer->Type;
			j++;
			count--;
		}
	}
	KeReleaseSpinLock(pIopTimerLock, irql);
}