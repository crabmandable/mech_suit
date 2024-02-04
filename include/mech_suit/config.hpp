#pragma once
#include <chrono>
#include <cstdint>
#include <thread>

namespace mech_suit
{
struct config
{
    static constexpr uint16_t default_port = 3000;
    static constexpr auto default_address = "0.0.0.0";
    static constexpr std::chrono::duration<unsigned int> default_timeout = std::chrono::seconds(30);

    std::string address = default_address;
    uint16_t port = default_port;
    size_t num_threads = std::thread::hardware_concurrency();
    std::chrono::duration<unsigned int> connection_timeout = default_timeout;
};
}  // namespace mech_suit
