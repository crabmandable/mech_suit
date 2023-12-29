#include <utility>

namespace mech_suit {
#pragma once
    template <typename T>
    struct body_json : std::type_identity<T> {};

    template <typename T>
    struct body_is_json : std::false_type {};

    template <typename T>
    struct body_is_json<body_json<T>> : std::true_type {};

    template <typename T>
    static constexpr bool body_is_json_v = body_is_json<T>();
}
