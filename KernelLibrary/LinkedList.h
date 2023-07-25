#pragma once

#include "AutoLock.h"
#include "FastMutex.h"

template<typename T, typename TLock = FastMutex>
class LinkedList {
public:
	void Init() {
		// 头节点初始化
		InitializeListHead(&_head);
		_lock.Init();
	}

	// expects a LIST_ENTRY named "Entry"

	// 节点插入
	void PushBack(T* item) {
		AutoLock locker(_lock);
		InsertTailList(&_head, &item->Entry);
	}

	void PushFront(T* item) {
		AutoLock locker(_lock);
		InsertHeadList(&_head, &item->Entry);
	}

	// 节点移除
	T* RemoveHead() {
		AutoLock locker(_lock);
		auto entry = RemoveHeadList(&_head);
		return CONTAINING_RECORD(entry, T, Entry);
	}

	T* RemoveTail() {
		AutoLock locker(_lock);
		auto entry = RemoveTailList(&_head);
		return CONTAINING_RECORD(entry, T, Entry);
	}

	T* RemoveEntry(PLIST_ENTRY entry) {
		AutoLock locker(_lock);
		RemoveEntryList(entry);
		return CONTAINING_RECORD(entry, T, Entry);
	}

	T* GetHeadItem() {
		AutoLock locker(_lock);
		auto entry = _head.Flink;
		return CONTAINING_RECORD(entry, T, Entry);
	}

	PLIST_ENTRY GetHead() {
		return &_head;
	}

private:
	LIST_ENTRY _head;
	TLock _lock;
};