#pragma once

template<typename T,ULONG Tag,POOL_TYPE PoolType = PagedPool>
struct kvector {
	static_assert(Tag != 0);
	kvector(ULONG capacity = 0) {
		if (capacity == 0)
			capacity = 4;
		m_Size = 0;

		m_array = Allocate(m_Capacity = capacity);
	}

	kvector(const kvector&) = delete;
	kvector& operator=(const kvector&) = delete;

	~kvector() {
		if (m_array)
			ExFreePoolWithTag(m_array, Tag);
	}

	ULONG Size() const {
		return m_Size;
	}

	size_t Capacity() const {
		return m_Capacity;
	}

	void Add(const T& value) {
		NT_ASSERT(m_Size <= m_Capacity);
		if (m_Size == m_Capacity)
			Resize(m_Capacity * 2);
		m_array[m_Size++] = value;
	}

	T& GetAt(size_t index) {
		NT_ASSERT(index < m_Size);
		return m_array[index];
	}

	const T& GetAt(size_t index) const {
		NT_ASSERT(index < m_Size);
		return m_array[index];
	}

	T& operator[](size_t index) {
		return GetAt(index);
	}

	const T& operator[](size_t index) const{
		return GetAt(index);
	}

	void SetAt(size_t index, const T& value) {
		NT_ASSERT(index < m_Size);
		m_array[index] = value;
	}

	void RemoveAt(size_t index) {
		NT_ASSERT(index < m_Size);
		if (index < m_Size - 1) {
			::memcpy(m_array + index, m_array + (index + 1),
				(m_Size - index - 1) * sizeof(T));
		}
		m_Size--;
	}

	void Clear() {
		m_Size = 0;
	}

	void Resize(ULONG capacity) {
		T* array = Allocate(m_Capacity = capacity);
		memcpy(array, m_array, sizeof(T) * m_Size);
		m_array = array;
	}

	T* begin() {
		return m_array;
	}

	const T* begin() const {
		return m_array;
	}

	T* end() {
		return m_array + m_Size;
	}

	const T* end() const {
		return m_array + m_Size;
	}
private:
	T* Allocate(ULONG size) {
		auto buffer = static_cast<T*>(ExAllocatePoolWithTag(PoolType,
			sizeof(T) * size, Tag));
		if (!buffer)
			return nullptr;
		RtlZeroMemory(buffer, sizeof(T) * size);
		return buffer;
	}
private:
	T* m_array;
	ULONG m_Size, m_Capacity;
};