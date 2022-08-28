#pragma once

typedef struct _IO_TIMER {
    CSHORT Type;
    CSHORT TimerFlag;
    LIST_ENTRY TimerList;
    PIO_TIMER_ROUTINE TimerRoutine;
    PVOID Context;
    struct _DEVICE_OBJECT* DeviceObject;
}IO_TIMER;

struct IoTimerInfo;
struct IoTimerData;
struct IoTimer {
	NTSTATUS Init(PDEVICE_OBJECT DeviceObject, PIO_TIMER_ROUTINE TimerRoutine,
		PVOID Context);
	void Start(PDEVICE_OBJECT DeviceObject);
	void Stop(PDEVICE_OBJECT DeviceObject);
	static void EnumIoTimer(IoTimerData* pData, IoTimerInfo* pInfo);
};