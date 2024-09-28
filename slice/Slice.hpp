#include <cstddef>
#include <span>
#include <concepts>
#include <cstdlib>
#include <array>
#include <iterator>
#include <iostream>
#include <algorithm>
#include <numeric>

inline constexpr std::ptrdiff_t dynamic_stride = -1;

namespace helper {
template <typename T, size_t extent, std::ptrdiff_t stride>
struct fields_storage {
	constexpr fields_storage(T* data, size_t, std::ptrdiff_t) noexcept
    : data_(data) {}

    constexpr bool operator==(const fields_storage& other) const {
        return Data() == other.Data() && Size() == other.Size() && Stride() == other.Stride();
    }

    constexpr T* Data() const { return data_; }
	constexpr size_t Size() const  { return extent; }
	constexpr std::ptrdiff_t Stride() const  { return stride; }

  private:
    T* data_;
};

template <typename T, std::ptrdiff_t stride>
struct fields_storage<T, std::dynamic_extent, stride> {
	constexpr fields_storage(T* data, size_t size, std::ptrdiff_t) noexcept
	: data_(data), size_(size) {}

    constexpr bool operator==(const fields_storage& other) const {
        return Data() == other.Data() && Size() == other.Size() && Stride() == other.Stride();
    }

    constexpr T* Data() const { return data_; }
	constexpr size_t Size() const  { return size_; }
	constexpr std::ptrdiff_t Stride() const  { return stride; }
    
  private:
    T* data_;
	size_t size_;
};

template <typename T, size_t extent>
struct fields_storage<T, extent, dynamic_stride> {
	constexpr fields_storage(T* data, size_t, std::ptrdiff_t stride) noexcept
	: data_(data), stride_(stride) {}

    constexpr bool operator==(const fields_storage& other) const {
        return Data() == other.Data() && Size() == other.Size() && Stride() == other.Stride();
    }

    constexpr T* Data() const { return data_; }
	constexpr size_t Size() const  { return extent; };
	constexpr std::ptrdiff_t Stride() const  { return stride_; };

  private:
    T* data_;
	std::ptrdiff_t stride_;
};

template <typename T>
struct fields_storage<T, std::dynamic_extent, dynamic_stride> {
	constexpr fields_storage(T* data, size_t size, std::ptrdiff_t stride) noexcept
	: data_(data), size_(size), stride_(stride) {}

    constexpr bool operator==(const fields_storage& other) const {
        return Data() == other.Data() && Size() == other.Size() && Stride() == other.Stride();
    }

    constexpr T* Data() const { return data_; }
	constexpr size_t Size() const  { return size_; };
	constexpr std::ptrdiff_t Stride() const  { return stride_; };

  private:
    T* data_;
	size_t size_;
	std::ptrdiff_t stride_;
};
}

template <typename T, size_t extent = std::dynamic_extent, std::ptrdiff_t stride = 1>
class Slice {
  public:
	template <bool IsConst>
  	class CommonIterator;

	// Member Types
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = CommonIterator<false>;
    using const_iterator = CommonIterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  public:
    // Member functions
	constexpr Slice() noexcept : storage(0, 0) {}

	template <std::contiguous_iterator It>
	Slice(It first, size_t count, std::ptrdiff_t skip)
    : storage{&(*first), count/skip, skip} {}

    template <size_t E, std::ptrdiff_t S>
	Slice(pointer data, size_t ext, std::ptrdiff_t str)
    : storage{data, ext, str} {}

    template <typename U, size_t N>
    constexpr Slice(std::array<U, N>& arr) noexcept
        : Slice(arr.begin(), N, 1) {}

    template <size_t N>
    constexpr Slice(const std::array<T, N>& arr) noexcept
        : Slice(arr.cbegin(), N, 1) {}

    template <typename U, size_t E, std::ptrdiff_t S>
    constexpr Slice(const Slice<U, E, S>& source) noexcept
    : storage(source.Data(), source.Size(), source.Stride()) {
    }

    constexpr Slice(const Slice& other) noexcept = default;

    constexpr Slice& operator=(const Slice& other) noexcept = default;

    ~Slice() noexcept = default;

	// Data, Size, Stride, begin, end, casts, etc...

	

	constexpr size_t Stride() const noexcept {
		return storage.Stride();
	}

    constexpr bool operator==(const Slice& other) const {
        return storage == other.storage;
    }


    // Iterators
    constexpr iterator begin() const noexcept {
        return iterator(Data());
    }

    constexpr iterator end() const noexcept {
        return iterator(Data() + Size());
    }

    constexpr const_iterator cbegin() const noexcept {
        return const_iterator(begin());
    }

    constexpr const_iterator cend() const noexcept {
        return const_iterator(end);
    }

    constexpr reverse_iterator rbegin() const noexcept {
        return reverse_iterator(end);
    }

    constexpr reverse_iterator rend() const noexcept {
        return reverse_iterator(begin());
    }

    constexpr const_reverse_iterator crbegin() const noexcept {
        return reverse_iterator(cbegin());
    }

    constexpr const_reverse_iterator crend() const noexcept {
        return reverse_iterator(rend());
    }


    // Element access
    constexpr reference operator[](size_type idx) const {
        return begin()[idx];
    }

    constexpr reference at(size_type pos) const {
        if (pos >= Size()) {
            throw std::out_of_range("Invalid argument in at() method");
        }
        return operator[](pos);
    }

    constexpr reference Front() const {
        return operator[](0);
    }

    constexpr reference Back() const {
        return operator[](Size() - 1);
    }

    constexpr pointer Data() const {
		return storage.Data();
	}


    // Observers
    constexpr size_type Size() const {
		return storage.Size();
	}

    constexpr bool empty() const noexcept {
        return Size() == 0;
    }


	// Slice<T, dynamic_extent, stride>
	// First(size_t count) const;

	// template <size_t count>
	// Slice<T, /*?*/, stride>
	// First() const;

	// Slice<T, dynamic_extent, stride>
	// Last(size_t count) const;

	// template <size_t count>
	// Slice<T, /*?*/, stride>
	// Last() const;

	// Slice<T, dynamic_extent, stride>
	// DropFirst(size_t count) const;

	// template <size_t count>
	// Slice<T, /*?*/, stride>
	// DropFirst() const;

	// Slice<T, dynamic_extent, stride>
	// DropLast(size_t count) const;

	// template <size_t count>
	// Slice<T, /*?*/, stride>
	// DropLast() const;

	// auto Skip(ptrdiff_t skip) const {
    //     return Slice<T, extent * stride / skip, skip>(Data(), extent * stride / skip, skip);
    // }

	// template <ptrdiff_t skip>
	// Slice<T, >
	// Skip() const {
    //     return Slice<T, extent / skip, skip>(Data(), extent / skip, skip);
    // }

private:
	helper::fields_storage<T, extent, stride> storage;
};

template <typename T, size_t extent, std::ptrdiff_t stride>
template <bool IsConst>
class Slice<T, extent, stride>::CommonIterator {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = std::conditional_t<IsConst, const T, T>;
    using pointer = std::conditional_t<IsConst, const T, T>*;
    using reference = std::conditional_t<IsConst, const T, T>&;
    using difference_type = std::ptrdiff_t;

    CommonIterator(T* ptr) noexcept : data_(ptr) {}
    CommonIterator(const CommonIterator& other) = default;

    value_type operator*() const {
        return *data_;
    }

    pointer operator->() const {
        return data_;
    }

    CommonIterator& operator++() {
        data_ += stride;
        return *this;
    }

    CommonIterator operator++(int) {
        CommonIterator old = *this;
        operator++();
        return old;
    }

    CommonIterator& operator--() {
        data_ -= stride;
        return *this;
    }

    CommonIterator operator--(int) {
        CommonIterator old = *this;
        operator--();
        return old;
    }

    CommonIterator& operator+=(int value) {
        data_ += stride * value;
        return *this;
    }

    CommonIterator operator+(int value) const {
        CommonIterator it = *this;
        return it += value;
    }

    CommonIterator& operator-=(int value) {
        data_ -= stride * value;
        return *this;
    }

    CommonIterator operator-(int value) const {
        CommonIterator it = *this;
        return it -= value;
    }

    reference operator[](int value) const {
        return *(data_ + stride * value);
    }

    difference_type operator-(const CommonIterator& other) const {
        return (data_ - other.data_) / stride;
    }

    bool operator<(const CommonIterator& other) const {
        return data_ < other.data_;
    }

    bool operator>(const CommonIterator& other) const {
        return data_ > other.data_;
    }

    bool operator<=(const CommonIterator& other) const {
        return !(*this > other);
    }

    bool operator>=(const CommonIterator& other) const {
        return !(*this < other);
    }

    bool operator==(const CommonIterator& other) const {
        return (!(*this < other) && !(*this > other));
    }

    bool operator!=(const CommonIterator& other) const {
        return ((*this < other) || (*this > other));
    }

  private:
    pointer data_;
};

// int main() {
// 	std::array<int, 42> arr{};
//     std::iota(arr.begin(), arr.end(), 0);


//     Slice all(arr);
// }