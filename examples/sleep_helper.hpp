#ifndef SLEEP_HELPER_HPP
#define SLEEP_HELPER_HPP

#include <chrono>
#include <thread>

namespace geoqik::examples
{

void
sleep_for_seconds(double seconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(seconds * 1000)));
}

} // namespace geoqik::examples

#endif // SLEEP_HELPER_HPP
