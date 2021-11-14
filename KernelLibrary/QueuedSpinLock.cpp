#include "pch.h"
#include "QueuedSpinLock.h"

void QueuedSpinLock::Init() {
	KeInitializeSpinLock(&_lock);
}

void QueuedSpinLock::Lock() {
	KeAcquireInStackQueuedSpinLock(&_lock, &_handle);
}

void QueuedSpinLock::Unlock() {
	KeReleaseInStackQueuedSpinLock(&_handle);
}