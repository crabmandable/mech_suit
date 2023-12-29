#pragma once

#include <functional>
#include <utility>
#include <glaze/glaze.hpp>

#include "mech_suit/meta_string.hpp"
#include "mech_suit/body.hpp"
#include "mech_suit/path_params.hpp"

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

    template <typename... Ts>
    class http_params {
    private:
        template <typename... Us>
        struct actual_params {
            using type = std::tuple<typename Us::type...>;
        };

        actual_params<Ts...>::type m_params;

        template<size_t idx, meta::string ParamName, typename... Us>
        struct find_index_by_name {
            constexpr static ssize_t value = -1;
        };

        template<size_t idx, meta::string ParamName, typename Head, typename... Tail>
        struct find_index_by_name<idx, ParamName, Head, Tail...> : std::conditional_t<
                Head::name == ParamName,
                std::integral_constant<size_t, idx>,
                find_index_by_name<idx + 1, ParamName, Tail...>
            >
        { };

    public:
        template<meta::string ParamName>
        auto get() const {
            static constexpr ssize_t idx = find_index_by_name<0, ParamName, Ts...>::value;
            static_assert(idx >= 0, "Parameter does not exist");
            return std::get<static_cast<size_t>(idx)>(m_params);
        }
    };

    struct http_request {
        std::string body_raw;
    };

    struct http_response {};

    namespace detail {
        template<http_method Method, typename T, typename Body>
        struct route_callback;

        template<http_method Method, typename... Ts>
        struct route_callback<Method, std::tuple<Ts...>, void> {
            using type = std::function<void(const http_request&, http_response&, const typename Ts::type&...)>;
        };

        template<http_method Method, typename... Ts, typename Body>
        struct route_callback<Method, std::tuple<Ts...>, Body> {
            using type = std::function<void(const http_request&, http_response&, const typename Ts::type&..., const typename Body::type&)>;
        };

        template<meta::string Path, http_method Method, typename Body>
        struct callback_type {
            using params_tuple_t = typename params_tuple<Path>::type;
            using type = route_callback<Method, params_tuple_t, Body>::type;
        };

        template<meta::string Path, http_method Method, typename Body=void> using callback_type_t = callback_type<Path, Method, Body>::type;

    };

    template <meta::string Path>
    void get(detail::callback_type_t<Path, GET>)
    {
    }

    template <meta::string Path, typename Body=void>
    void post(detail::callback_type_t<Path, POST, Body>)
    {
        http_request request;

        if constexpr (body_is_json_v<Body>) {
            using type_t = typename Body::type;
            type_t body;
            auto err = glz::read_json<type_t>(body, request.body_raw);
            if (err) {
                std::string descriptive_error = glz::format_error(err, request.body_raw);
                // do something with error
            } else {
                // do something with body
            }
        }
    }
}
