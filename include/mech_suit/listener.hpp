#pragma once
#include <stdexcept>
#include <utility>

#include "mech_suit/boost.hpp"
#include "mech_suit/error_handlers.hpp"
#include "mech_suit/http_session.hpp"
#include "mech_suit/router.hpp"
#include "mech_suit/config.hpp"

namespace mech_suit::detail
{
class listener : public std::enable_shared_from_this<listener>
{
    std::shared_ptr<config> m_config;
    net::io_context& m_ioc;
    tcp::acceptor m_acceptor;
    std::shared_ptr<const router> m_router;
    socket_error_handler_t m_socket_error_handler;

  public:
    listener(std::shared_ptr<config> conf, net::io_context& ioc, std::shared_ptr<const router> router, socket_error_handler_t socket_error_handler)
    : m_config(std::move(conf))
        , m_ioc(ioc)
        , m_acceptor(net::make_strand(ioc))
        , m_router(std::move(router))
        , m_socket_error_handler(std::move(socket_error_handler))
    {
        auto addr = net::ip::make_address(m_config->address);
        tcp::endpoint endpoint {addr, m_config->port};

        beast::error_code err;

        // Open the acceptor
        m_acceptor.open(endpoint.protocol(), err);
        if (err)
        {
            throw std::runtime_error("Unable to open acceptor: " + err.message());
        }

        // Allow address reuse
        m_acceptor.set_option(net::socket_base::reuse_address(true), err);
        if (err)
        {
            throw std::runtime_error("Unable to set option on acceptor: " + err.message());
        }

        // Bind to the server address
        m_acceptor.bind(endpoint, err);
        if (err)
        {
            throw std::runtime_error("Unable to bind to address: " + err.message());
        }

        // Start listening for connections
        m_acceptor.listen(net::socket_base::max_listen_connections, err);
        if (err)
        {
            throw std::runtime_error("Unable to start listening: " + err.message());
        }
    }

    // Start accepting incoming connections
    void run() { do_accept(); }

  private:
    void do_accept()
    {
        // The new connection gets its own strand
        m_acceptor.async_accept(net::make_strand(m_ioc),
                                beast::bind_front_handler(&listener::on_accept, shared_from_this()));
    }

    void on_accept(beast::error_code err, tcp::socket socket)
    {
        if (err)
        {
            throw std::runtime_error("Error in listener" + err.message());
        }

        // Create the session and run it
        std::make_shared<http_session>(m_config, std::move(socket), m_router, m_socket_error_handler)->run();

        // Accept another connection
        do_accept();
    }
};

}  // namespace mech_suit::detail
