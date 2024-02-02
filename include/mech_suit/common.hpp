#pragma once
#include <algorithm>
#include <array>
#include <functional>

namespace mech_suit
{

template<bool... T>
static constexpr bool any_of = (... || T);

template<bool... T>
static constexpr bool all_of = (... && T);

template<bool... T>
struct index_of_first
{
  private:
    static constexpr auto impl() -> int
    {
        constexpr std::array<bool, sizeof...(T)> a {T...};
        const auto it = std::find(a.begin(), a.end(), true);

        if (it == a.end())
        {
            return -1;
        }
        return static_cast<int>(std::distance(a.begin(), it));
    }

  public:
    static constexpr int value = impl();
};

template<class... Ts>
struct overloaded_visitor : Ts...
{
    using Ts::operator()...;
};

template<class Func, class Tuple, size_t N = 0>
void runtime_get(Func func, Tuple& tup, size_t idx)
{
    if (N == idx)
    {
        std::invoke(func, std::get<N>(tup));
        return;
    }

    if constexpr (N + 1 < std::tuple_size_v<Tuple>)
    {
        return runtime_get<Func, Tuple, N + 1>(func, tup, idx);
    }
}
}  // namespace mech_suit
