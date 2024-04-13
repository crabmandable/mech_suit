#pragma once
#include <cstdint>
#include <memory>

#include <boost/asio/signal_set.hpp>

#include "mech_suit/body.hpp"
#include "mech_suit/config.hpp"
#include "mech_suit/error_handlers.hpp"
#include "mech_suit/listener.hpp"
#include "mech_suit/meta_string.hpp"
#include "mech_suit/route.hpp"
#include "mech_suit/router.hpp"

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
    socket_error_handler_t m_socket_error_handler = [](auto){};

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

    template<http::verb Method, meta::string Path, typename Body = no_body_t>
    void add_route(detail::callback_type_t<Path, Method, Body> callback)
    {
        m_router->add_route<Path, Method, Body>(callback);
    }

    template<meta::string Path>
    void get(detail::callback_type_t<Path, http::verb::get> callback)
    {
        add_route<http::verb::get, Path>(callback);
    }

    template<meta::string Path, typename Body = no_body_t>
    void head(detail::callback_type_t<Path, http::verb::head, Body> callback)
    {
        add_route<http::verb::head, Path, Body>(callback);
    }

    template<meta::string Path, typename Body = no_body_t>
    void post(detail::callback_type_t<Path, http::verb::post, Body> callback)
    {
        add_route<http::verb::post, Path, Body>(callback);
    }

    template<meta::string Path, typename Body = no_body_t>
    void put(detail::callback_type_t<Path, http::verb::put, Body> callback)
    {
        add_route<http::verb::put, Path, Body>(callback);
    }

    template<meta::string Path, typename Body = no_body_t>
    void delete_(detail::callback_type_t<Path, http::verb::delete_, Body> callback)
    {
        add_route<http::verb::delete_, Path, Body>(callback);
    }

    template<meta::string Path, typename Body = no_body_t>
    void options(detail::callback_type_t<Path, http::verb::options, Body> callback)
    {
        add_route<http::verb::options, Path, Body>(callback);
    }

    void add_not_found_handler(not_found_handler_t handler)
    {
        m_router->add_not_found_handler(std::move(handler));
    }

    void add_exception_handler(exception_handler_t handler)
    {
        m_router->add_exception_handler(std::move(handler));
    }

    void add_glz_parse_error_handler(glz_parse_error_handler_t handler)
    {
        m_router->add_glz_parse_error_handler(std::move(handler));
    }

    void add_socket_error_handler(socket_error_handler_t handler)
    {
        m_socket_error_handler = std::move(handler);
    }

    void run()
    {
        // Create and launch a listening port
        std::make_shared<detail::listener>(m_config, m_ioc, m_router, m_socket_error_handler)->run();

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
