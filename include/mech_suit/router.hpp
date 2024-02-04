#pragma once
#include "mech_suit/boost.hpp"
#include "mech_suit/route.hpp"

namespace mech_suit::detail
{
class router
{
    std::map<std::string_view, std::unique_ptr<detail::base_route>> m_routes;
    std::vector<std::unique_ptr<detail::base_route>> m_dynamic_routes;

    static auto not_found(const http_request& request) -> http::message_generator
    {
        // TODO: allow customization
        http::response<http::string_body> res {http::status::not_found, request.beast_request.version()};
        res.set(http::field::content_type, "text/html");
        res.keep_alive(false);
        res.body() = "Not found\n";
        res.prepare_payload();

        return res;
    }

  public:
    template<meta::string Path, http_method Method, typename Body = no_body_t>
    void add_route(detail::callback_type_t<Path, Method, Body> callback)
    {
        using route_t = detail::route<Path, Method, Body>;
        auto route = std::make_unique<route_t>(callback);

        if constexpr (route_t::route_is_explicit)
        {
            m_routes.emplace(static_cast<const char*>(Path), std::move(route));
        }
        else
        {
            m_dynamic_routes.emplace_back(std::move(route));
        }
    }

    auto handle_request(http_request request) const -> http::message_generator
    {
        if (m_routes.contains(request.path))
        {
            return m_routes.at(request.path)->handle_request(request);
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

        for (const auto& route : m_dynamic_routes)
        {
            if (route->test_match(parts))
            {
                return route->handle_request(request);
            }
        }

        return not_found(request);
    }
};
}  // namespace mech_suit::detail
