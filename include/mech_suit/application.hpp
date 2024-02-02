#pragma once
#include <chrono>
#include <iostream>
#include <string_view>

#include <boost/asio/ip/address.hpp>
#include <boost/asio/use_awaitable.hpp>

#include "mech_suit/body.hpp"
#include "mech_suit/boost.hpp"
#include "mech_suit/http_session.hpp"
#include "mech_suit/meta_string.hpp"
#include "mech_suit/route.hpp"

namespace mech_suit
{

class application
{
public:
private:

    std::unordered_map<std::string_view, std::unique_ptr<detail::base_route>> m_routes;
    std::vector<std::unique_ptr<detail::base_route>> m_dynamic_routes;

    // TODO: make configurable
    static constexpr std::chrono::duration<unsigned int> m_connection_timeout = std::chrono::seconds(30);

    template<meta::string Path, http_method Method, typename Body = no_body_t>
    void add_route(detail::callback_type_t<Path, Method, Body> callback)
    {
        if constexpr (0 == std::tuple_size<detail::params_tuple_t<Path>>())
        {
            m_routes.emplace(Path, std::make_unique<detail::route<Path, Method, Body>>(callback));
        }
        else
        {
            m_dynamic_routes.emplace_back(std::make_unique<detail::route<Path, Method, Body>>(callback));
        }
    }

    static auto not_found(http_session& session) -> http::message_generator
    {
        // TODO allow customization
        http::response<http::string_body> res {http::status::not_found, session.request.version()};
        res.set(http::field::server, "mech_suit");
        res.set(http::field::content_type, "text/html");
        res.keep_alive(false);
        res.body() = "The resource '" + std::string(session.request.target()) + "' was not found.";
        res.prepare_payload();

        return res;
    }

    auto handle_request(http_session session) -> net::awaitable<http::message_generator>
    {
        const std::string_view& path = session.request.target();

        if (m_routes.contains(path))
        {
            auto resp = co_await m_routes[path]->process(std::move(session));
            if (not resp) [[unlikely]]
            {
                // route not handled
                // this should ONLY happen due to programming error, since the
                // path is not dynamic
                throw std::runtime_error("Explicit route refused to handle a request");
            }

            co_return *resp;
        }
        else
        {
            for (const auto& route : m_dynamic_routes)
            {
                auto resp = co_await route->process(std::move(session));
                if (resp)
                {
                    co_return *resp;
                }
            }
        }

        co_return not_found(session);
    }

    // Handles an HTTP server connection
    auto do_session(tcp_stream stream) -> net::awaitable<void>
    {
        http_session session {std::move(stream)};

        // This lambda is used to send messages
        try
        {
            for (;;)
            {
                // Set the timeout.
                // TODO make configurable
                session.stream.expires_after(m_connection_timeout);

                // Read a request
                co_await http::async_read(session.stream, session.buffer, session.request, net::use_awaitable);

                // Handle the request
                http::message_generator msg = co_await handle_request(std::move(session));

                // Determine if we should close the connection
                bool keep_alive = msg.keep_alive();

                // Send the response
                co_await beast::async_write(stream, std::move(msg), net::use_awaitable);

                if (not keep_alive)
                {
                    // This means we should close the connection, usually
                    // because the response indicated the "Connection: close"
                    // semantic.
                    break;
                }
            }
        }
        catch (boost::system::system_error& se)
        {
            if (se.code() != http::error::end_of_stream)
            {
                throw;
            }
        }

        // Send a TCP shutdown
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
        // we ignore the error because the client might have
        // dropped the connection already.
    }

    // Accepts incoming connections and launches the sessions
    auto do_listen(tcp::endpoint endpoint) -> net::awaitable<void>
    {
        // Open the acceptor
        auto acceptor =
            net::use_awaitable_t<net::any_io_executor>::as_default_on(tcp::acceptor(co_await net::this_coro::executor));
        acceptor.open(endpoint.protocol());

        // Allow address reuse
        acceptor.set_option(net::socket_base::reuse_address(true));

        // Bind to the server address
        acceptor.bind(endpoint);

        // Start listening for connections
        acceptor.listen(net::socket_base::max_listen_connections);

        for (;;)
        {
            net::co_spawn(acceptor.get_executor(),
                          do_session(tcp_stream(co_await acceptor.async_accept())),
                          [](std::exception_ptr e_ptr)
                          {
                              if (e_ptr)
                              {
                                  try
                                  {
                                      std::rethrow_exception(e_ptr);
                                  }
                                  catch (std::exception& exc)
                                  {
                                      // TODO: do something with this error
                                      std::cerr << "Error in session: " << exc.what() << "\n";
                                  }
                              }
                          });
        }
    }

    // TODO make configurable
    size_t m_num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> m_threads;
    std::string m_address = "0.0.0.0";
    unsigned short m_port = 80;

  public:

    template<meta::string Path>
    void get(detail::callback_type_t<Path, GET> callback)
    {
        add_route<Path, GET>(callback);
    }

    template<meta::string Path, typename Body = no_body_t>
    void post(detail::callback_type_t<Path, POST, Body> callback)
    {
        add_route<Path, POST, Body>(callback);
    }

    void run()
    {
        net::io_context ioc {static_cast<int>(m_num_threads)};

        // Spawn a listening port
        boost::asio::co_spawn(ioc,
                              do_listen(tcp::endpoint {net::ip::make_address(m_address), m_port}),
                              [](std::exception_ptr e)
                              {
                                  if (e)
                                  {
                                      std::rethrow_exception(e);
                                  }
                              });

        // Run the I/O service on the requested number of threads
        m_threads.reserve(m_num_threads - 1);

        for (auto i = m_num_threads; i > 0; --i)
        {
            m_threads.emplace_back([&ioc] { ioc.run(); });
        }
    }
};
}  // namespace mech_suit
