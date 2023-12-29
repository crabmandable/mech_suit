#pragma once
namespace mech_suit {
    enum class http_method {
        GET,
        POST,
        PUT,
        DELETE,
        REPLACE,
    };

    static auto constexpr GET = http_method::GET;
    static auto constexpr POST = http_method::POST;
    static auto constexpr PUT = http_method::PUT;
    static auto constexpr DELETE = http_method::DELETE;
    static auto constexpr REPLACE = http_method::REPLACE;
};
