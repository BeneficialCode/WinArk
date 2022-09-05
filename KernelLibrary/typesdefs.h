#pragma once

#include "pch.h"

typedef struct _IO_TIMER {
    CSHORT Type;
    CSHORT TimerFlag;
    LIST_ENTRY TimerList;
    PIO_TIMER_ROUTINE TimerRoutine;
    PVOID Context;
    struct _DEVICE_OBJECT* DeviceObject;
}IO_TIMER;

typedef struct _KTIMER_TABLE_ENTRY {
    ULONG_PTR Lock;
    LIST_ENTRY Entry;
    ULARGE_INTEGER Time;
}KTIMER_TABLE_ENTRY, * PKTIMER_TABLE_ENTRY;


