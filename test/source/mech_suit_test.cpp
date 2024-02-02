#include "mech_suit/mech_suit.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Can parse a path with placeholders", "[library]")
{
    mech_suit::application app;
    app.get<"/path/:int(name)">([](const mech_suit::http_request&, const int&) -> mech_suit::http::message_generator {
    });

    /* app.get<"/path/:int(int_name)/:long(long_name)">([](const mech_suit::http_request&, const int&, const long&) { */
    /* }); */
    /**/
    /* app.get<"/path/:int(int_name)/lol/:float(float_name)/:long(long_name)">([](const mech_suit::http_request&, const int&, const float&, const long&) { */
    /* }); */
    /**/
    /* app.get<"/:string(some_string)">([](const mech_suit::http_request&, const std::string_view&) { */
    /* }); */

    /* get<"/:nope">([](const http_request&, http_response&) { */
    /* }); */

    /* get<"/:int">([](const auto&, http_response&) { */
    /* }); */

    /* app.get<"/">([](const mech_suit::http_request&) { */
    /* }); */
}

struct Foo {
    int a;
    std::string s;
};
TEST_CASE("Can define a body struct", "[library]")
{
    mech_suit::application app;
    /* app.post<"/", mech_suit::body_json<Foo>>([](const mech_suit::http_request&, const Foo&) { */
    /* }); */
    /**/
    /* app.post<"/users/:long(id)", mech_suit::body_json<Foo>>([](const mech_suit::http_request&, long, const Foo&) { */
    /* }); */
}
