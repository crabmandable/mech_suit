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
        constexpr std::array<bool, sizeof...(T)> arr {T...};
        const auto iter = std::find(arr.begin(), arr.end(), true);

        if (iter == arr.end())
        {
            return -1;
        }
        return static_cast<int>(std::distance(arr.begin(), iter));
    }

  public:
    static constexpr int value = impl();
};

template<class... Ts>
struct overloaded_visitor : Ts...
{
    using Ts::operator()...;
};
}  // namespace mech_suit
