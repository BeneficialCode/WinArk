#pragma once

typedef struct _KTIMER_TABLE_ENTRY {
	ULONG_PTR Lock;
	LIST_ENTRY Entry;
	ULARGE_INTEGER Time;
}KTIMER_TABLE_ENTRY,*PKTIMER_TABLE_ENTRY;

struct KernelTimerData;
struct DpcTimerInfo;
struct KernelTimer {
	void Init();
	bool SetOneShot(LARGE_INTEGER interval,PKDPC Dpc);
	bool SetPeriod(LARGE_INTEGER interval,ULONG period,PKDPC Dpc);
	void Cancel();

	static void EnumKernelTimer(KernelTimerData* pData,DpcTimerInfo* pInfo);
	static ULONG GetKernelTimerCount(KernelTimerData* pData);
	KTIMER _Timer;
};