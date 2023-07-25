#pragma once

// 遵循"最小化原则"，非分页内存比较昂贵，所以默认是分页内存
void* _cdecl operator new(size_t size, POOL_TYPE type, ULONG tag = 0);

void* _cdecl operator new(size_t size, POOL_TYPE pool,
	EX_POOL_PRIORITY priority, ULONG tag = 0);

// size 是编译器确定的
// placement new
void* _cdecl operator new(size_t size, void* p);

// https://stackoverflow.com/questions/10397826/overloading-operator-new-with-wdk
void _cdecl operator delete(void* p, size_t);

void _cdecl operator delete(void* p);

void* _cdecl operator new[](size_t size, POOL_TYPE type, ULONG tag);

void _cdecl operator delete[](void* p, size_t);

template<typename T = void>
struct kunique_ptr {
	explicit kunique_ptr(T* p = nullptr) :_p(p) {}

	// remove copy ctor and copy = (single owner)
	kunique_ptr(const kunique_ptr&) = delete;
	kunique_ptr& operator=(const kunique_ptr&) = delete;

	// allow ownership transfer
	kunique_ptr(kunique_ptr&& other) :_p(other._p) {
		other._p = nullptr;
	}

	kunique_ptr& operator=(kunique_ptr&& other) {
		if (&other != this) {
			Release();
			_p = other._p;
			other._p = nullptr;
		}
		return *this;
	}

	~kunique_ptr() {
		if (_p)
			ExFreePool(_p);
	}

	operator bool() const {
		return _p != nullptr;
	}

	T* operator->() const {
		return _p;
	}

	T& operator*() const {
		return *_p;
	}

	void Release() {
		if (_p)
			ExFreePool(_p);
	}

private:
	T* _p;
};
