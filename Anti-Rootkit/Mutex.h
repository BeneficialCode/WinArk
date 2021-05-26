#pragma once

struct Mutex {
public:
	void Init();

	void Lock();
	void Unlock();
private:
	KMUTEX _mutex;
};

/*
Mutex MyMutex;

void Init(){
	MyMutex.Init();
}

void DoWork(){
	AutoLock<Mutex> locker(MyMutex);

	// access DateHead freely
}
*/