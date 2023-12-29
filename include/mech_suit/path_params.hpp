#pragma once
#include <utility>
#include <stddef.h>
#include <array>

#include "mech_suit/meta_string.hpp"

namespace mech_suit {
    namespace detail {

        template<typename T, meta::string Name>
        struct named_param {
            using type = T;
            static constexpr auto name = Name;
        };

        template <char... C>
        struct param_name {
            static_assert(false, "Parameter is unamed! This is not legal");
        };

        template <char... C>
        struct param_name<'(', C...> {

            template <char ToFind, char... Cs>
            static constexpr size_t find_in_chars() {
                auto arr = std::array<char, sizeof...(Cs)>{Cs...};
                for (size_t idx = 0; idx < sizeof...(Cs); idx++) {
                    if (arr[idx] == ToFind) { return idx; }
                }
                return 0;
            }

            static constexpr auto get_str() {
                auto s = meta::string(meta::string<2>(C)...);
                return s.template substr<0, find_in_chars<')', C...>()>();
            }

            static constexpr auto value = get_str();

            static_assert(value.size() > 0, "Parameter name should be enclosed by parethesis");
        };

        template<char... C>
            struct chars_to_type : std::false_type {};

        template<char... C>
            struct chars_to_type<':', C...> : std::false_type {
                static_assert(false, "Un-parsed path parameter. Type isn't implemented");
            };

        template<char... C>
            struct chars_to_type<':', 's', 't', 'r', 'i', 'n', 'g', C...> : std::type_identity<
                named_param<std::string_view, param_name<C...>::value>
            > {};

        template<char... C>
            struct chars_to_type<':', 'i', 'n', 't', C...> : std::type_identity<
                named_param<int, param_name<C...>::value>
            > {};

        template<char... C>
            struct chars_to_type<':', 'l', 'o', 'n', 'g', C...> : std::type_identity<
                named_param<long, param_name<C...>::value>
            > {};

        template<char... C>
            struct chars_to_type<':', 'f', 'l', 'o', 'a', 't', C...> : std::type_identity<
                named_param<float, param_name<C...>::value>
            > {};

        template<char... C>
            struct chars_to_type<':', 'd', 'o', 'u', 'b', 'l', 'e', C...> : std::type_identity<
                named_param<double, param_name<C...>::value>
            > {};

        template<char... C>
            struct chars_to_type<':', 'b', 'o', 'o', 'l', C...> : std::type_identity<
                named_param<bool, param_name<C...>::value>
            > {};

        template<char... C>
            using chars_to_type_t = typename chars_to_type<C...>::type;

        template<typename Types, char... C>
        struct charlist_to_types: std::type_identity<Types> {};

        template<typename... Types, char Head, char... Tail>
        struct charlist_to_types<std::tuple<Types...>, Head, Tail...>
            : std::conditional_t<
                std::is_same_v<std::false_type, chars_to_type_t<Head, Tail...>>,
                charlist_to_types<std::tuple<Types...>, Tail...>,
                charlist_to_types<std::tuple<Types..., chars_to_type_t<Head, Tail...>>, Tail...>
            >
        {};

        template <meta::string Str, typename T=std::make_index_sequence<Str.size() - 1>>
        struct params_tuple;

        template <meta::string Str, size_t... Is>
        struct params_tuple<Str, std::index_sequence<Is...>> : charlist_to_types<std::tuple<>, Str.elems[Is]...> {};
    };
};
