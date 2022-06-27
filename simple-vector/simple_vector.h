#pragma once

#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include <iterator>

#include "array_ptr.h"

class ReserveProxyObj {
public:
    ReserveProxyObj() = default;
    
    ReserveProxyObj(size_t size) {
        capacity_ = size;
    }
    
    size_t GetCapacity() const noexcept {
        return capacity_;;
    } 
    
private:
    size_t capacity_ = 0u;
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

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) : items_(new Type[size]{}), size_(size), capacity_(size){}

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) : items_(new Type[size]{}), size_(size), capacity_(size) {
        std::fill(items_.Get(), &items_[size], value);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) : items_(new Type[init.size()]{}), size_(init.size()), capacity_(init.size()){
        for (size_t i = 0; i < init.size(); ++i) {
            items_[i] = *(init.begin() + i);
        }
    }
    
    // Создает вектор и резервирует место для переданного в качестве аргумента числа элементов
    SimpleVector(ReserveProxyObj R) : items_(new Type[R.GetCapacity()]{}), capacity_(R.GetCapacity()) {
    }
    
    // Конструктор копирования
    SimpleVector(const SimpleVector& other) : items_(new Type[other.GetCapacity()]{}), size_(other.GetSize()), capacity_(other.GetCapacity()) {
        for (size_t i = 0; i < other.GetSize(); ++i) {
            items_[i] = *(other.begin() + i);
        }
    }
    // Конструктор копирования с перемещением
    SimpleVector(SimpleVector&& other) : items_(new Type[other.GetCapacity()]{}), size_(std::exchange(other.size_, 0u)), capacity_(std::exchange(other.capacity_, 0u)) {
        for (size_t i = 0; i < size_; ++i) {
            items_[i] = *(std::make_move_iterator(other.begin() + i));
        }
    }
    
    // Конструктор присваивания
    SimpleVector& operator=(const SimpleVector& rhs) {
        SimpleVector tmp(rhs);
        swap(tmp);
        return *this;
    }
    
    // Конструктор присваивания с перемещением
    SimpleVector& operator=(SimpleVector&& other) {
        SimpleVector tmp(other);
        swap(tmp);
        return *this;
    }
    
    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ArrayPtr<Type> new_items(new_capacity);
            std::copy(items_.Get(), &items_[size_], new_items.Get());
            items_.swap(new_items);
            capacity_ = new_capacity;
        }
    }
    
    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& value) {
        if (size_ < capacity_) {
            items_[size_] = value;
            ++size_;
        } else if (size_ == capacity_) {
            size_t new_capacity = (capacity_ == 0 ? 1 : capacity_ * 2);
            ArrayPtr<Type> new_items(new Type[new_capacity]{});
            std::copy(items_.Get(), &items_[size_], new_items.Get());
            new_items[size_] = value;
            ++size_;
            items_.swap(new_items);
            capacity_ = new_capacity;
        }
    }
    
    
    void PushBack(Type&& value) {
        if (size_ < capacity_) {
            items_[size_] = std::move(value);
            ++size_;
        } else if (size_ == capacity_) {
            size_t new_capacity = (capacity_ == 0 ? 1 : capacity_ * 2);
            ArrayPtr<Type> new_items(new Type[new_capacity]{});
            for (size_t i = 0; i < size_; ++i) {
                new_items[i] = std::move(items_[i]);
            }
            new_items[size_] = std::move(value);
            ++size_;
            items_.swap(new_items);
            capacity_ = new_capacity;
        }
    }
    

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(pos >= begin() && pos <= end());
        size_t new_item = pos - items_.Get();
        if (size_ < capacity_) {
            std::copy_backward(&items_[new_item], &items_[size_], &items_[size_ + 1]);
            items_[new_item] = value;
            ++size_;
        } else if (size_ == capacity_) {
            size_t new_capacity = (capacity_ == 0 ? 1 : capacity_ * 2);
            ArrayPtr<Type> new_items(new Type[new_capacity]{});
            std::copy(items_.Get(), &items_[new_item], new_items.Get());
            new_items[new_item] = value;
            std::copy(&items_[new_item], &items_[size_], &new_items[new_item + 1]);
            ++size_;
            items_.swap(new_items);
            capacity_ = new_capacity;
        }
        return &items_[new_item];
    }
    
    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(pos >= begin() && pos <= end());
        size_t new_item = pos - items_.Get();
        if (size_ < capacity_) {
            for (size_t i = size_; i > new_item; --i) {
                items_[i] = std::move(items_[i - 1]);
            }
            items_[new_item] = std::move(value);
            ++size_;
        } else if (size_ == capacity_) {
            size_t new_capacity = (capacity_ == 0 ? 1 : capacity_ * 2);
            ArrayPtr<Type> new_items(new Type[new_capacity]{});
            for (size_t i = 0; i < new_item; ++i) {
                new_items[i] = std::move(items_[i]);
            }
            new_items[new_item] = std::move(value);
            for (size_t i = new_item; i < size_; ++i) {
                new_items[i + 1] = std::move(items_[i]);
            }
            ++size_;
            items_.swap(new_items);
            capacity_ = new_capacity;
        }
        return &items_[new_item];
    }
    

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        if(size_) {
            --size_;
        }
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos <= end());
        size_t next = pos - items_.Get();
        for (size_t i = next; i < size_ - 1; ++i) {
            items_[i] = std::move(items_[i + 1]);
        }
        --size_;
        return &items_[next]; 
    }
    

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
        items_.swap(other.items_);
    }
    

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0 ? true : false;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("oops");
        }
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("oops");
        }
        return items_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0u;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size < size_) {
            size_ = new_size; 
        } else if (new_size > size_ && new_size < capacity_) {
            for (size_t i = size_; i < new_size; ++i) {
                items_[i] = Type{};
            }
            size_ = new_size;
        } else if (new_size >= capacity_) {
            size_t new_capacity = std::max(new_size, (capacity_ * 2));
            ArrayPtr<Type> new_items(new Type[new_capacity]{});
            for (size_t i = 0; i < size_; ++i) {
                new_items[i] = std::move(items_[i]);
            }
            items_.swap(new_items);
            size_ = new_size;
            capacity_ = new_capacity;
        }
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return &items_[size_];
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return &items_[size_];
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return &items_[size_];
    }
    
private:
    ArrayPtr<Type> items_;
    size_t size_ = 0u;
    size_t capacity_ = 0u;
};


template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !std::lexicographical_compare(rhs.begin(), rhs.end(), lhs.begin(), lhs.end());
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(rhs.begin(), rhs.end(), lhs.begin(), lhs.end());
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
} 