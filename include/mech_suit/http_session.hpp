#pragma once

#include <utility>
#include <boost/beast/http/string_body.hpp>

#include "mech_suit/boost.hpp"
#include "mech_suit/router.hpp"
#include "mech_suit/http_request.hpp"

namespace mech_suit::detail
{
class http_session : public std::enable_shared_from_this<http_session>
{
    beast::flat_buffer m_buffer;
    beast::tcp_stream m_stream;
    http_request::beast_request_t m_request;
    std::shared_ptr<const router> m_router;

    // TODO: make configurable
    static constexpr std::chrono::duration<unsigned int> m_connection_timeout = std::chrono::seconds(30);

  public:
    explicit http_session(tcp::socket socket, std::shared_ptr<const router> router)
        : m_stream(std::move(socket))
        , m_router(std::move(router))
    {
    }

    http_session(http_session&&) noexcept = default;
    auto operator=(http_session&&) noexcept -> http_session& = default;
    http_session(http_session&) = delete;
    auto operator=(const http_session&) -> http_session& = delete;
    ~http_session() = default;

    // Start the asynchronous operation
    void run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        net::dispatch(m_stream.get_executor(), beast::bind_front_handler(&http_session::do_read, shared_from_this()));
    }

    void do_read()
    {
        // Make sure request is reset
        m_request = {};

        // Set the timeout.
        m_stream.expires_after(m_connection_timeout);

        // Read a request
        http::async_read(
            m_stream, m_buffer, m_request, beast::bind_front_handler(&http_session::on_read, shared_from_this()));
    }

    void on_read(beast::error_code err, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This means they closed the connection
        if (err == http::error::end_of_stream)
        {
            return do_close();
        }

        if (err)
        {
            // TODO:
            return;
        }

        // Send the response
        send_response(m_router->handle_request(http_request{std::move(m_request)}));
    }

    void send_response(http::message_generator&& msg)
    {
        bool keep_alive = msg.keep_alive();

        // Write the response
        beast::async_write(
            m_stream, std::move(msg), beast::bind_front_handler(&http_session::on_write, shared_from_this(), keep_alive));
    }

    void on_write(bool keep_alive, beast::error_code err, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (err)
        {
            //TODO:
            return;
        }

        if (!keep_alive)
        {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            return do_close();
        }

        // Read another request
        do_read();
    }

    void do_close()
    {
        // Send a TCP shutdown
        beast::error_code err;
        m_stream.socket().shutdown(tcp::socket::shutdown_send, err);

        // At this point the connection is closed gracefully
    }
};
}  // namespace mech_suit
