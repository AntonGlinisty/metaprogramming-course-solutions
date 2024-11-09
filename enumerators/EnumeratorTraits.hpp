#include <array>
#include <cstddef>
#include <type_traits>
#include <cstdint>
#include <string>
#include <tuple>
#include <iostream>
#include <concepts>
#include <string_view>
#include <utility>
#include <ranges>


namespace detail {

template <auto EnumObject>
constexpr std::string_view helper() { return __PRETTY_FUNCTION__; }

constexpr int IsEnumField(const std::string_view& pretty) {
    constexpr std::string_view pattern = "EnumObject = ("; 
    return pretty.find(pattern) == std::string_view::npos;
}

template <class Enum, int64_t Number, int64_t Decrement>
struct Storage {
    std::array<int64_t, Number> arr;
    std::size_t real_size = 0;

    constexpr Storage<Enum, Number + 1, Decrement> operator+(std::size_t) const {
        Storage<Enum, Number + 1, Decrement> result;
        
        std::copy(arr.begin(), arr.begin() + real_size, result.arr.begin());
        result.real_size = real_size;

        if constexpr(IsEnumField(helper<static_cast<Enum>(Number + Decrement)>())) {
            result.arr[real_size] = Number + Decrement;
            ++result.real_size;
        }
        return result;
    }
};

template <class Enum, std::size_t MAXN>
struct CommonHelpers {
    using UnderType = std::underlying_type_t<Enum>;

    static constexpr int64_t UPPERSIZE = std::min(static_cast<int>(MAXN), static_cast<int>(std::numeric_limits<UnderType>::max()));
    static constexpr int64_t LOWERSIZE = std::max(static_cast<int>(-MAXN), static_cast<int>(std::numeric_limits<UnderType>::min()));

    static constexpr std::size_t SIZE = UPPERSIZE - LOWERSIZE + 1;

    template <int64_t UpperBound = UPPERSIZE, int64_t LowerBound = LOWERSIZE>
    struct SizeHelper {
        static constexpr size_t Cnt
            = IsEnumField(helper<static_cast<Enum>(UpperBound)>())
            + IsEnumField(helper<static_cast<Enum>(LowerBound)>())
            + SizeHelper<UpperBound - 1, LowerBound + 1>::Cnt;
    };

    template <int64_t UpperBound, int64_t LowerBound>
    requires (UpperBound == LowerBound + 1)
    struct SizeHelper<UpperBound, LowerBound> {
        static constexpr size_t Cnt
            = IsEnumField(helper<static_cast<Enum>(UpperBound)>())
            + IsEnumField(helper<static_cast<Enum>(LowerBound)>());
    };

    template <int64_t Bound>
    struct SizeHelper<Bound, Bound> {
        static constexpr size_t Cnt
            = IsEnumField(helper<static_cast<Enum>(Bound)>());
    };

    template<std::size_t... indices>
    static constexpr auto generate(std::index_sequence<indices...>) {
        return ((Storage<Enum, 0, LOWERSIZE>{} + 0) + ... + indices);
    }

    static constexpr Storage<Enum, SIZE - 1, LOWERSIZE> generator = generate(std::make_index_sequence<SIZE - 1>{});

};
} // namespace ::detail 

template <class Enum, std::size_t MAXN = 512>
requires std::is_enum_v<Enum>
struct EnumeratorTraits {
    static constexpr std::size_t size() noexcept {
        return detail::CommonHelpers<Enum, MAXN>::template SizeHelper<>::Cnt;
    }

    static constexpr Enum at(std::size_t index) noexcept {
        return static_cast<Enum>(detail::CommonHelpers<Enum, MAXN>::generator.arr[index]);
    };

    // static constexpr std::string_view nameAt(std::size_t i) noexcept;
};