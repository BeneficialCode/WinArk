#pragma once

#include "Common.h"

struct KernelTimer {
	void Init();
	bool SetOneShot(LARGE_INTEGER interval,PKDPC Dpc);
	bool SetPeriod(LARGE_INTEGER interval,ULONG period,PKDPC Dpc);
	void Cancel();

	static void EnumKernelTimer(KernelTimerData* pData,DpcTimerInfo* pInfo);
	static ULONG GetKernelTimerCount(KernelTimerData* pData);
	KTIMER _Timer;
};