#include<ntddk.h>
#include<intrin.h>
#include"util.h"


KTIMER Timer;
KDPC TimerDpc;

void InitializeAndStartTimer(ULONG msec) {
	KeInitializeTimer(&Timer);
	KeInitializeDpc(&TimerDpc,
		OnTimerExpired,	// callback function
		nullptr);		// passed to callback as "context"

	// relative interval is in 100nsec units (and must be negative)
	// convert to msec by multiplying by 10000

	LARGE_INTEGER interval;
	interval.QuadPart = -100000LL * msec;
	KeSetTimer(&Timer, interval, &TimerDpc);
}

//  Usinga DPC is more powerful than a zero IRQL based callback, 
// since it is guaranteed to execute beforeany user mode code (and most kernel mode code).
void OnTimerExpired(KDPC* Dpc, PVOID context, PVOID, PVOID) {
	UNREFERENCED_PARAMETER(Dpc);
	UNREFERENCED_PARAMETER(context);

	NT_ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	// handle timer expiration
	KdPrint(("DPC's IRQL: %d\n", KeGetCurrentIrql()));
}
