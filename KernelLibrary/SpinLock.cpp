#include "pch.h"
#include "SpinLock.h"

void SpinLock::Init() {
	KeInitializeSpinLock(&_lock);
}

void SpinLock::Lock() {
	KeAcquireSpinLock(&_lock,&_oldIrql);
}

void SpinLock::Unlock() {
	KeReleaseSpinLock(&_lock, _oldIrql);
}

