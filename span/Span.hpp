#include <iostream>
#include <ranges>
#include <span>
#include <numeric>
#include <concepts>
#include <cstdlib>
#include <iterator>
#include <type_traits>
#include <vector>
#include <limits>
#include <list>
#include <cassert>

constexpr size_t dynamic_extent = std::numeric_limits<size_t>::max();

template <size_t Extent>
class extent_storage {
  public:
    constexpr extent_storage(size_t) noexcept {}
    static constexpr size_t size() noexcept { return Extent; }
};

template <>
class extent_storage<dynamic_extent> {
  public:
    constexpr extent_storage(size_t size) noexcept : size_(size) {}
    constexpr size_t size() const noexcept { return size_; }

  private:
    size_t size_;
};

template <typename T, size_t Extent = dynamic_extent>
class Span {
  public:
    // Member Types
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;


    // Member constant
    static constexpr size_t extent = Extent;

  public:
    // Member functions
    constexpr Span() noexcept
    requires (Extent == 0 || Extent == dynamic_extent)
    : storage(0) {}

    template <std::contiguous_iterator It>
    explicit(Extent != dynamic_extent)
    constexpr Span(It first, size_type count)
    : data_(std::to_address(first)), storage(count) {
        if constexpr (Extent != dynamic_extent) {
            MPC_VERIFY(Extent == count);
        }
    }

    template <std::contiguous_iterator It, std::sized_sentinel_for<It> End>
    requires (!std::is_convertible_v<End, size_type>)
    explicit(Extent != dynamic_extent)
    constexpr Span(It first, End last) noexcept(noexcept(last - first))
    : data_(std::to_address(first)), storage(last - first) {
        if constexpr (Extent != dynamic_extent) {
            MPC_VERIFY(Extent == last - first);
        }
    }

    template <size_t N>
    requires (Extent == dynamic_extent || Extent == N)
    constexpr Span(std::type_identity_t<element_type> (&arr)[N]) noexcept
        : Span(static_cast<pointer>(arr), N) {}


    template <typename U, size_t N>
    requires (Extent == dynamic_extent || Extent == N)
    constexpr Span(std::array<U, N>& arr) noexcept
        : Span(static_cast<pointer>(arr.data()),  N) {}

    template <typename U, size_t N>
    requires (Extent == dynamic_extent || Extent == N)
    constexpr Span(const std::array<U, N>& arr) noexcept
        : Span(static_cast<const pointer>(arr.data()), N) {}

    template <typename R>
    requires (std::ranges::contiguous_range<R> && std::ranges::sized_range<R>)
    && (std::ranges::borrowed_range<R> || std::is_const_v<element_type>)
	&& (!std::is_array_v<std::remove_cvref_t<R>>)
    explicit(extent != dynamic_extent)
    constexpr Span(R&& range)
    : Span(std::ranges::data(range), std::ranges::size(range)) {
        if constexpr (extent != dynamic_extent) {
            MPC_VERIFY(Extent == std::ranges::size(range));
        }
    }

    explicit(Extent != dynamic_extent)
    constexpr Span(std::initializer_list<value_type> il) noexcept
    requires (std::is_const_v<element_type>)
    : Span(il.begin(), il.end()) {
        if constexpr (Extent != dynamic_extent) {
            MPC_VERIFY(Extent == il.size());
        }
    }    

    template <typename U, size_t N>
    requires (Extent == dynamic_extent || N == dynamic_extent || Extent == N)
    && (std::__is_array_convertible<element_type, U>::value)
    explicit(Extent != dynamic_extent && N == dynamic_extent)
    constexpr Span(const Span<U, N>& source) noexcept
    : data_(source.data()), storage(source.size()) {
        if constexpr (Extent != dynamic_extent) {
            MPC_VERIFY(Extent == source.size());
        }
    }

    constexpr Span(const Span& other) noexcept = default;

    constexpr Span& operator=(const Span& other) noexcept = default;

    ~Span() noexcept = default;


    // Iterators
    constexpr iterator begin() const noexcept {
        return data();
    }

    constexpr const_iterator cbegin() const noexcept {
        return const_pointer(begin());
    }

    constexpr iterator end() const noexcept {
        return data() + size();
    }

    constexpr const_iterator cend() const noexcept {
        return const_pointer(end());
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
        return reverse_iterator(cend());
    }


    // Element access
    constexpr reference Front() const {
        MPC_VERIFY(!empty());
        return *data();
    }

    constexpr reference Back() const {
        MPC_VERIFY(!empty());
        return *(data() + size() - 1);
    }

    constexpr reference at(size_type pos) const {
        if (pos >= size()) {
            throw std::out_of_range("Invalid argument in at() method");
        }
        return *(data() + pos);
    }

    constexpr reference operator[](size_type idx) const {
        MPC_VERIFY(idx < size());
        return data()[idx];
    }

    constexpr pointer Data() const noexcept {
        return data();
    }

    constexpr pointer data() const noexcept {
        return data_;
    }


    // Observers
    constexpr size_t Size() const noexcept {
        return size();
    }

    constexpr size_t size() const noexcept {
        return storage.size();
    }

    constexpr size_t size_bytes() const noexcept {
        return size() * sizeof(element_type);
    }

    constexpr bool empty() const noexcept {
        return size() == 0;
    }


    // Subviews
    template <size_t Count>
    constexpr Span<element_type, Count> First() const {
        MPC_VERIFY(Count <= size());
        return Span<element_type, Count>(data(), Count);
    }

    constexpr Span<element_type, dynamic_extent> First(size_type Count) const {
        MPC_VERIFY(Count <= size());
        return Span<element_type, dynamic_extent>(data(), Count);
    }

    template <size_t Count>
    constexpr Span<element_type, Count> Last() const {
        MPC_VERIFY(Count <= size());
        return Span<element_type, Count>(data() + size() - Count, Count);
    }

    constexpr Span<element_type, dynamic_extent> Last(size_type Count) const {
        MPC_VERIFY(Count <= size());
        return Span<element_type, dynamic_extent>(data() + size() - Count, Count);
    }

    template <size_t Offset, size_t Count>
    static constexpr size_t subspan_extent() {
        if constexpr (Count != dynamic_extent) {
            return Count;
        } else if constexpr (extent != dynamic_extent) {
            return extent - Offset;
        } else {
            return dynamic_extent;
        }
    }
    
    template <size_t Offset, size_t Count = dynamic_extent>
    constexpr Span<element_type, subspan_extent<Offset, Count>> subspan() const {
        MPC_VERIFY(Offset <= size());
        if constexpr (Count == dynamic_extent) {
            return Span<element_type, subspan_extent<Offset, Count>>(data() + Offset, size() - Offset);
        }
        MPC_VERIFY(Count <= size() - Offset);
        return Span<element_type, subspan_extent<Offset, Count>>(data() + Offset, Count);
    }

    constexpr Span<element_type, dynamic_extent> subspan(size_type Offset, size_type Count = dynamic_extent) const {
        MPC_VERIFY(Offset <= size());
        if (Count == dynamic_extent) {
            return {data() + Offset, size() - Offset};
        }
        MPC_VERIFY(Count <= size() - Offset);
        return {data() + Offset, Count};
    }

  private:
    pointer data_ = nullptr;
    [[no_unique_address]] extent_storage<Extent> storage;
};

template <typename T, size_t N>
Span<const std::byte, N == dynamic_extent ? dynamic_extent : N * sizeof(T)> as_bytes(Span<T, N> s) noexcept {
    return {reinterpret_cast<const std::byte*>(s.data(), s.size_bytes()), s.size_bytes()};
}

template <typename T, size_t N>
Span<std::byte, N == dynamic_extent ? dynamic_extent : N * sizeof(T)> as_writable_bytes(Span<T, N> s) noexcept {
    return {reinterpret_cast<std::byte*>(s.data(), s.size_bytes()), s.size_bytes()};
}

// Deduction guides

template <typename It, typename EndOrSize>
Span(It, EndOrSize) -> Span<std::remove_reference_t<std::iter_reference_t<It>>>;

template <typename T, size_t N>
Span(T(&)[N]) -> Span<T, N>;

template <typename T, size_t N>
Span(std::array<T, N>&) -> Span<T, N>;

template <typename T, size_t N>
Span(const std::array<T, N>&) -> Span<const T, N>;

template <typename R>
Span(R&&) -> Span<std::remove_reference_t<std::ranges::range_reference_t<R>>>;
