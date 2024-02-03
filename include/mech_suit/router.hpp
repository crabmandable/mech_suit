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
        http::response<http::string_body> res {http::status::not_found, request.version()};
        res.set(http::field::content_type, "text/html");
        res.keep_alive(false);
        res.body() = "Not found";
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
            m_routes.emplace(Path, std::move(route));
        }
        else
        {
            m_dynamic_routes.emplace_back(std::move(route));
        }
    }

    auto handle_request(const http_request& request) const -> http::message_generator
    {
        const std::string_view& path = request.target();

        if (m_routes.contains(path))
        {
            return m_routes.at(path)->handle_request(request);
        }

        for (const auto& route : m_dynamic_routes)
        {
            if (route->test_match(path))
            {
                return route->handle_request(request);
            }
        }

        return not_found(request);
    }
};
}  // namespace mech_suit::detail
