#pragma once
#include <cstdint>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/use_awaitable.hpp>

#include "mech_suit/body.hpp"
#include "mech_suit/boost.hpp"
#include "mech_suit/listener.hpp"
#include "mech_suit/meta_string.hpp"
#include "mech_suit/route.hpp"
#include "mech_suit/router.hpp"

namespace mech_suit
{

class application
{
  public:
    static constexpr uint16_t default_port = 3000;
    static constexpr auto default_address = "0.0.0.0";
    struct config
    {
        std::string address;
        uint16_t port;
        size_t num_threads;
    };

  private:
    std::shared_ptr<detail::router> m_router = std::make_shared<detail::router>();
    std::vector<std::thread> m_threads {};
    config m_config;
    net::io_context m_ioc;

  public:
    explicit application(config conf = {.address = default_address,
                                        .port = default_port,
                                        .num_threads = std::thread::hardware_concurrency()})
        : m_config(std::move(conf))
        , m_ioc(static_cast<int>(m_config.num_threads))
    {
    }

    application(const application&) = delete;
    application(application&&) = delete;
    auto operator=(const application&) -> application& = delete;
    auto operator=(application&&) -> application& = delete;

    ~application() { stop(); }

    template<meta::string Path>
    void get(detail::callback_type_t<Path, GET> callback)
    {
        m_router->add_route<Path, GET>(callback);
    }

    template<meta::string Path, typename Body = no_body_t>
    void post(detail::callback_type_t<Path, POST, Body> callback)
    {
        m_router->add_route<Path, POST, Body>(callback);
    }

    void run()
    {
        auto addr = net::ip::make_address(m_config.address);

        // Create and launch a listening port
        std::make_shared<detail::listener>(m_ioc, tcp::endpoint {addr, m_config.port}, m_router)->run();

        // Run the I/O service on the requested number of threads
        m_threads.reserve(m_config.num_threads - 1);
        for (auto i = m_config.num_threads - 1; i > 0; --i)
        {
            m_threads.emplace_back([&] { m_ioc.run(); });
        }

        m_ioc.run();

        for (auto& thread : m_threads)
        {
            thread.join();
        }
    }

    template<typename... Arg>
    void stop_on_signals(Arg&&... arg)
    {
        //TODO: fix this, its copy pasta that doesn't work
        net::signal_set signals(m_ioc, std::forward<Arg>(arg)...);
        signals.async_wait(
            [&](beast::error_code const&, int)
            {
                // Stop the `io_context`. This will cause `run()`
                // to return immediately, eventually destroying the
                // `io_context` and all of the sockets in it.
                m_ioc.stop();
            });
    }

    void stop() { m_ioc.stop(); }
};
}  // namespace mech_suit
