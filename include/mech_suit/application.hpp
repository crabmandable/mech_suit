#pragma once
#include <cstdint>
#include <memory>
#include <boost/asio/signal_set.hpp>

#include "mech_suit/body.hpp"
#include "mech_suit/listener.hpp"
#include "mech_suit/meta_string.hpp"
#include "mech_suit/route.hpp"
#include "mech_suit/router.hpp"
#include "mech_suit/config.hpp"

namespace mech_suit
{

class application
{
  public:

  private:
    std::shared_ptr<detail::router> m_router = std::make_shared<detail::router>();
    std::vector<std::thread> m_threads {};
    std::shared_ptr<config> m_config;
    net::io_context m_ioc;
    std::unique_ptr<net::signal_set> m_signals;

  public:
    explicit application(config conf = {})
        : m_config(std::make_shared<config>(conf))
        , m_ioc(static_cast<int>(m_config->num_threads))
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
        // Create and launch a listening port
        std::make_shared<detail::listener>(m_config, m_ioc, m_router)->run();

        // Run the I/O service on the requested number of threads
        m_threads.reserve(m_config->num_threads - 1);
        for (auto i = m_config->num_threads - 1; i > 0; --i)
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
        m_signals = std::make_unique<net::signal_set>(m_ioc, std::forward<Arg>(arg)...);
        m_signals->async_wait(
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
