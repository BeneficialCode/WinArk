#pragma once

struct QueuedSpinLock {
public:
	void Init();

	void Lock();
	void Unlock();

private:
	KSPIN_LOCK _lock;
	KLOCK_QUEUE_HANDLE _handle;
};