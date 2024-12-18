#include <fmt/format.h>
#include <fmt/compile.h>

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <string>
#include <utility>

namespace irods
{
    // systemd requires that some status messages contain a monotonic timestamp in microseconds.
    static inline std::uint64_t get_monotonic_usec()
    {
        // we want CLOCK_MONOTONIC.
        // std::chrono::steady_clock is supposed to be CLOCK_MONOTONIC, but might not be, and might
        // not even be present.
        // boost::chrono::steady_clock may also be absent.
        // therefore, we do it the old fashioned way.

        std::timespec ts; // NOLINT(cppcoreguidelines-pro-type-member-init)
        // TODO: check for error, fall back to another implementation
        clock_gettime(CLOCK_MONOTONIC, &ts);
        const std::uint64_t mt_s = static_cast<std::uint64_t>(ts.tv_sec);
        const std::uint64_t mt_ns = static_cast<std::uint64_t>(ts.tv_nsec);
        const std::uint64_t usec = (mt_s * 1000000) + (mt_ns / 1000);
        return usec;
    }

    void notify_service_manager(const std::size_t _msg_length, const char* _msg, const char* _socket_path);

    static inline void notify_service_manager(const std::string& _msg)
    {
        const char* sm_socket_path = std::getenv("NOTIFY_SOCKET");
        if (sm_socket_path == nullptr) {
            // if NOTIFY_SOCKET is not set, we're done
            return;
        }

        notify_service_manager(_msg.size(), _msg.data(), sm_socket_path);
    }

    template <typename... Args>
    static inline void notify_service_manager(const fmt::format_string<Args...>& _format, Args&&... _args)
    {
        // early check to avoid unneeded fmt evaluation
        const char* sm_socket_path = std::getenv("NOTIFY_SOCKET");
        if (sm_socket_path == nullptr) {
            // if NOTIFY_SOCKET is not set, we're done
            return;
        }

        auto msg = fmt::format(_format, std::forward<Args>(_args)...);
        notify_service_manager(msg.size(), msg.data(), sm_socket_path);
    }

    template <typename CompiledFormat, typename... Args>
        requires fmt::detail::is_compiled_string<CompiledFormat>::value ||
                 fmt::detail::is_compiled_format<CompiledFormat>::value
    static inline void notify_service_manager(const CompiledFormat& _format, Args&&... _args)
    {
        // early check to avoid unneeded fmt evaluation
        const char* sm_socket_path = std::getenv("NOTIFY_SOCKET");
        if (sm_socket_path == nullptr) {
            // if NOTIFY_SOCKET is not set, we're done
            return;
        }

        auto msg = fmt::format(_format, std::forward<Args>(_args)...);
        notify_service_manager(msg.size(), msg.data(), sm_socket_path);
    }
} //namespace irods
