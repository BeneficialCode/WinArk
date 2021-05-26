#pragma once


struct Mutex {
public:
	void Init();

	void Lock();
	void Unlock();
private:
	FAST_MUTEX _mutex;
};