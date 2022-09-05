#pragma once

#include "Common.h"

struct IoTimer {
	NTSTATUS Init(PDEVICE_OBJECT DeviceObject, PIO_TIMER_ROUTINE TimerRoutine,
		PVOID Context);
	void Start(PDEVICE_OBJECT DeviceObject);
	void Stop(PDEVICE_OBJECT DeviceObject);
	static void EnumIoTimer(IoTimerData* pData, IoTimerInfo* pInfo);
};