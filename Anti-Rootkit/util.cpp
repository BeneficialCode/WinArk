#include<ntddk.h>
#include<intrin.h>
#include"util.h"


KTIMER Timer;
KDPC TimerDpc;
void RaiseIRQL() {
	KIRQL oldIrql;
	KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
	KeLowerIrql(oldIrql);
}

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

void WriteProtectOff()
{
	KIRQL oldIrql;
	KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
	auto cr0 = __readcr0();
	cr0 &= 0xfffffffffffeffff;
	__writecr0(cr0);
	KeLowerIrql(oldIrql);
}

void WriteProtectOn()
{
	KIRQL oldIrql;
	KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
	auto cr0 = __readcr0();
	cr0 |= 0x10000;
	__writecr0(cr0);
	KeLowerIrql(oldIrql);
}

void DisableAPC()
{
	KIRQL oldIrql;
	KeRaiseIrql(APC_LEVEL, &oldIrql);
}

void EnableAPC()
{
	KeLowerIrql(PASSIVE_LEVEL);
}

PVOID GetFunctionAddr(PCWSTR FunctionName)
{
	UNICODE_STRING UniCodeFunctionName;
	RtlInitUnicodeString(&UniCodeFunctionName, FunctionName);
	return MmGetSystemRoutineAddress(&UniCodeFunctionName);
}

void MyGetCurrentTime() {
	LARGE_INTEGER CurrentTime;
	LARGE_INTEGER LocalTime;
	TIME_FIELDS TimeField;
	KeQuerySystemTime(&CurrentTime);
	ExSystemTimeToLocalTime(&CurrentTime, &LocalTime);
	RtlTimeToTimeFields(&LocalTime, &TimeField);
	KdPrint(("Now Time: %4d-%02d-%2d %2d:%2d:%2d", TimeField.Year,
		TimeField.Month,TimeField.Day,TimeField.Hour,
		TimeField.Minute,TimeField.Second));
}

#define DELAY_ONE_MICROSECOND 	(-10)
#define DELAY_ONE_MILLISECOND	(DELAY_ONE_MICROSECOND*1000)

void MySleep(LONG msec) {
	LARGE_INTEGER my_interval;
	my_interval.QuadPart = DELAY_ONE_MILLISECOND;
	my_interval.QuadPart *= msec;
	KeDelayExecutionThread(KernelMode, false, &my_interval);
}

UINT_PTR SearchFeature(UINT_PTR nAddr, UCHAR* pFeature, int nLeng)
{
	UCHAR szFeatureCode[256] = "";
	int i = 0x4000;
	while (i--)
	{
		memcpy(szFeatureCode, (UCHAR*)nAddr, nLeng);
		if (memcmp(pFeature, szFeatureCode, nLeng) == 0)
		{
			return nAddr;
		}
		nAddr++;
	}
	return NULL;
}


