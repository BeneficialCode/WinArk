#include "pch.h"
#include "Thread.h"

NTSTATUS Thread::CreateSystemThread(PKSTART_ROUTINE routine, PVOID context) {
	return PsCreateSystemThread(&_handle, 0, nullptr, NULL, nullptr, routine, context);
}

NTSTATUS Thread::Close() {
	return ZwClose(_handle);
}

NTSTATUS Thread::SetPriporty(int id, int priority) {
	PETHREAD Thread;
	auto status = PsLookupThreadByThreadId(UlongToHandle(id), &Thread);
	if (!NT_SUCCESS(status))
		return status;

	KeSetPriorityThread(Thread, priority);
	ObDereferenceObject(Thread);
	return status;
}