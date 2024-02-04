#pragma once
#include "mech_suit/boost.hpp"
namespace mech_suit
{
struct http_request
{
    using beast_request_t = http::request<http::string_body>;

    explicit http_request(beast_request_t&& req)
        : beast_request(std::move(req))
    {
        path = beast_request.target();

        // parse query
        const auto query_pos = path.find('?');
        if (std::string_view::npos != query_pos)
        {
            query = path.substr(query_pos);
            path = path.substr(0, query_pos);
        }

        // strip trailing slash
        path = path.size() > 1 && path.back() == '/' ? path.substr(0, path.size() - 1) : path;
    }

    beast_request_t beast_request;
    std::string_view path;
    std::string_view query;
};
}  // namespace mech_suit
