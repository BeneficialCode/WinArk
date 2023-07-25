#pragma once
#include <vector>
#include <algorithm>
#include <functional>

template<typename T>
class SortedFilteredVector {
public:
	SortedFilteredVector(size_t capacity = 16) {
		_items.reserve(capacity);
		_indices.reserve(capacity);
	}

	void reserve(size_t capacity) {
		_items.reserve(capacity);
		_indices.reserve(capacity);
	}

	void clear() {
		_items.clear();
		_indices.clear();
	}

	bool empty() const {
		return _items.empty();
	}

	void push_back(const T& value) {
		_items.push_back(value);
		_indices.push_back(_indices.size());
	}

	void shrink_to_fit() {
		_items.shrink_to_fit();
		_indices.shrink_to_fit();
	}

	void Remove(size_t index) {
		auto realIndex = _indices[index];
		_items.erase(_items.begin() + realIndex);
		_indices.erase(_indices.begin() + index);
	}

	typename std::vector<T>::const_iterator begin() const {
		return _items.begin();
	}

	typename std::vector<T>::const_iterator end() const {
		return _items.end();
	}

	void Set(std::vector<T> items) {
		_items = std::move(items);
		auto count = _items.size();
		_indices.clear();
		_indices.reserve(count);
		for (decltype(count) i = 0; i < count; i++)
			_indices.push_back(i);
	}

	const T& operator[](size_t index) const {
		return _items[_indices[index]];
	}

	T& operator[](size_t index) {
		return _items[_indices[index]];
	}

	void Sort(std::function<bool(const T& value1, const T& value2)> compare) {
		std::sort(_indices.begin(), _indices.end(), [&](size_t i1, size_t i2) {
			return compare(_items[i1], _items[i2]);
			});
	}

	size_t size() const {
		return _indices.size();
	}

	size_t TotalSize() const {
		return _items.size();
	}

	void Filter(std::function<bool(const T& value)> predicate) { // ´ÊÌõ
		_indices.clear();
		auto count = _items.size();
		if (predicate == nullptr) {
			for (decltype(count) i = 0; i < count; i++) {
				_indices.push_back(i);
			}
		}
		else {
			for (decltype(count)i = 0; i < count; i++)
				if (predicate(_items[i]))
					_indices.push_back(i);
		}
	}

private:
	std::vector<T> _items;
	std::vector<size_t> _indices; // Ë÷Òý
};