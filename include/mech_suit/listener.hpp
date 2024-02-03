#pragma once
#include "mech_suit/boost.hpp"
#include "mech_suit/http_session.hpp"
#include "mech_suit/router.hpp"

namespace mech_suit::detail
{
class listener : public std::enable_shared_from_this<listener>
{
    net::io_context& m_ioc;
    tcp::acceptor m_acceptor;
    std::shared_ptr<const router> m_router;

  public:
    listener(net::io_context& ioc, tcp::endpoint endpoint, std::shared_ptr<const router> router)
        : m_ioc(ioc)
        , m_acceptor(net::make_strand(ioc))
        , m_router(std::move(router))
    {
        beast::error_code err;

        // Open the acceptor
        m_acceptor.open(endpoint.protocol(), err);
        if (err)
        {
            // TODO:
            return;
        }

        // Allow address reuse
        m_acceptor.set_option(net::socket_base::reuse_address(true), err);
        if (err)
        {
            // TODO:
            return;
        }

        // Bind to the server address
        m_acceptor.bind(endpoint, err);
        if (err)
        {
            // TODO:
            return;
        }

        // Start listening for connections
        m_acceptor.listen(net::socket_base::max_listen_connections, err);
        if (err)
        {
            // TODO:
            return;
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
            // TODO:
            return;  // To avoid infinite loop
        }

        // Create the session and run it
        std::make_shared<http_session>(std::move(socket), m_router)->run();

        // Accept another connection
        do_accept();
    }
};

}  // namespace mech_suit::detail
