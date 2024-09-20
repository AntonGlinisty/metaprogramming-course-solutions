#include <span>
#include <numeric>
#include <concepts>
#include <cstdlib>
#include <iterator>

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
    constexpr Span() noexcept {}

    template <typename It>
    explicit(extent != dynamic_extent)
    constexpr Span(It first, size_type count) : storage(count) {
        data() = std::to_address(first);
    }

    template <typename It, typename End>
    requires (!std::is_convertible_v<End, size_type>)
    explicit(extent != dynamic_extent)
    constexpr Span(It first, End last) : storage(static_cast<size_type>(last - first)) {
        data() = std::to_address(first);
    }

    template <size_t N>
    constexpr Span(std::type_identity_t<element_type> (&arr)[N]) noexcept
        : Span(static_cast<pointer>(arr), N) {}


    template <typename U, size_t N>
    constexpr Span(std::array<U, N>& arr) noexcept
        :Span(static_cast<pointer>(arr.data()),  N) {}

    template <typename U, size_t N>
    constexpr Span(const std::array<U, N>& arr) noexcept
        :Span(static_cast<const pointer>(arr.data()), N) {}

    template <typename R>
    explicit(extent != dynamic_extent)
    constexpr Span(R&& range) 
    : Span(std::ranges::data(range), std::ranges::size(range)) {}

    explicit(extent != dynamic_extent)
    constexpr Span(std::initializer_list<value_type> il) noexcept
    : Span(il.begin(), il.end()) {}    

    template <typename U, size_t N>
    explicit(extent != dynamic_extent && N == dynamic_extent)
    constexpr Span(const Span<U, N>& source) noexcept : storage(source.size()) {
        data() = source.data();
    }

    constexpr Span(const Span& other) noexcept = default;

    constexpr Span& operator=(const Span& other) noexcept = default;

    ~Span() = default;


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
        return *data();
    }

    constexpr reference Back() const {
        return *(data() + size() - 1);
    }

    constexpr reference at(size_type pos) const {
        if (pos >= size()) {
            throw std::out_of_range("Invalid argument in at() method");
        }
        return *(data() + pos);
    }

    constexpr reference operator[](size_type idx) const {
        return data()[idx];
    }

    constexpr pointer data() const noexcept {
        return data_;
    }


    // Observers
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
    constexpr Span<element_type, Count> first() const {
        return Span<element_type, Count>(data(), Count);
    }

    constexpr Span<element_type, dynamic_extent> first(size_type Count) const {
        return Span<element_type, dynamic_extent>(data(), Count);
    }

    template <size_t Count>
    constexpr Span<element_type, Count> last() const {
        return Span<element_type, Count>(data() + size() - Count, Count);
    }

    constexpr Span<element_type, dynamic_extent> last(size_type Count) const {
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
        if constexpr (Count == dynamic_extent) {
            return Span<element_type, size() - Count>(data() + Offset, size() - Count);
        }
        return Span<element_type, Count>(data() + Offset, Count);
    }

    constexpr Span<element_type, dynamic_extent> subspan(size_type Offset, size_type Count = dynamic_extent) const {
        if (Count == dynamic_extent) {
            return {data() + Offset, size() - Offset};
        }
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

