#pragma once

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/verb.hpp>

namespace mech_suit {

    namespace beast = boost::beast;
    namespace http = boost::beast::http;
    namespace net = boost::asio;
    using tcp = boost::asio::ip::tcp;

    using tcp_stream = typename boost::beast::tcp_stream::rebind_executor<
        net::use_awaitable_t<>::executor_with_default<net::any_io_executor>>::other;

}  // namespace mech_suit
