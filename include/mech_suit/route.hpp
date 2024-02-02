#pragma once

#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <glaze/glaze.hpp>

#include "mech_suit/body.hpp"
#include "mech_suit/boost.hpp"
#include "mech_suit/common.hpp"
#include "mech_suit/http_method.hpp"
#include "mech_suit/http_session.hpp"
#include "mech_suit/path_params.hpp"
#include "path_params.hpp"

namespace mech_suit::detail
{
template<http_method Method, typename T, typename Body>
struct route_callback;

template<http_method Method, typename... Ts, typename Body>
struct route_callback<Method, std::tuple<Ts...>, Body>
{
    using type = std::function<http::message_generator(
        const http_request&, const typename Ts::type&..., const typename Body::type&)>;
};

template<http_method Method, typename... Ts>
struct route_callback<Method, std::tuple<Ts...>, no_body_t>
{
    using type = std::function<http::message_generator(const http_request&, const typename Ts::type&...)>;
};

template<meta::string Path, http_method Method, typename Body>
struct callback_type
{
    using type = route_callback<Method, params_tuple_t<Path>, Body>::type;
};

template<meta::string Path, http_method Method, typename Body = no_body_t>
using callback_type_t = callback_type<Path, Method, Body>::type;

class base_route
{
    std::string_view m_path;
    bool m_route_is_explicit;

  protected:
    using awaitable_response = net::awaitable<std::optional<http::message_generator>>;

    virtual auto handle_request(http_session session) const -> awaitable_response = 0;
    virtual auto path_part_match(size_t idx, std::string_view part) const -> bool = 0;

  public:
    base_route(const base_route&) = default;
    base_route(base_route&&) = default;
    auto operator=(const base_route&) -> base_route& = default;
    auto operator=(base_route&&) -> base_route& = default;
    explicit base_route(std::string_view path, bool route_is_explicit)
        : m_path(path)
        , m_route_is_explicit(route_is_explicit)
    {
    }
    virtual ~base_route() = default;

    auto process(http_session session) -> net::awaitable<std::optional<http::message_generator>>
    {
        // if path matches, handle_request
        if (m_route_is_explicit)
        {
            if (m_path == session.request.target())
            {
                co_return handle_request(std::move(session));
            }
        }
        else
        {
            std::string_view path = session.request.target();
            size_t i = 0;
            do
            {
                auto part = path.substr(0, path.find('/'));

                if (not path_part_match(i++, part))
                {
                    co_return std::nullopt;
                }
                path = path.substr(part.size());
            } while (!path.empty());

            co_return handle_request(std::move(session));
        }
        co_return std::nullopt;
    }
};

template<meta::string Path, http_method Method, typename Body>
class route : public base_route
{
  public:
    static constexpr std::string_view path = std::string_view(Path);
    using callback_t = callback_type_t<Path, Method, Body>;
    using params_t = http_params<Path>;

    explicit route(callback_t callback)
        : base_route(path, std::tuple_size_v<typename params_t::tuple_t> == 0)
        , m_callback(callback)
    {
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
        static constexpr auto impl()
        {
            size_t idx = 0;
            size_t start = 1;
            while (start < Path.size())
            {
                if (Path.elems[start] == '/')
                {
                    idx++;
                }
                if (idx == Idx)
                {
                    break;
                }
                start++;
            }

            size_t end = start + 1;
            while (end < Path.size())
            {
                if (Path.elems[end] == '/')
                {
                    idx++;
                }
                if (idx == Idx)
                {
                    break;
                }
                end++;
            }

            return std::pair<size_t, size_t> {start, end};
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
    auto call_callback(const http_request& request, const body_t& body) const -> awaitable_response
    {
        params_t pararms {request.target()};
        return m_callback(request, body);
    }

    // The case for a route with params and body
    template<size_t... Is>
        requires(not std::is_same_v<std::false_type, body_t>)
    auto call_callback(std::index_sequence<Is...> /*unused*/, const http_request& request, const body_t& body) const
        -> awaitable_response
    {
        params_t params {request.target()};
        return m_callback(
            request,
            std::get<typename decltype(params)::template param_type_by_index<Is>::type>(params.params[Is])...,
            body);
    }

    // The case for a route with no params and no body
    auto call_callback(const http_request& request) const -> awaitable_response
    {
        params_t pararms {request.target()};
        return m_callback(request);
    }

    // The case for a route with params but no body
    template<size_t... Is>
    auto call_callback(std::index_sequence<Is...> /*unused*/, const http_request& request) const -> awaitable_response
    {
        params_t params {request.target()};
        return m_callback(
            request, std::get<typename decltype(params)::template param_type_by_index<Is>::type>(params.params[Is])...);
    }

    auto handle_request(http_session session) const -> awaitable_response final
    {
        using iseq_t = decltype(std::make_index_sequence<params_t::size>());

        // stream body if needed
        if constexpr (not std::is_same_v<std::false_type, body_t>)
        {
            // TODO: investigate using custom body

            body_t body;
            if constexpr (body_is_json_v<Body>)
            {
                auto err = glz::read_json<body_t>(body, session.request.body());
                if (err)
                {
                    std::string descriptive_error = glz::format_error(err, session.request.body());
                    // TODO: do something with error
                    throw std::runtime_error(descriptive_error);
                }
            }
            else if constexpr (std::is_same_v<body_string, Body>)
            {
                body = std::move(session.request.body());
            }

            if constexpr (params_t::size)
            {
                co_return call_callback(iseq_t(), session.request, body);
            }
            else
            {
                co_return call_callback(session.request, body);
            }
        }
        else
        {
            if constexpr (params_t::size)
            {
                co_return call_callback(iseq_t(), session.request);
            }
            else
            {
                co_return call_callback(session.request);
            }
        }
    }

    auto path_part_match(size_t idx, std::string_view part) const -> bool final
    {
        if (idx >= std::tuple_size_v<param_parts_tuple_t>)
        {
            return false;
        }

        return m_param_parts[idx]->test_path_part(part);
    }

  private:
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
