#pragma once


void RaiseIRQL();
void InitializeAndStartTimer(ULONG msec);
void OnTimerExpired(KDPC* Dpc, PVOID context, PVOID, PVOID);
void WriteProtectOff();
void WriteProtectOn();
void DisableAPC();
PVOID GetFunctionAddr(PCWSTR FunctionName);
void MyGetCurrentTime();
void MySleep(LONG msec);
UINT_PTR SearchFeature(UINT_PTR nAddr, UCHAR* pFeature, int nLeng);


extern "C" {
	extern void ForceReboot();
	extern void ForceShutdown();
	extern UINT_PTR GetSSDTFunctionAddr(ULONG index, UINT_PTR tablebase);
}