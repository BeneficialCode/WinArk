#pragma once

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

template<typename TLock>
struct SharedLocker {
	SharedLocker(TLock& lock) :m_lock(lock) {
		m_lock.LockShared();
	}

	~SharedLocker() {
		m_lock.UnlockShared();
	}

private:
	TLock& m_lock;
};