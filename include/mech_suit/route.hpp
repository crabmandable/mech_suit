#pragma once

#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <glaze/glaze.hpp>

#include "mech_suit/body.hpp"
#include "mech_suit/boost.hpp"
#include "mech_suit/common.hpp"
#include "mech_suit/http_request.hpp"
#include "mech_suit/path_params.hpp"
#include "mech_suit/error_handlers.hpp"

namespace mech_suit::detail
{
template<http::verb Method, typename T, typename Body>
struct route_callback;

template<http::verb Method, typename... Ts, typename Body>
struct route_callback<Method, std::tuple<Ts...>, Body>
{
    using type =
        std::function<http::message_generator(const http_request&, typename Ts::type..., const typename Body::type&)>;
};

template<http::verb Method, typename... Ts>
struct route_callback<Method, std::tuple<Ts...>, no_body_t>
{
    using type = std::function<http::message_generator(const http_request&, typename Ts::type...)>;
};

template<meta::string Path, http::verb Method, typename Body>
struct callback_type
{
    using type = route_callback<Method, params_tuple_t<Path>, Body>::type;
};

template<meta::string Path, http::verb Method, typename Body = no_body_t>
using callback_type_t = callback_type<Path, Method, Body>::type;

class base_route
{
  public:
    base_route() = default;
    base_route(const base_route&) = default;
    base_route(base_route&&) = default;
    auto operator=(const base_route&) -> base_route& = default;
    auto operator=(base_route&&) -> base_route& = default;

    virtual ~base_route() = default;

    virtual auto test_match(const std::vector<std::string_view>& path) const -> bool = 0;
    virtual auto handle_request(const http_request& request, exception_handler_t e_handler, unprocessable_handler_t u_handler) const -> http::message_generator = 0;
};

template<meta::string Path, http::verb Method, typename Body>
class route : public base_route
{
  public:
    using callback_t = callback_type_t<Path, Method, Body>;
    using params_t = http_params<Path>;

    static constexpr bool route_is_explicit = std::tuple_size_v<typename params_t::tuple_t> == 0;

    explicit route(callback_t callback)
        : m_callback(callback)
    {
    }

    auto test_match(const std::vector<std::string_view>& parts) const -> bool final
    {
        // This should not be called for explicit routes,
        // since we can do a simple string comparison
        if constexpr (route_is_explicit)
        {
            assert(not route_is_explicit);
            return false;
        }

        if (parts.size() != std::tuple_size_v<param_parts_tuple_t>)
        {
            return false;
        }

        for (size_t i = 0; i < parts.size(); i++)
        {
            if (not test_part_match(i, parts[i]))
            {
                return false;
            }
        }

        return true;
    }

  private:
    using body_t = typename Body::type;

    static constexpr auto path_part_count() -> size_t
    {
        size_t n = 1;
        for (size_t i = 1; i < Path.size(); i++)
        {
            if (Path.elems[i] == '/')
            {
                n++;
            }
        }
        return n;
    }

    // the regular case when no placeholder is at the part
    template<size_t Idx>
    struct part_at
    {
        static constexpr auto impl() -> std::pair<size_t, size_t>
        {
            if (Path.size() < 2)
            {
                return {0, 0};
            }

            constexpr auto path = static_cast<std::array<char, Path.size()>>(Path);

            auto begin = path.begin() + 1;
            auto end = begin;
            size_t idx = 0;
            while (path.end() != begin)
            {
                end = std::find(begin, path.end(), '/');
                if (idx++ == Idx)
                {
                    break;
                }
                begin = end + 1;
            }

            return {std::distance(path.begin(), begin), std::distance(path.begin(), end) - 1};
        }

        static constexpr auto pair = impl();

        using type = literal_path_part<Path.template substr<pair.first, pair.second>(), pair.first, Idx>;
    };

    // the case when part is a placeholder
    template<size_t Idx>
        requires(not std::is_same_v<std::false_type, typename params_t::template param_at_path_idx_t<Idx>>)
    struct part_at<Idx>
    {
        using type = typename params_t::template param_at_path_idx_t<Idx>;
    };

    template<typename T>
    struct make_parts;

    template<size_t... Is>
    struct make_parts<std::index_sequence<Is...>>
    {
        using type = std::tuple<typename part_at<Is>::type...>;
    };

    using path_parts_seq_t = decltype(std::make_index_sequence<path_part_count()>());
    using param_parts_tuple_t = typename make_parts<path_parts_seq_t>::type;

  protected:
    // The case for a route with no params but a body
    template<typename T = void>
        requires(not std::is_same_v<std::false_type, body_t> && params_t::size == 0)
    auto call_callback(const http_request& request, const body_t& body) const
    {
        params_t pararms {request.path};
        return m_callback(request, body);
    }

    // The case for a route with params and body
    template<size_t... Is>
        requires(not std::is_same_v<std::false_type, body_t>)
    auto call_callback(std::index_sequence<Is...> /*unused*/, const http_request& request, const body_t& body) const
    {
        params_t params {request.path};
        return m_callback(
            request, std::get<typename params_t::template param_type_at_index<Is>::type>(params.params[Is])..., body);
    }

    // The case for a route with no params and no body
    auto call_callback(const http_request& request) const
    {
        params_t pararms {request.path};
        return m_callback(request);
    }

    // The case for a route with params but no body
    template<size_t... Is>
    auto call_callback(std::index_sequence<Is...> /*unused*/, const http_request& request) const
    {
        params_t params {request.path};
        return m_callback(request,
                          std::get<typename params_t::template param_type_at_index<Is>::type>(params.params[Is])...);
    }

    auto handle_request(const http_request& request, exception_handler_t e_handler, unprocessable_handler_t u_handler) const -> http::message_generator final
    {
        using iseq_t = decltype(std::make_index_sequence<params_t::size>());

        if constexpr (std::is_same_v<std::false_type, body_t>)
        {
            if constexpr (params_t::size)
            {
                return call_callback(iseq_t(), request);
            }
            else
            {
                return call_callback(request);
            }
        }
        else
        {
            // TODO: investigate using custom body
            // TODO: investigate lazy streaming of body

            body_t body;
            if constexpr (body_is_glz_v<Body>)
            {
                auto err = glz::read<Body::opts>(body, request.beast_request.body());
                if (err)
                {
                    std::string descriptive_error = glz::format_error(err, request.beast_request.body());
                    return u_handler(request, descriptive_error);
                }
            }
            else if constexpr (std::is_same_v<body_string, Body>)
            {
                body = request.beast_request.body();
            }

            try {
                if constexpr (params_t::size)
                {
                    return call_callback(iseq_t(), request, body);
                }
                else
                {
                    return call_callback(request, body);
                }
            }
            catch (std::exception const& except)
            {
                return e_handler(request, except);
            }
        }
    }

  private:
    auto test_part_match(size_t idx, std::string_view part) const -> bool
    {
        if (idx >= std::tuple_size_v<param_parts_tuple_t>)
        {
            return false;
        }

        return m_param_parts[idx]->test_path_part(part);
    }

    template<typename... Ts>
    static constexpr auto init_parts(std::tuple<Ts...> /*unused*/)
        -> std::array<std::unique_ptr<base_path_part>, sizeof...(Ts)>
    {
        return {std::make_unique<Ts>()...};
    }

    callback_t m_callback;
    std::array<std::unique_ptr<base_path_part>, std::tuple_size_v<param_parts_tuple_t>> m_param_parts =
        init_parts(param_parts_tuple_t {});
};
}  // namespace mech_suit::detail
