#pragma once


void InitializeAndStartTimer(ULONG msec);
void OnTimerExpired(KDPC* Dpc, PVOID context, PVOID, PVOID);

