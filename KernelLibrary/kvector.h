#pragma once

#ifndef  DRIVER_TAG
#define DRIVER_TAG 'dltk'
#endif // ! DRIVER_TAG

#include "Memory.h"

#ifdef KTL_NAMESPACE
namespace ktl {
#endif

	template<typename T, POOL_TYPE PoolType = PagedPool, ULONG Tag = DRIVER_TAG>
	struct kvector {
		struct Iterator {
			Iterator(kvector& v, ULONG const index) :m_vector(v), m_index(index) {}
			Iterator& operator++() {
				++m_index;
				return *this;
			}
			Iterator operator++(int) const {
				return Iterator(m_vector, m_index + 1);
			}

			bool operator==(Iterator const& other) const {
				return &m_vector == &other.m_vector && m_index == other.m_index;
			}

			bool operator!=(Iterator const& other) const {
				return !(*this == other);
			}

			T const& operator*() const {
				return m_vector[m_index];
			}

			T& operator*() {
				return m_vector[m_index];
			}

		private:
			ULONG m_index;
			kvector& m_vector;
		};

		static_assert(Tag != 0);
		explicit kvector(ULONG capacity = 0) {
			m_Capacity = capacity;
			m_Size = 0;
			if (capacity) {
				m_pData = static_cast<T*>(ExAllocatePoolWithTag(PoolType,
					sizeof(T) * capacity, Tag));
				if (!m_pData)
					::ExRaiseStatus(STATUS_NO_MEMORY);
			}
			else {
				m_pData = nullptr;
			}
		}

		kvector(kvector const& other) :m_Capacity(other.Size()) {
			m_pData = static_cast<T*>(ExAllocatePoolWithTag(PoolType, (m_Size = m_Capacity) * sizeof(T), Tag));
			if (!m_pData)
				::ExRaiseStatus(STATUS_NO_MEMORY);
			memcpy(m_pData, other.m_pData, sizeof(T) * m_Size);
		}


		kvector& operator=(kvector const& other) {
			if (this != other) {
				Free();
				m_Capacity = other.m_Capacity;
				m_pData = static_cast<T*>(ExAllocatePoolWithTag(PoolType, (m_Size = m_Capacity) * sizeof(T), Tag));
				if (!m_pData) {
					::ExRaiseStatus(STATUS_NO_MEMORY);
					memcpy(m_pData, other.m_pData, sizeof(T) * m_Size);
				}
				return *this;
			}
		}

		kvector(kvector&& other) :m_Capacity(other.m_Capacity), m_Size(other.m_Size) {
			m_pData = other.m_pData;
			other.m_pData = nullptr;
			other.m_Size = other.m_Capacity = 0;
		}

		kvector& operator=(kvector&& other) {
			if (this != &other) {
				Free();
				m_Size = other.m_Size;
				m_Capacity = other.m_Capacity;
				m_pData = other.m_pData;
				other.m_pData = nullptr;
				other.m_Size = other.m_Capacity = 0;
			}
			return *this;
		}

		~kvector() {
			if (m_pData)
				ExFreePoolWithTag(m_pData, Tag);
		}

		ULONG Size() const {
			return m_Size;
		}

		size_t Capacity() const {
			return m_Capacity;
		}

		bool IsEmpty() const {
			return m_Size == 0;
		}

		void Add(const T& value) {
			NT_ASSERT(m_Size <= m_Capacity);
			if (m_Size == m_Capacity)
				Resize(m_Capacity * 2);
			m_pData[m_Size++] = value;
		}

		T& GetAt(size_t index) {
			NT_ASSERT(index < m_Size);
			return m_pData[index];
		}

		const T& GetAt(size_t index) const {
			NT_ASSERT(index < m_Size);
			return m_pData[index];
		}

		T& operator[](size_t index) {
			return GetAt(index);
		}

		const T& operator[](size_t index) const {
			return GetAt(index);
		}

		void SetAt(size_t index, const T& value) {
			NT_ASSERT(index < m_Size);
			m_pData[index] = value;
		}

		void RemoveAt(size_t index) {
			NT_ASSERT(index < m_Size);
			if (index < m_Size - 1) {
				::memcpy(m_pData + index, m_pData + (index + 1),
					(m_Size - index - 1) * sizeof(T));
			}
			m_Size--;
		}

		void Clear() {
			m_Size = m_Capacity = 0;
			Free();
		}

		void Resize(ULONG newSize) {
			m_Capacity = newSize;
			auto data = static_cast<T*>(ExAllocatePoolWithTag(PoolType, sizeof(T) * newSize, Tag));
			if (data == nullptr)
				::ExRaiseStatus(STATUS_NO_MEMORY);
			if (m_pData) {
				memcpy(data, m_pData, sizeof(T) * m_Size);
				ExFreePool(m_pData);
			}
			m_pData = data;
		}

		T* begin() {
			return m_pData;
		}

		const T* begin() const {
			return m_pData;
		}

		T* end() {
			return m_pData + m_Size;
		}

		const T* end() const {
			return m_pData + m_Size;
		}

		void Free() {
			if (m_pData) {
				ExFreePool(m_pData);
				m_pData = nullptr;
			}
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
		T* m_pData;
		ULONG m_Size, m_Capacity;
	};

#ifdef KTL_NAMESPACE
}
#endif
