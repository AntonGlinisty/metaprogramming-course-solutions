#pragma once


#include "type_lists.hpp"
#include "type_tuples.hpp"
#include <concepts>
#include <value_types.hpp>

using type_lists::Cons;
using type_lists::Iterate;
using type_lists::Filter;
using value_types::ValueTag;

namespace detail {
    template <typename T>
    using AddOne = value_types::ValueTag<T::Value + 1>;

    template <typename T, typename U>
    using AddTwoNums = value_types::ValueTag<T::Value + U::Value>;

    template <template <typename, typename> typename F, typename T, typename U>
    struct FibHelper {
        using Head = F<T, U>;
        using Tail = FibHelper<F, U, Head>;
    };

    template <typename T>
    concept GreaterThanOne = requires (T) {
        requires(T::Value > 1 == true);
    };

    template <typename Dividend, typename Divisor>
    struct IsPrimeHelper {
        static constexpr bool Value = (Dividend::Value % Divisor::Value != 0) &&
            IsPrimeHelper<Dividend, ValueTag<Divisor::Value - 1>>::Value;
    };

    template <typename Dividend>
    struct IsPrimeHelper<Dividend, ValueTag<1>> {
        static constexpr bool Value = true;
    };

    template <typename T>
    struct IsPrime {
        static constexpr bool Value = IsPrimeHelper<T, ValueTag<T::Value - 1>>::Value;
    };

    template <>
    struct IsPrime<ValueTag<1>> {
        static constexpr bool Value = false;
    };

    template <>
    struct IsPrime<ValueTag<0>> {
        static constexpr bool Value = false;
    };

}; // namespace ::detail

using Nats = Iterate<detail::AddOne, value_types::ValueTag<0>>;
using Fib = Cons<ValueTag<0>, Cons<ValueTag<1>,
    detail::FibHelper<detail::AddTwoNums, ValueTag<0>, ValueTag<1>>>>;
using Primes = Filter<detail::IsPrime, Nats>;
