#pragma once
#include "array_ptr.h"

#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <iterator>
#include <utility>

class ReserveProxyObj {
public:
	ReserveProxyObj() = default;

	ReserveProxyObj(size_t capacity_to_reserve)
		: new_capacity_(capacity_to_reserve) {
	}

	size_t GetCapacity() const noexcept {
		return new_capacity_;
	}

private:
	size_t new_capacity_ = 0;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
	return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
	using Iterator = Type*;
	using ConstIterator = const Type*;

	SimpleVector() noexcept = default;

	// Создаёт вектор из size элементов, инициализированных значением value
	explicit SimpleVector(size_t size) : items_(size), size_(size), capacity_(size) 
	{
		std::fill(begin(), end(), Type());
	}

	// Создаёт вектор из size элементов, инициализированных значением value
	SimpleVector(size_t size, const Type& value) : items_(size), size_(size), capacity_(size) 
	{
		std::fill(begin(), end(), value);
	}

	// Создаёт вектор из std::initializer_list
	SimpleVector(std::initializer_list<Type> init) : items_(init.size()), size_(init.size()), capacity_(init.size()) 
	{
		std::copy(init.begin(), init.end(), begin());
	}
	
	SimpleVector(ReserveProxyObj res) : items_(res.GetCapacity()), size_(0), capacity_(res.GetCapacity()) {}

	SimpleVector(const SimpleVector& other)
		: items_(other.GetSize()), size_(other.GetSize()), capacity_(other.GetCapacity())
	{
		std::copy(other.begin(), other.end(), begin());
	}
	
	SimpleVector(SimpleVector&& other) noexcept
		: items_(std::move(other.items_))
		, size_(std::exchange(other.size_, 0))
		, capacity_(std::exchange(other.capacity_, 0))
	{
	}

	// Возвращает количество элементов в массиве
	size_t GetSize() const noexcept {
		return size_;
	}

	// Возвращает вместимость массива
	size_t GetCapacity() const noexcept {
		return capacity_;
	}

	// Сообщает, пустой ли массив
	bool IsEmpty() const noexcept {
		return size_ == 0;
	}

	Type& operator[](size_t index) noexcept {
		return items_[index];
	}

	const Type& operator[](size_t index) const noexcept {
		return items_[index];
	}

	SimpleVector& operator=(const SimpleVector& rhs) {
		if (this != &rhs) {
			SimpleVector rhs_copy(rhs);
			swap(rhs_copy);
		}
		return *this;
	}

	SimpleVector& operator=(SimpleVector&& rhs) noexcept {
		if (&rhs != this)
		{
			items_ = std::move(rhs.items_);
			capacity_ = std::exchange(rhs.capacity_, 0);
			size_ = std::exchange(rhs.size_, 0);
		}
		return *this;
	}

	Type& At(size_t index) {
		if (index >= size_) {
			throw std::out_of_range("Expect invalid_argument for too large index");
		}
		return items_[index];
	}

	const Type& At(size_t index) const {
		if (index >= size_) {
			throw std::out_of_range("Expect invalid_argument for too large index");
		}
		return items_[index];
	}

	void Clear() noexcept {
		size_ = 0;
	}

	// Добавляет элемент в конец вектора
	void PushBack(Type&& item)
	{
		if (size_ < capacity_) {
			items_[size_] = std::move(item);
			++size_;
		}
		else {
			Resize(size_ + 1);
			items_[size_ - 1] = std::move(item);
		}
	}

	void PushBack(const Type& item)
	{
		if (size_ < capacity_) {
			items_[size_] = item;
			++size_;
		}
		else {
			Resize(size_ + 1);
			items_[size_ - 1] = item;
		}
	}

	void Reserve(size_t new_capacity) {
		if (new_capacity > capacity_) {
			ArrayPtr<Type> new_items(new_capacity);
			std::copy(cbegin(), cend(), new_items.Get());
			items_.swap(new_items);
			capacity_ = new_capacity;
		}
	}

	void Resize(size_t new_size)
	{
		if (new_size < size_)
		{
			size_ = new_size;
			return;
		}

		if (new_size > capacity_)
		{
			ArrayPtr<Type> resize_array(new_size);
			std::move(begin(), end(), resize_array.Get());
			items_.swap(resize_array);
			size_ = new_size;
			capacity_ = new_size;
		}
		else
		{
			std::generate(begin() + size_, end() + new_size, []() {return Type{}; });
		}
	}

	// Вставляет значение value в позицию pos.
	Iterator Insert(ConstIterator pos, const Type& value) {
		assert(pos <= end() && pos >= begin());
		Iterator it = Iterator(pos);
		if (size_ == capacity_) {
			auto dist = it - begin();
			size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
			auto size = size_;
			Resize(new_capacity);
			size_ = size;
			capacity_ = new_capacity;
			it = begin() + dist;
		}
		std::copy_backward(it, end(), end() + 1);
		*it = const_cast<Type&&>(value);
		++size_;
		return it;
	}

	Iterator Insert(Iterator pos, Type&& value) {
		assert(pos <= end() && pos >= begin());
		Iterator it = Iterator(pos);
		if (size_ == capacity_) {
			auto dist = it - begin();
			size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
			auto size = size_;
			Resize(std::move(new_capacity));
			size_ = size;
			capacity_ = new_capacity;
			it = begin() + dist;
		}
		std::move_backward(it, end(), end() + 1);
		*it = std::move(const_cast<Type&&>(value));
		++size_;
		return it;
	}

	// "Удаляет" последний элемент вектора. Вектор не должен быть пустым
	void PopBack() noexcept {
		assert(!IsEmpty());
		size_--;
	}

	// Удаляет элемент вектора в указанной позиции
	Iterator Erase(ConstIterator pos) {
		assert(pos <= end() && pos >= begin());
		if (size_ > 0) {
			std::copy(pos + 1, cend(), begin() + std::distance(cbegin(), pos));
			--size_;
		}
		return begin() + std::distance(cbegin(), pos);
	}

	// Удаляет элемент вектора в указанной позиции
	Iterator Erase(Iterator pos) {
		assert(pos <= end() && pos >= begin());
		if (size_ > 0) {
			std::move(pos + 1, end(), begin() + std::distance(begin(), pos));
			--size_;
		}
		return begin() + std::distance(begin(), pos);
	}
	
	// Обменивает значение с другим вектором
	void swap(SimpleVector& other) noexcept {
		items_.swap(other.items_);
		std::swap(size_, other.size_);
		std::swap(capacity_, other.capacity_);
	}

	[[nodiscard]] Iterator begin() noexcept {
		return items_.Get();
	}

	[[nodiscard]] Iterator end() noexcept {
		return items_.Get() + size_;
	}

	[[nodiscard]] ConstIterator begin() const noexcept {
		return items_.Get();
	}
	[[nodiscard]] ConstIterator end() const noexcept {
		return items_.Get() + size_;
	}
	[[nodiscard]] ConstIterator cbegin() const noexcept {
		return items_.Get();
	}
	[[nodiscard]] ConstIterator cend() const noexcept {
		return items_.Get() + size_;
	}

private:
	ArrayPtr<Type> items_{};
	size_t size_ = 0;
	size_t capacity_ = 0;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return (lhs.GetSize() == rhs.GetSize()
		&& std::equal(lhs.begin(), lhs.end(), rhs.begin()));
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return (rhs < lhs);
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return !(lhs < rhs);
}
