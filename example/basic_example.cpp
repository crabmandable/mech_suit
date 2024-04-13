#include <format>
#include <iostream>
#include <glaze/core/context.hpp>

#include "mech_suit/mech_suit.hpp"

namespace ms = mech_suit;

struct foo
{
    int bar = 0;
};

auto main() -> int
{
    ms::config conf {
        .address = "0.0.0.0",
        .port = 3000,
        .num_threads = 1,
        .connection_timeout = std::chrono::seconds(30),
    };

    ms::application app(conf);

    app.get<"/">(
        [](const ms::http_request& request)
        {
            ms::http::response<ms::http::string_body> response {ms::http::status::ok,
                                                                request.beast_request.version()};
            response.set(ms::http::field::content_type, "text/html");
            response.keep_alive(request.beast_request.keep_alive());
            response.body() = "Hello world!\n";
            response.prepare_payload();
            return response;
        });

    app.get<"/:string(name)">(
        [](const ms::http_request& request, std::string_view name)
        {
            ms::http::response<ms::http::string_body> response {ms::http::status::ok,
                                                                request.beast_request.version()};
            response.set(ms::http::field::content_type, "text/html");
            response.keep_alive(request.beast_request.keep_alive());
            response.body() = std::format("Hello {}!\n", name);
            response.prepare_payload();
            return response;
        });

    app.get<"/number/:int(n)/good">(
        [](const ms::http_request& request, int n)
        {
            ms::http::response<ms::http::string_body> response {ms::http::status::ok,
                                                                request.beast_request.version()};
            response.set(ms::http::field::content_type, "text/html");
            response.keep_alive(request.beast_request.keep_alive());
            response.body() = std::format("{} is a pretty good number!\n", n);
            response.prepare_payload();
            return response;
        });

    app.get<"/number/:int(n)/bad">(
        [](const ms::http_request& request, int n)
        {
            ms::http::response<ms::http::string_body> response {ms::http::status::ok,
                                                                request.beast_request.version()};
            response.set(ms::http::field::content_type, "text/html");
            response.keep_alive(request.beast_request.keep_alive());
            response.body() = std::format("{} is a bad number!\n", n);
            response.prepare_payload();
            return response;
        });

    app.post<"/", ms::body_json<foo>>(
        [](ms::http_request const& request, foo const& body)
        {
            ms::http::response<ms::http::string_body> response {ms::http::status::ok,
                                                                request.beast_request.version()};
            response.set(ms::http::field::content_type, "text/html");
            response.keep_alive(request.beast_request.keep_alive());
            response.body() = std::format("foo.bar = {}\n", body.bar);
            response.prepare_payload();
            return response;
        });

    app.stop_on_signals(SIGTERM, SIGINT);

    std::cout << std::format("Running basic_example on {}:{}\n", conf.address, conf.port);
    app.run();

    std::cout << "Goodbye.\n";

    return 0;
}
