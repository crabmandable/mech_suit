#include "mech_suit/mech_suit.hpp"

#include <boost/beast/http/message_generator.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Can parse a path with placeholders", "[library]")
{
    mech_suit::application app;
    app.get<"/path/:int(name)">([](const mech_suit::http_request&, int) -> mech_suit::http::message_generator {});

    app.get<"/path/:int(int_name)/:long(long_name)">(
        [](const mech_suit::http_request&, int, long) -> mech_suit::http::message_generator {});

    app.get<"/path/:int(int_name)/lol/:float(float_name)/:long(long_name)">(
        [](const mech_suit::http_request&, int, float, long) -> mech_suit::http::message_generator {});

    app.get<"/:string(some_string)">(
        [](const mech_suit::http_request&, const std::string_view&) -> mech_suit::http::message_generator {});

    app.get<"/">([](const mech_suit::http_request&) -> mech_suit::http::message_generator {});

    app.get<"/a/literal/path">([](const mech_suit::http_request&) -> mech_suit::http::message_generator {});

    // should not compile:
    /* app.get<"/:nope(name)">([](const mech_suit::http_request& ) -> mech_suit::http::message_generator { */
    /* }); */
    /**/
    /* app.get<"/:int">([](const auto&) -> mech_suit::http::message_generator { */
    /* }); */
}

struct foo
{
    int a;
    std::string s;
};
TEST_CASE("Can define a body struct", "[library]")
{
    mech_suit::application app;
    app.post<"/", mech_suit::body_json<foo>>(
        [](const mech_suit::http_request&, const foo&) -> mech_suit::http::message_generator {});

    app.post<"/users/:long(id)", mech_suit::body_json<foo>>(
        [](const mech_suit::http_request&, long, const foo&) -> mech_suit::http::message_generator {});

    app.post<"/", mech_suit::body_string>(
        [](const mech_suit::http_request&, std::string_view) -> mech_suit::http::message_generator {});
}
