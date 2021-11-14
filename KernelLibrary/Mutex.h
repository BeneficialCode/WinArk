#pragma once

struct Mutex {
public:
	void Init();

	void Lock();
	void Unlock();

private:
	KMUTEX _mutex;
};