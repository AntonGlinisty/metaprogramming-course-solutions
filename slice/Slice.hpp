#include <cstddef>
#include <span>
#include <concepts>
#include <cstdlib>
#include <array>
#include <iterator>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <ranges>

inline constexpr ptrdiff_t dynamic_stride = -1;
inline constexpr size_t dynamic_extent = std::numeric_limits<std::size_t>::max();

namespace helper {
template <typename T, size_t extent, ptrdiff_t stride>
struct fields_storage {
    T* data_;

    constexpr fields_storage(T* data, size_t, ptrdiff_t) noexcept
 	: data_(data) {}
    constexpr T* Data() const { return data_; }
	constexpr size_t Size() const  { return extent; }
	constexpr ptrdiff_t Stride() const { return stride; }
};

template <typename T, ptrdiff_t stride>
struct fields_storage<T, dynamic_extent, stride> {
    T* data_;
	size_t size_;

    constexpr fields_storage(T* data, size_t size, ptrdiff_t) noexcept
 	: data_(data), size_(size) {}
    constexpr T* Data() const { return data_; }
	constexpr size_t Size() const  { return size_; }
	constexpr ptrdiff_t Stride() const { return stride; }
};

template <typename T, size_t extent>
struct fields_storage<T, extent, dynamic_stride> {
    T* data_;
	ptrdiff_t stride_;

    constexpr fields_storage(T* data, size_t, ptrdiff_t stride) noexcept
 	: data_(data), stride_(stride) {}
    constexpr T* Data() const { return data_; }
	constexpr size_t Size() const  { return extent; };
	constexpr ptrdiff_t Stride() const { return stride_; };
};

template <typename T>
struct fields_storage<T, dynamic_extent, dynamic_stride> {
    T* data_;
	size_t size_;
	ptrdiff_t stride_;

    constexpr fields_storage(T* data, size_t size, ptrdiff_t stride) noexcept
 	: data_(data), size_(size), stride_(stride) {}
    constexpr T* Data() const { return data_; }
	constexpr size_t Size() const  { return size_; };
	constexpr ptrdiff_t Stride() const { return stride_; };
};
}

template <typename T, size_t extent = dynamic_extent, ptrdiff_t stride = 1>
class Slice {
  public:
	template <bool IsConst>
  	class CommonIterator;

	// Member Types
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
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
	constexpr Slice() noexcept
    : storage(0, 0) {}

	template <std::contiguous_iterator It>
	constexpr Slice(It first, size_type count, difference_type skip = stride)
    : storage(std::to_address(first), count, skip) {
        if constexpr (extent != dynamic_extent) {
            // MPC_VERIFY(extent == count);
        }
    }

    template <std::contiguous_iterator It, std::sized_sentinel_for<It> End>
    requires (!std::is_convertible_v<End, size_type>)
    constexpr Slice(It first, End last, difference_type skip)
    noexcept(noexcept(last - first))
    : storage(std::to_address(first), last - first, skip) {
        if constexpr (extent != dynamic_extent) {
            MPC_VERIFY(extent == last - first);
        }
    }

    template <size_t N>
    requires (extent == N || extent == dynamic_extent)
    constexpr Slice(std::type_identity_t<element_type> (&arr)[N], ptrdiff_t step = stride) noexcept
        : Slice(static_cast<pointer>(arr), N, step) {}

    template <typename U, size_t N>
    requires (extent == N || extent == dynamic_extent)
    constexpr Slice(std::array<U, N>& arr, ptrdiff_t step = stride) noexcept
        : Slice(static_cast<pointer>(arr.data()), N, step) {}

    template <typename U, size_t N>
    requires (extent == N || extent == dynamic_extent)
    constexpr Slice(const std::array<U, N>& arr, ptrdiff_t step = stride) noexcept
        : Slice(static_cast<const pointer>(arr.data()), N, step) {}

    template <typename R>
    requires (std::ranges::contiguous_range<R> && std::ranges::sized_range<R>)
    && (std::ranges::borrowed_range<R> || std::is_const_v<element_type>)
	&& (!std::is_array_v<std::remove_cvref_t<R>>)
    constexpr Slice(R&& range, ptrdiff_t step = stride)
    : Slice(std::ranges::data(range), std::ranges::size(range), step) {
        if constexpr (extent != dynamic_extent) {
            // MPC_VERIFY(extent == std::ranges::size(range));
        }
    }

    template <typename U, size_type E, difference_type S>
    constexpr Slice(const Slice<U, E, S>& source) noexcept
    : storage(source.Data(), source.Size(), source.Stride()) {
        if constexpr (extent != dynamic_extent) {
            // MPC_VERIFY(extent == source.Size());
        }
    }
    
    constexpr Slice(const Slice& other) noexcept = default;

    constexpr Slice& operator=(const Slice& other) noexcept = default;

    ~Slice() noexcept = default;


    template <typename U, size_type E, difference_type S>
    constexpr bool operator==(const Slice<U, E, S>& other) const {
        return Data() == other.Data()
            && Size() == other.Size()
            && Stride() == other.Stride();
    }


    // Iterators
    constexpr iterator begin() const noexcept {
        return iterator(Data(), this);
    }

    constexpr iterator end() const noexcept {
        return iterator(Data() + Size() * Stride(), this);
    }

    constexpr const_iterator cbegin() const noexcept {
        return const_iterator(Data(), this);
    }

    constexpr const_iterator cend() const noexcept {
        return const_iterator(Data() + Size() * Stride(), this);
    }

    constexpr reverse_iterator rbegin() const noexcept {
        return reverse_iterator(end());
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
        // MPC_VERIFY(idx < Size());
        return begin().operator[](idx);
    }

    constexpr reference at(size_type pos) const {
        if constexpr (pos >= Size()) {
            throw std::out_of_range("Invalid argument in at() method");
        }
        return operator[](pos);
    }

    constexpr reference Front() const {
        MPC_VERIFY(!empty());
        return operator[](0);
    }

    constexpr reference Back() const {
        MPC_VERIFY(!empty());
        return operator[](Size() - 1);
    }

    constexpr pointer Data() const {
		return storage.Data();
	}

    constexpr difference_type Stride() const {
		return storage.Stride();
	}


    // Observers
    constexpr size_type Size() const {
		return storage.Size();
	}

    constexpr bool empty() const noexcept {
        return Size() == 0;
    }


    // Subviews
	constexpr Slice<element_type, dynamic_extent, stride>
    First(size_type count) const {
        // MPC_VERIFY(count <= Size());
        return {Data(), count, Stride()};
    }

	template <size_type count>
	constexpr Slice<element_type, count, stride> First() const {
        // MPC_VERIFY(count <= Size());
        return {Data(), count, Stride()};
    }

	constexpr Slice<element_type, dynamic_extent, stride>
    Last(size_type count) const {
        // MPC_VERIFY(count <= Size());
        return {Data() + (Size() - count) * Stride(), count, Stride()};
    }

	template <size_type count>
	constexpr Slice<element_type, count, stride>
    Last() const {
        // MPC_VERIFY(count <= Size());
        return {Data() + (Size() - count) * Stride(), count, Stride()};
    }

	constexpr Slice<element_type, dynamic_extent, stride>
    DropFirst(size_type count) const {
        // MPC_VERIFY(count <= Size());
        return {Data() + count * Stride(), Size() - count, Stride()};
    }

	template <size_type count>
	constexpr Slice<element_type, extent == dynamic_extent ? dynamic_extent : extent - count, stride>
    DropFirst() const {
        // MPC_VERIFY(count <= Size());
        return {Data() + count * Stride(), Size() - count, Stride()};
    }

	constexpr Slice<element_type, dynamic_extent, stride>
	DropLast(size_type count) const {
        // MPC_VERIFY(count <= Size());
        return {Data(), Size() - count, Stride()};
    }

	template <size_type count>
	constexpr Slice<element_type, extent == dynamic_extent ? dynamic_extent : extent - count, stride>
	DropLast() const {
        // MPC_VERIFY(count <= Size());
        return {Data(), Size() - count, Stride()};
    }

	auto Skip(ptrdiff_t skip) -> Slice<element_type, dynamic_extent, dynamic_stride> const {
        return {Data(), (Size() + skip - 1) / skip, Stride() * skip};
    }

	template <ptrdiff_t skip>
    Slice<element_type, extent == dynamic_extent ? dynamic_extent : (extent + skip - 1) / skip,
            stride == dynamic_stride ? dynamic_stride : stride * skip> Skip() const {
        return {Data(), (Size() + skip - 1) / skip, Stride() * skip};
    }

private:
	helper::fields_storage<T, extent, stride> storage;
};

template <typename It, typename EndOrSize>
Slice(It, EndOrSize, ptrdiff_t) ->
Slice<std::remove_reference_t<std::iter_reference_t<It>>, dynamic_extent, dynamic_stride>;

template <typename T, size_t N>
Slice(T(&)[N]) -> Slice<T, N, 1>;

template <typename T, size_t N>
Slice(std::array<T, N>&) -> Slice<T, N, 1>;

template <typename T, size_t N>
Slice(const std::array<T, N>&) -> Slice<const T, N, 1>;

template <typename R>
Slice(R&&) -> Slice<std::remove_reference_t<std::ranges::range_reference_t<R>>>;

template <typename T, size_t extent, ptrdiff_t stride>
template <bool IsConst>
class Slice<T, extent, stride>::CommonIterator {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = std::conditional_t<IsConst, const T, T>;
    using pointer = std::conditional_t<IsConst, const T, T>*;
    using reference = std::conditional_t<IsConst, const T, T>&;
    using difference_type = ptrdiff_t;

    CommonIterator() = default;

    CommonIterator(T* ptr, const Slice<T, extent, stride>* slice) noexcept
    : data_(ptr), slice_(slice) {}
    
    CommonIterator(const CommonIterator& other) = default;

    reference operator*() const {
        return *data_;
    }

    pointer operator->() const {
        return data_;
    }

    CommonIterator& operator++() {
        data_ += slice_->Stride();
        return *this;
    }

    CommonIterator operator++(int) {
        CommonIterator old = *this;
        operator++();
        return old;
    }

    CommonIterator& operator--() {
        data_ -= slice_->Stride();
        return *this;
    }

    CommonIterator operator--(int) {
        CommonIterator old = *this;
        operator--();
        return old;
    }

    CommonIterator& operator+=(difference_type value) {
        data_ += slice_->Stride() * value;
        return *this;
    }

    CommonIterator operator+(difference_type value) const {
        CommonIterator it = *this;
        return it += value;
    }

    friend CommonIterator operator+(difference_type value, CommonIterator<IsConst> iter) {
        return iter + value;
    }

    CommonIterator& operator-=(difference_type value) {
        data_ -= slice_->Stride() * value;
        return *this;
    }

    CommonIterator operator-(difference_type value) const {
        CommonIterator it = *this;
        return it -= value;
    }

    friend CommonIterator operator-(difference_type value, CommonIterator<IsConst> iter) {
        return iter - value;
    }


    reference operator[](difference_type value) const {
        return *(data_ + slice_->Stride() * value);
    }

    difference_type operator-(const CommonIterator& other) const {
        return (data_ - other.data_) / slice_->Stride();
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
    const Slice<T, extent, stride>* slice_;
};


// int main() {
// 	std::array<int, 42> arr{};
//     std::iota(arr.begin(), arr.end(), 0);


//     Slice all(arr);
// }