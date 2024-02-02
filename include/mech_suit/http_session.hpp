#pragma once

#include "mech_suit/boost.hpp"

namespace mech_suit
{
struct http_session
{
    tcp_stream stream;
    beast::flat_buffer buffer;
    http_request request;

    explicit http_session(tcp_stream s)
        : stream(std::move(s))
    {
    }

    http_session(http_session&&) noexcept = default;
    auto operator=(http_session&&) -> http_session& = default;
    http_session(http_session&) noexcept = delete;
    auto operator=(const http_session&) -> http_session& = delete;
    ~http_session() = default;
};
}  // namespace mech_suit
