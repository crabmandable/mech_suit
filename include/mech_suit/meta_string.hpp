#pragma once

#include <cstddef>
#include <type_traits>
#include <algorithm>
#include <functional>
#include <tuple>
#include <stdexcept>

namespace meta
{

struct string_base
{
    using size_type = std::size_t;
    using char_type = char;
    static constexpr size_type npos = size_type(-1);
};


template <string_base::size_type N> requires (N >= 1)
class string;


template <typename>
struct is_string : std::false_type {};

template <string_base::size_type N>
struct is_string<meta::string<N>> : std::true_type {};


template <typename>
struct is_string_constructible : std::false_type{};

template <string_base::size_type N>
struct is_string_constructible<string_base::char_type [N]> : std::true_type {};

template <string_base::size_type N>
struct is_string_constructible<meta::string<N>> : std::true_type {};

template <template <typename...> typename container, typename... T>
    requires ((is_string_constructible<T>::value && ...))
struct is_string_constructible<container<T...>> : std::true_type {};

template <typename T>
inline constexpr bool is_string_constructible_v = is_string_constructible<T>::value;


template <typename T>
concept string_constructible = is_string_constructible_v<T>;


template <string_base::size_type N> requires (N >= 1)
class string : public string_base
{
public:

    char elems[N];

    string() = delete;

    constexpr string(const char_type c)
    {
        elems[0] = c;
        elems[1] = '\0';
    }

    constexpr string(const char_type (&s)[N])
    {
        std::copy_n(s, N, elems);
    }

    template <size_type Ni, size_type pos = 0, size_type count = npos>
    constexpr string(const string<Ni> (&s), std::integral_constant<size_type, pos>, std::integral_constant<size_type, count>)
    {
        *std::copy_n(
            &s.elems[std::min(pos, Ni - 1)],
            std::min(count, Ni - 1 - std::min(pos, Ni - 1)),
            elems
        ) = '\0';
    }

    template <string_constructible... T>
    constexpr string(const T&... input)
    {
        copy_from(meta::string(input)...);
    }

    template <template <typename...> typename container, string_constructible... T>
    constexpr string(const container<T...>& input)
    {
        std::apply([this](const auto&... s) constexpr {
            copy_from(meta::string(s)...);
        }, input);
    }

    template<typename OtherString>
    consteval bool operator==(const OtherString& other) const {
        if (other.size() != size()) return false;
        for (size_t i = 0; i < N; i++) {
            if (elems[i] != other.elems[i]) return false;
        }
        return true;
    }

    constexpr auto operator + (const auto& rhs) const
    {
        return meta::string(*this, meta::string(rhs));
    }

    static constexpr size_type size_static() noexcept
    { return N; }

    constexpr size_type size() const noexcept
    { return N; }

    constexpr bool empty() const noexcept
    { return N == 1; }

    constexpr const char_type* data() const noexcept
    { return elems; }

    constexpr operator const char_type* () const noexcept
    { return elems; }

    constexpr operator std::string_view () const
    { return std::string_view{ elems, N }; }

    constexpr const char_type& at(size_type pos) const
    {
        if(pos >= N - 1)
            throw std::out_of_range("out of bounds");
        return elems[pos];
    }

    constexpr const char_type& operator [] (size_type pos) const
    { return at(pos); }

    constexpr const char_type& front() const noexcept
    { return elems[0]; }

    constexpr const char_type& back() const noexcept
    { return elems[N - 1]; }

    template <size_type pos = 0, size_type count = npos>
    constexpr auto substr() const
    {
        return meta::string(*this, std::integral_constant<size_type, pos>{}, std::integral_constant<size_type, count>{});
    }

    constexpr size_type copy(char_type* dest, size_type count = npos, size_type pos = 0) const
    {
        if(pos >= N)
            throw std::out_of_range("out of bounds");
        return std::copy_n(&elems[pos], std::min(count, N - 1 - std::min(pos, N - 1)), dest) - dest;
    }

private:

    template <size_type... Ni>
    constexpr void copy_from(const string<Ni> (&... input))
    {
        auto pos = elems;
        ((pos = std::copy_n(input.elems, Ni - 1, pos)), ...);
        *pos = 0;
    }
};

namespace detail {

template <string_constructible... T>
constexpr inline string_base::size_type string_length = ((decltype(string(std::declval<T>()))::size_static() - 1) + ... + 1);

} // namespace detail

template <string_base::size_type N>
string(const string_base::char_type (&)[N])
    -> string<N>;

template <string_base::size_type Ni, string_base::size_type pos, string_base::size_type count>
string(const string<Ni> (&), std::integral_constant<string_base::size_type, pos>, std::integral_constant<string_base::size_type, count>)
    -> string<std::min(count, Ni - 1 - std::min(pos, Ni - 1)) + 1>;

template <string_constructible... T>
string(const T&...)
    -> string<detail::string_length<T...>>;

template <template <typename...> typename container, string_constructible... T>
string(const container<T...>&)
    -> string<detail::string_length<T...>>;


inline namespace meta_string_literals {

template <string ms>
inline constexpr auto operator"" _ms() noexcept
{
    return ms;
}

} // inline namespace meta_string_literals

} // namespace meta
