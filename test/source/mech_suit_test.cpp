#include "mech_suit/mech_suit.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace mech_suit;
TEST_CASE("Can parse a path with placeholders", "[library]")
{
    get<"/path/:int(name)">([](const auto& request, http_response&, const int&) {
            static_assert(std::is_same_v<int, decltype(request.params.template get<"name">())>, "Oops");
    });

    get<"/path/:int(int_name)/:long(long_name)">([](const auto& request, http_response&, const int&, const long&) {
            static_assert(std::is_same_v<int, decltype(request.params.template get<"int_name">())>, "Oops");
            static_assert(std::is_same_v<long, decltype(request.params.template get<"long_name">())>, "Oops");
    });

    get<"/path/:int(int_name)/lol/:float(float_name)/:long(long_name)">([](const auto& request, http_response&, const int&, const float&, const long&) {
            static_assert(std::is_same_v<int, decltype(request.params.template get<"int_name">())>, "Oops");
            static_assert(std::is_same_v<float, decltype(request.params.template get<"float_name">())>, "Oops");
            static_assert(std::is_same_v<long, decltype(request.params.template get<"long_name">())>, "Oops");
    });

    get<"/:string(some_string)">([](const auto& request, http_response&, const std::string_view&) {
            static_assert(std::is_same_v<std::string_view, decltype(request.params.template get<"some_string">())>, "Oops");
    });

    /* get<"/:nope">([](const http_request&, http_response&) { */
    /* }); */

    /* get<"/:int">([](const auto&, http_response&) { */
    /* }); */

    get<"/">([](const auto& request, http_response&) {
            (void)request;
    });
}
