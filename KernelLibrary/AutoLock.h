#pragma once
#include "pch.h"

template<typename TLock>
struct AutoLock {
	AutoLock(TLock& lock) :_lock(lock) {
		_lock.Lock();
	}

	~AutoLock() {
		_lock.Unlock();
	}
private:
	TLock& _lock;
};