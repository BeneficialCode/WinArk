#pragma once

// 加锁后，IRQL会提升到Dispatch Level,在这个IRQL,当前cpu被占用，不能调度其他线程
// 对系统性能的影响是巨大的，应根据不同的IRQL,选择可用的同步方案，做到最小性能牺牲
struct SpinLock {
public:
	void Init();

	void Lock();
	void Unlock();

private:
	KSPIN_LOCK _lock;
	KIRQL _oldIrql;
};