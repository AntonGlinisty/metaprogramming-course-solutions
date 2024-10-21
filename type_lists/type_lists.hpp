#pragma once

#include <concepts>

#include <cstddef>
#include <type_traits>
#include <type_tuples.hpp>



namespace type_lists
{

template<class TL>
concept TypeSequence =
    requires {
        typename TL::Head;
        typename TL::Tail;
    };

struct Nil {};

template<class TL>
concept Empty = std::derived_from<TL, Nil>;

template<class TL>
concept TypeList = Empty<TL> || TypeSequence<TL>;

template <typename T, typename TL>
struct Cons {
    using Head = T;
    using Tail = TL;
};


namespace detail {

    // FromTuple

    template <typename... Types>
    struct FromTupleHelper {};

    template <typename... Types>
    struct FromTupleHelper<type_tuples::TTuple<Types...>> : Nil {};

    template <typename T, typename... Types>
    struct FromTupleHelper<type_tuples::TTuple<T, Types...>> :
        Cons<T, FromTupleHelper<type_tuples::TTuple<Types...>>> {};


    // ToTuple

    template <TypeList TL, typename... Accumulated>
    struct ToTupleHelper {
        using Type = typename ToTupleHelper<typename TL::Tail, Accumulated..., typename TL::Head>::Type;
    };

    template <Empty TL, typename... Accumulated>
    struct ToTupleHelper<TL, Accumulated...> {
        using Type = type_tuples::TTuple<Accumulated...>;
    };


    // Take

    template <std::size_t N, TypeList TL>
    struct TakeHelper {
        using Type = Cons<typename TL::Head, typename TakeHelper<N - 1, typename TL::Tail>::Type>;
    };

    template <std::size_t N, Empty TL>
    struct TakeHelper<N, TL> {
        using Type = Nil;
    };

    template <TypeList TL>
    struct TakeHelper<0, TL> {
        using Type = Nil;
    };

    
    // Drop

    template <std::size_t N, TypeList TL>
    struct DropHelper {
        using Type = typename DropHelper<N - 1, typename TL::Tail>::Type;
    };

    template <std::size_t N, Empty TL>
    struct DropHelper<N, TL> {
        using Type = Nil;
    };

    template <TypeList TL>
    struct DropHelper<0, TL> {
        using Type = TL;
    };
    
    
    // Replicate

    template <std::size_t N, typename T>
    struct ReplicateHelper {
        using Type = Cons<T, typename ReplicateHelper<N - 1, T>::Type>;
    };

    template <typename T>
    struct ReplicateHelper<0, T> {
        using Type = Nil;
    };

    
    // Map

    template <template <typename> typename F, TypeList TL>
    struct MapHelper {
        using Head = F<typename TL::Head>;
        using Tail = MapHelper<F, typename TL::Tail>;
    };

    template <template <typename> typename F, Empty TL>
    struct MapHelper<F, TL> : Nil {};


    // Filter

    template <template <typename> typename P, TypeList TL>
    struct FilterHelper : FilterHelper<P, typename TL::Tail> {};

    template <template <typename> typename P, TypeList TL>
    requires (P<typename TL::Head>::Value)
    struct FilterHelper<P, TL> {
        using Head = TL::Head;
        using Tail = FilterHelper<P, typename TL::Tail>;
    };

    template <template <typename> typename P, Empty TL>
    struct FilterHelper<P, TL> : Nil{};


    // Cycle

    template <TypeList Current, TypeList Source>
    struct CycleHelper {
        using Head = Current::Head;
        using Tail = CycleHelper<typename Current::Tail, Source>;
    };

    template <Empty Current, TypeList Source>
    struct CycleHelper<Current, Source> {
        using Head = Source::Head;
        using Tail = CycleHelper<typename Source::Tail, Source>; 
    };

    template <Empty Current, Empty Source>
    struct CycleHelper<Current, Source> : Nil {};


    // Scanl

    template <template <typename, typename> typename OP, typename T, TypeList TL>
    struct ScanlHelper {
        using Head = OP<T, typename TL::Head>;
        using Tail = ScanlHelper<OP, Head, typename TL::Tail>;
    };

    template <template <typename, typename> typename OP, typename T, Empty TL>
    struct ScanlHelper<OP, T, TL> : Nil {};


    // Foldl

    template <template <typename, typename> typename OP, typename T, TypeList TL>
    struct FoldlHelper {
        using Type = typename FoldlHelper<OP, OP<T, typename TL::Head>, typename TL::Tail>::Type;
    };

    template <template <typename, typename> typename OP, typename T, Empty TL>
    struct FoldlHelper<OP, T, TL> : Nil {
        using Type = T;
    };


    // Foldr

    template <template <typename, typename> typename OP, typename T, TypeList TL>
    struct FoldrHelper {
        using Type = OP<typename TL::Head, typename FoldrHelper<OP, T, typename TL::Tail>::Type>;
    };

    template <template <typename, typename> typename OP, typename T, Empty TL>
    struct FoldrHelper<OP, T, TL> : Nil {
        using Type = T;
    };


    // Zip2

    template <TypeList L, TypeList R>
    struct Zip2Helper {
        using Head = type_tuples::TTuple<typename L::Head, typename R::Head>;
        using Tail = Zip2Helper<typename L::Tail, typename R::Tail>;
    };

    template <TypeList L, Empty R>
    struct Zip2Helper<L, R> : Nil {};

    template <Empty L, TypeList R>
    struct Zip2Helper<L, R> : Nil {};

    template <Empty L, Empty R>
    struct Zip2Helper<L, R> : Nil {};


    // Zip

    template <typename... TL>
    struct ZipHelper {};

    template <TypeList... TL>
    requires (!Empty<TL> && ...)
    struct ZipHelper<TL...> {
        using Head = type_tuples::TTuple<typename TL::Head...>;
        using Tail = ZipHelper<typename TL::Tail...>;
    };
}; // namespace ::detail

// FromTuple
template <typename TT>
using FromTuple = detail::FromTupleHelper<TT>;

// ToTuple
template <TypeList TL>
using ToTuple = typename detail::ToTupleHelper<TL>::Type;

// Repeat
template <typename T>
struct Repeat {
    using Head = T;
    using Tail = Repeat<T>;
};

// Take
template <std::size_t N, TypeList TL>
using Take = typename detail::TakeHelper<N, TL>::Type;

// Drop
template <std::size_t N, TypeList TL>
using Drop = typename detail::DropHelper<N, TL>::Type;

// Replicate
template <std::size_t N, typename T>
using Replicate = typename detail::ReplicateHelper<N, T>::Type;

// Map
template <template <typename> typename F, TypeList TL>
using Map = detail::MapHelper<F, TL>;

// Filter
template <template <typename> typename P, TypeList TL>
using Filter = detail::FilterHelper<P, TL>;

// Iterate
template <template <typename> typename F, typename T>
struct Iterate {
    using Head = T;
    using Tail = Iterate<F, F<T>>;
};

// Cycle
template <TypeList TL>
using Cycle = detail::CycleHelper<TL ,TL>;

// Inits
template <TypeList TL, typename... Types>
struct Inits {
    using Head = FromTuple<type_tuples::TTuple<Types...>>;
    using Tail = Inits<typename TL::Tail, Types..., typename TL::Head>;
};

template <Empty TL, typename... Types>
struct Inits<TL, Types...> {
    using Head = FromTuple<type_tuples::TTuple<Types...>>;
    using Tail = Nil;
};

// Tails
template<TypeList TL>
struct Tails {
    using Head = TL;
    using Tail = Tails<typename TL::Tail>;
};

template<Empty TL>
struct Tails<TL> {
    using Head = Nil;
    using Tail = Nil;
};

// Scanl
template <template <typename, typename> typename OP, typename T, TypeList TL>
using Scanl = Cons<T, detail::ScanlHelper<OP, T, TL>>;

// Foldl
template <template <typename, typename> typename OP, typename T, TypeList TL>
using Foldl = typename detail::FoldlHelper<OP, T, TL>::Type;

// Foldr
template <template <typename, typename> typename OP, typename T, TypeList TL>
using Foldr = typename detail::FoldrHelper<OP, T, TL>::Type;

// Zip2
template <TypeList L, TypeList R>
using Zip2 = detail::Zip2Helper<L, R>;

// Zip
template <TypeList... TL>
using Zip = detail::ZipHelper<TL...>;

// Your fun, fun metaalgorithms :)

} // namespace type_lists
