#pragma once

// 可重入的意思是某一个线程是否可多次获得一个锁,如递归函数调用时

// 把停止线程的任务放到了驱动卸载函数时，
// 需注意在DriverEntry中把KeWaitForMultipleObjects所需要的资源提前申请好
struct Thread {
public:
	NTSTATUS CreateSystemThread(PKSTART_ROUTINE routine, PVOID context);
	NTSTATUS Close();
	
	static NTSTATUS SetPriporty(int id,int priority);
private:
	HANDLE _handle;
};