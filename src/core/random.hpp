#pragma once

#include <cstdint>
#include <random>

namespace hb
{

// Global game RNG - single-threaded, main thread only
inline std::mt19937& rng()
{
    static std::mt19937 instance{std::random_device{}()};
    return instance;
}

// Random integer in [min, max] inclusive
inline int32_t random_int(int32_t min, int32_t max)
{
    if (min >= max)
        return min;
    std::uniform_int_distribution<int32_t> dist(min, max);
    return dist(rng());
}

// Random float in [min, max]
inline float random_float(float min, float max)
{
    if (min >= max)
        return min;
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng());
}

// Random integer in [0, bound) exclusive upper bound (replaces rand() % bound)
inline int32_t random_mod(int32_t bound)
{
    if (bound <= 1)
        return 0;
    std::uniform_int_distribution<int32_t> dist(0, bound - 1);
    return dist(rng());
}

// Random bool with 50/50 chance
inline bool random_bool()
{
    return std::uniform_int_distribution<int32_t>(0, 1)(rng()) != 0;
}

} // namespace hb
