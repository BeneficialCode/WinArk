#pragma once

template<typename T>
struct LookasideList {
	NTSTATUS Init(POOL_TYPE pool, ULONG tag) {
		return ExInitializeLookasideListEx(&m_lookaside, nullptr, nullptr,
			pool, 0, sizeof(T), tag, 0);
	}

	void Delete() {
		ExDeleteLookasideListEx(&m_lookaside);
	}

	T* Alloc() {
		return (T*)ExAllocateFromLookasideListEx(&m_lookaside);
	}

private:
	LOOKASIDE_LIST_EX m_lookaside;
};