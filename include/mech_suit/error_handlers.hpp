#pragma once
#include <functional>
#include <boost/beast/core/error.hpp>
#include <glaze/core/context.hpp>

#include "mech_suit/http_request.hpp"

namespace mech_suit
{
using not_found_handler_t = std::function<http::message_generator(http_request const&)>;
using exception_handler_t =
    std::function<http::message_generator(http_request const&, std::exception const& except)>;
using glz_parse_error_handler_t =
    std::function<http::message_generator(http_request const&, glz::parse_error)>;
using socket_error_handler_t = std::function<void(beast::error_code)>;
}  // namespace mech_suit
