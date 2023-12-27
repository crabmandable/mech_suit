#include "mech_suit/mech_suit.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace mech_suit;
TEST_CASE("Can parse a path with placeholders", "[library]")
{
    route<"/path/:int">([](const http_request&, http_response&, int) {
    });

    route<"/path/:int/:long">([](const http_request&, http_response&, int, long) {
    });

    route<"/path/:int/lol/:float/:long">([](const http_request&, http_response&, int, float, long) {
    });

    route<"/:int">([](const http_request&, http_response&, int) {
    });

    route<"/:string">([](const http_request&, http_response&, const std::string_view&) {
    });

    /* route<"/:nope">([](const http_request&, http_response&) { */
    /* }); */

    route<"/">([](const http_request&, http_response&) {
    });
}
