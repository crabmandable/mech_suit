#pragma once
#include <array>
#include <charconv>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "mech_suit/common.hpp"
#include "mech_suit/meta_string.hpp"

namespace mech_suit::detail
{

struct base_path_part
{
    base_path_part() = default;
    constexpr base_path_part(const base_path_part&) = default;
    base_path_part(base_path_part&&) = default;
    auto operator=(const base_path_part&) -> base_path_part& = default;
    auto operator=(base_path_part&&) -> base_path_part& = default;
    virtual ~base_path_part() = default;
    virtual auto test_path_part(std::string_view part) const -> bool = 0;
};

template<meta::string Part, size_t Pos, size_t PartN>
struct literal_path_part : public base_path_part
{
    literal_path_part() = default;
    static constexpr size_t pos = Pos;
    static constexpr size_t part_n = PartN;

    auto test_path_part(std::string_view part) const -> bool final
    {
        return part == static_cast<std::string_view>(Part);
    }
};

template<meta::string Path, size_t Pos, typename T, meta::string Name>
struct path_param : public base_path_part
{
  private:
    static constexpr auto count_slashes_until_part() -> size_t
    {
        size_t n = 0;
        // skip the first `/`
        for (size_t i = 1; i < Pos; ++i)
        {
            if (Path.elems[i] == '/')
            {
                ++n;
            }
        }

        return n;
    }

  public:
    path_param() = default;

    auto test_path_part(std::string_view part) const -> bool final
    {
        if constexpr (std::is_same_v<T, std::string_view>)
        {
            return true;
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            constexpr auto valid = std::array<const char*, 6> {
                "yes",
                "no",
                "true",
                "false",
                "1",
                "0",
            };
            return valid.end() != std::find(valid.begin(), valid.end(), part);
        }
        else if constexpr (std::is_arithmetic_v<T>)
        {
            // TODO support negative numbers and floats
            return !part.empty() && std::all_of(part.begin(), part.end(), ::isdigit);
        }
        else
        {
            static_assert(sizeof(T), "Path param testing not implemented for type");
        }
    }

    static constexpr size_t pos = Pos;
    static constexpr size_t part_n = count_slashes_until_part();
    using type = T;
    static constexpr auto name = Name;
};

template<char... C>
struct param_name
{
    static_assert(false, "Parameter is unamed! This is not legal");
};

template<char... C>
struct param_name<'(', C...>
{
    template<char ToFind, char... Cs>
    static constexpr auto find_in_chars() -> size_t
    {
        auto arr = std::array<char, sizeof...(Cs)> {Cs...};
        for (size_t idx = 0; idx < sizeof...(Cs); idx++)
        {
            if (arr[idx] == ToFind)
            {
                return idx;
            }
        }
        return 0;
    }

    static constexpr auto get_str()
    {
        auto s = meta::string(meta::string<2>(C)...);
        return s.template substr<0, find_in_chars<')', C...>()>();
    }

    static constexpr auto value = get_str();

    static_assert(value.size() > 0, "Parameter name should be enclosed by parethesis");
};

template<meta::string Path, size_t Pos, char... C>
struct chars_to_type : std::false_type
{
};

template<meta::string Path, size_t Pos, char... C>
struct chars_to_type<Path, Pos, ':', C...> : std::false_type
{
    static_assert(false, "Un-parsed path parameter. Type isn't implemented");
};

template<meta::string Path, size_t Pos, char... C>
struct chars_to_type<Path, Pos, ':', 's', 't', 'r', 'i', 'n', 'g', C...>
    : std::type_identity<path_param<Path, Pos, std::string_view, param_name<C...>::value>>
{
};

template<meta::string Path, size_t Pos, char... C>
struct chars_to_type<Path, Pos, ':', 'i', 'n', 't', C...>
    : std::type_identity<path_param<Path, Pos, int, param_name<C...>::value>>
{
};

template<meta::string Path, size_t Pos, char... C>
struct chars_to_type<Path, Pos, ':', 'l', 'o', 'n', 'g', C...>
    : std::type_identity<path_param<Path, Pos, long, param_name<C...>::value>>
{
};

template<meta::string Path, size_t Pos, char... C>
struct chars_to_type<Path, Pos, ':', 'f', 'l', 'o', 'a', 't', C...>
    : std::type_identity<path_param<Path, Pos, float, param_name<C...>::value>>
{
};

template<meta::string Path, size_t Pos, char... C>
struct chars_to_type<Path, Pos, ':', 'd', 'o', 'u', 'b', 'l', 'e', C...>
    : std::type_identity<path_param<Path, Pos, double, param_name<C...>::value>>
{
};

template<meta::string Path, size_t Pos, char... C>
struct chars_to_type<Path, Pos, ':', 'b', 'o', 'o', 'l', C...>
    : std::type_identity<path_param<Path, Pos, bool, param_name<C...>::value>>
{
};

template<meta::string Path, size_t Pos, char... C>
using chars_to_type_t = typename chars_to_type<Path, Pos, C...>::type;

template<meta::string Path, size_t Pos, typename T, char... C>
struct charlist_to_types : std::type_identity<T>
{
};

template<meta::string Path, size_t Pos, typename... Types, char Head, char... Tail>
struct charlist_to_types<Path, Pos, std::tuple<Types...>, Head, Tail...>
    : std::conditional_t<
          std::is_same_v<std::false_type, chars_to_type_t<Path, Pos, Head, Tail...>>,
          charlist_to_types<Path, Pos + 1, std::tuple<Types...>, Tail...>,
          charlist_to_types<Path, Pos + 1, std::tuple<Types..., chars_to_type_t<Path, Pos, Head, Tail...>>, Tail...>>
{
};

template<meta::string Str, typename T = std::make_index_sequence<Str.size() - 1>>
struct params_tuple;

template<meta::string Str, size_t... Is>
struct params_tuple<Str, std::index_sequence<Is...>> : charlist_to_types<Str, 0, std::tuple<>, Str.elems[Is]...>
{
};

template<meta::string Str>
using params_tuple_t = typename params_tuple<Str>::type;

template<typename T>
class http_params_impl;

template<typename... Ts>
class http_params_impl<std::tuple<Ts...>>
{
    using variant_t = std::variant<std::nullopt_t, typename Ts::type...>;

    template<size_t... Is>
    auto parse(std::index_sequence<Is...> /*unused*/, size_t param_index, std::string_view path_part) const -> variant_t
    {
        return (
            ...,
            (param_index == Is ? (parse_from_str<typename param_type_by_index<Is>::type>(path_part)) : std::nullopt));
    }

    // Convert string_view to type T
    template<typename T>
    auto parse_from_str(std::string_view str) const -> T
    {
        if constexpr (std::is_same_v<std::string_view, T>)
        {
            return str;
        }
        else if (std::is_same_v<bool, T>)
        {
            return str == "1" || str == "yes" || str == "true";
        }
        else
        {
            T val;
            std::from_chars(str.data(), str.data() + str.size(), val);
            return val;
        }
    }

  public:
    using tuple_t = std::tuple<Ts...>;

    std::vector<std::variant<typename Ts::type...>> params {};

    static constexpr std::array<size_t, sizeof...(Ts)> part_numbers {Ts::part_n...};
    static constexpr size_t size = std::tuple_size_v<tuple_t>;

    template<size_t Idx>
    struct param_type_by_index
        : std::type_identity<std::remove_cvref_t<decltype(std::get<static_cast<size_t>(Idx)>(std::declval<tuple_t>()))>>
    {
    };

    template<size_t Idx>
    struct param_at_path_idx : std::type_identity<std::false_type>
    {
    };

    template<size_t Idx>
        requires(index_of_first<(Idx == Ts::part_n)...>::value >= 0)
    struct param_at_path_idx<Idx>
        : std::type_identity<
              typename param_type_by_index<static_cast<size_t>(index_of_first<(Idx == Ts::part_n)...>::value)>::type>
    {
    };

    template<size_t Idx>
    using param_at_path_idx_t = typename param_at_path_idx<Idx>::type;

    explicit http_params_impl(std::string_view path)
    {
        if (path.size() <= 1)
        {
            return;
        }

        // skip the first "/"
        path = path.substr(1);

        for (size_t i = 0, n = 0; n <= part_numbers.back(); n++)
        {
            if (n == part_numbers[i])
            {
                auto part = path.substr(0, path.find('/'));
                params.push_back(parse(std::index_sequence_for<Ts...>(), i++, part));
            }

            auto pos = path.find('/');
            if (pos == std::string_view::npos || pos + 1 >= path.size())
            {
                break;
            }

            path = path.substr(pos + 1);
        }

        // we should only be parsing paths that match, but just incase
        if (params.size() != sizeof...(Ts)) [[unlikely]]
        {
            throw std::runtime_error("Failed to parse parameters");
        }
    }
};

template<meta::string Path>
struct http_params : public http_params_impl<params_tuple_t<Path>>
{
    template<typename... Arg>
    explicit http_params(Arg&&... arg)
        : http_params_impl<params_tuple_t<Path>>(std::forward<Arg>(arg)...)
    {
    }
};

}  // namespace mech_suit::detail
