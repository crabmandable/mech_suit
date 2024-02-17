#pragma once

#include <unordered_map>

#include "mech_suit/boost.hpp"
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
        // TODO: allow customization
        http::response<http::string_body> res {http::status::not_found,
                                               request.beast_request.version()};
        res.set(http::field::content_type, "text/html");
        res.keep_alive(false);
        res.body() = "Not found\n";
        res.prepare_payload();

        return res;
    }

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

    auto handle_request(http_request request) const -> http::message_generator
    {
        const auto method = request.beast_request.method();

        if (m_routes.contains(method) && m_routes.at(method).contains(request.path))
        {
            return m_routes.at(method).at(request.path)->handle_request(request);
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
                return it->second->handle_request(request);
            }
        }

        return not_found(request);
    }
};
}  // namespace mech_suit::detail
