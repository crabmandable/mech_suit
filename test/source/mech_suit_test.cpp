#include "mech_suit/mech_suit.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace mech_suit;

TEST_CASE("Can parse a path with placeholders", "[library]")
{
    get<"/path/:int(name)">([](const http_request&, http_response&, const int&) {
    });

    get<"/path/:int(int_name)/:long(long_name)">([](const http_request& , http_response&, const int&, const long&) {
    });

    get<"/path/:int(int_name)/lol/:float(float_name)/:long(long_name)">([](const http_request&, http_response&, const int&, const float&, const long&) {
    });

    get<"/:string(some_string)">([](const http_request&, http_response&, const std::string_view&) {
    });

    /* get<"/:nope">([](const http_request&, http_response&) { */
    /* }); */

    /* get<"/:int">([](const auto&, http_response&) { */
    /* }); */

    get<"/">([](const http_request&, http_response&) {
    });
}

struct Foo {
    int a;
    std::string s;
};
TEST_CASE("Can define a body struct", "[library]")
{
    post<"/", body_json<Foo>>([](const http_request&, http_response&, const Foo&) {
    });

    post<"/users/:long(id)", body_json<Foo>>([](const http_request&, http_response&, long, const Foo&) {
    });
}
