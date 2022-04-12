#ifndef IRODS_DELAY_SERVER_HPP
#define IRODS_DELAY_SERVER_HPP

namespace irods
{
    // clang-format off
    constexpr const int default_delay_server_sleep_time_in_seconds   = 30;
    constexpr const int default_number_of_concurrent_delay_executors = 4;
    // clang-format on

    static_assert(default_delay_server_sleep_time_in_seconds > 0);
    static_assert(default_number_of_concurrent_delay_executors > 0);
} // namespace irods

#endif // IRODS_DELAY_SERVER_HPP 
