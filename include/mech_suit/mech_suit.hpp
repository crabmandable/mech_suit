#pragma once

#include "meta_string.hpp"
#include <functional>
#include <utility>

namespace mech_suit {
    enum class http_method {
        GET,
        POST,
        PUT,
        DELETE,
        REPLACE,
    };
    auto constexpr GET = http_method::GET;
    auto constexpr POST = http_method::POST;
    auto constexpr PUT = http_method::PUT;
    auto constexpr DELETE = http_method::DELETE;
    auto constexpr REPLACE = http_method::REPLACE;

    struct http_request {};
    struct http_response {};

    namespace detail {
        template<char... C>
            struct chars_to_type : std::false_type {};

        template<char... C>
            struct chars_to_type<':', C...> : std::false_type {
                static_assert(false, "Un-parsed path parameter. Type isn't implemented");
            };

        template<char... C>
            struct chars_to_type<':', 's', 't', 'r', 'i', 'n', 'g', C...> : std::type_identity<const std::string_view&> {};

        template<char... C>
            struct chars_to_type<':', 'i', 'n', 't', C...> : std::type_identity<int> {};

        template<char... C>
            struct chars_to_type<':', 'l', 'o', 'n', 'g', C...> : std::type_identity<long> {};

        template<char... C>
            struct chars_to_type<':', 'f', 'l', 'o', 'a', 't', C...> : std::type_identity<float> {};

        template<char... C>
            struct chars_to_type<':', 'd', 'o', 'u', 'b', 'l', 'e', C...> : std::type_identity<double> {};

        template<char... C>
            struct chars_to_type<':', 'b', 'o', 'o', 'l', C...> : std::type_identity<bool> {};

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

        template<typename T=std::tuple<>>
        struct tuple_to_callback {
            using type = std::function<void(const http_request&, http_response&)>;
        };

        template<typename... Ts>
        struct tuple_to_callback<std::tuple<Ts...>> {
            using type = std::function<void(const http_request&, http_response&, Ts...)>;
        };

        template<meta::string Path>
        struct callback_type_from_path {
            using params_tuple_t = typename params_tuple<Path>::type;
            using type = tuple_to_callback<params_tuple_t>::type;
        };

        template<meta::string Path> using callback_type_from_path_t = callback_type_from_path<Path>::type;
    };


    template <meta::string Path>
    void route(detail::callback_type_from_path_t<Path>) {
    }

}
