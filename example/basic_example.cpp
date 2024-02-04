#include "mech_suit/mech_suit.hpp"

auto main() -> int
{
    namespace ms = mech_suit;
    ms::application::config conf {
        .address = "0.0.0.0",
        .port = 3000,
        .num_threads = 1,
    };

    ms::application app(conf);

    app.get<"/">([](const ms::http_request& request) {

        ms::http::response<ms::http::string_body> response{ms::http::status::ok, request.beast_request.version()};
        response.set(ms::http::field::content_type, "text/html");
        response.keep_alive(request.beast_request.keep_alive());
        response.body() = "Hello world!\n";
        response.prepare_payload();
        return response;
    });

    app.get<"/:string(name)">([](const ms::http_request& request, std::string_view name) {

        ms::http::response<ms::http::string_body> response{ms::http::status::ok, request.beast_request.version()};
        response.set(ms::http::field::content_type, "text/html");
        response.keep_alive(request.beast_request.keep_alive());
        response.body() = std::format("Hello {}!\n", name);
        response.prepare_payload();
        return response;
    });

    app.run();

    return 0;
}
