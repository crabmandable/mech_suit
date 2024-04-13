#pragma once

#include <exception>
#include <unordered_map>

#include <boost/beast/http/message_generator.hpp>
#include <glaze/core/context.hpp>

#include "mech_suit/boost.hpp"
#include "mech_suit/error_handlers.hpp"
#include "mech_suit/route.hpp"

namespace mech_suit::detail
{
class router
{
    std::unordered_map<http::verb,
                       std::unordered_map<std::string_view, std::unique_ptr<detail::base_route>>>
        m_routes;

    std::unordered_multimap<http::verb, std::unique_ptr<detail::base_route>> m_dynamic_routes;

    static auto not_found(const http_request& request) -> http::message_generator
    {
        http::response<http::string_body> res {http::status::not_found,
                                               request.beast_request.version()};

        res.set(http::field::content_type, "text/html");
        res.keep_alive(false);
        res.body() = "Not found\n";
        res.prepare_payload();

        return res;
    }

    static auto unprocessable(const http_request& request, glz::parse_error error)
        -> http::message_generator
    {

        std::string message = glz::format_error(error, request.beast_request.body());

        http::response<http::string_body> res {http::status::unprocessable_entity,
                                               request.beast_request.version()};

        res.set(http::field::content_type, "text/html");
        res.keep_alive(false);
        res.body() = std::move(message);
        res.prepare_payload();

        return res;
    }

    static auto exception(const http_request& request, std::exception const& except)
        -> http::message_generator
    {
        http::response<http::string_body> res {http::status::internal_server_error,
                                               request.beast_request.version()};

        res.set(http::field::content_type, "text/html");
        res.keep_alive(false);
        res.body() = except.what();
        res.prepare_payload();

        return res;
    }

    glz_parse_error_handler_t m_glz_parse_error_handler = router::unprocessable;
    exception_handler_t m_exception_handler = router::exception;
    not_found_handler_t m_not_found_handler = router::not_found;

  public:
    template<meta::string Path, http::verb Method, typename Body = no_body_t>
    void add_route(detail::callback_type_t<Path, Method, Body> callback)
    {
        using route_t = detail::route<Path, Method, Body>;
        auto route = std::make_unique<route_t>(callback);

        if constexpr (route_t::route_is_explicit)
        {
            m_routes[Method].emplace(static_cast<const char*>(Path), std::move(route));
        }
        else
        {
            m_dynamic_routes.emplace(Method, std::move(route));
        }
    }

    void add_not_found_handler(not_found_handler_t handler)
    {
        m_not_found_handler = std::move(handler);
    }

    void add_exception_handler(exception_handler_t handler)
    {
        m_exception_handler = std::move(handler);
    }

    void add_glz_parse_error_handler(glz_parse_error_handler_t handler)
    {
        m_glz_parse_error_handler = std::move(handler);
    }

    auto handle_request(http_request request) const -> http::message_generator
    {
        const auto method = request.beast_request.method();

        if (m_routes.contains(method) && m_routes.at(method).contains(request.path))
        {
            const auto& route = m_routes.at(method).at(request.path);

            return route->handle_request(request, m_exception_handler, m_glz_parse_error_handler);
        }

        std::vector<std::string_view> parts;
        // skip the leading slash
        std::string_view path = request.path.substr(1);
        while (!path.empty())
        {
            auto part = path.substr(0, path.find('/'));
            parts.push_back(part);

            // skip the next slash (if there is one)
            if (path.size() > part.size() + 1)
            {
                path = path.substr(part.size() + 1);
            }
            else
            {
                path = path.substr(part.size());
            }
        }

        auto range = m_dynamic_routes.equal_range(method);
        for (auto it = range.first; it != range.second; it++)
        {
            if (it->second->test_match(parts))
            {
                return it->second->handle_request(
                    request, m_exception_handler, m_glz_parse_error_handler);
            }
        }

        return m_not_found_handler(request);
    }
};
}  // namespace mech_suit::detail
