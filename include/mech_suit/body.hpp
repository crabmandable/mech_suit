#pragma once

#include <string>
#include <utility>
#include <glaze/glaze.hpp>

namespace mech_suit
{
struct body_string : std::type_identity<std::string>
{
};

template<typename T, glz::opts Opts = glz::opts{.format = glz::json}>
struct body_glz : std::type_identity<T>
{
    static constexpr glz::opts opts = Opts;
};

template<typename T, glz::opts Opts = glz::opts{.format = glz::json}>
using body_json = body_glz<T, Opts>;

template<typename T, glz::opts Opts = glz::opts{.format = glz::binary}>
using body_binary = body_glz<T, Opts>;

template<typename T>
struct body_is_glz : std::false_type
{
};

template<typename T, auto Opts>
struct body_is_glz<body_glz<T, Opts>> : std::true_type
{
};

template<typename T>
static constexpr bool body_is_glz_v = body_is_glz<T>();

using no_body_t = std::type_identity<std::false_type>;
}  // namespace mech_suit
