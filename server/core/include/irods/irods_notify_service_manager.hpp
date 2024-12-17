#include "irods/irods_error.hpp"
#include "irods/rodsErrorTable.h"

#include <fmt/format.h>
#include <fmt/compile.h>

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
} //namespace irods

#ifdef IRODS_USE_SD_NOTIFY

#include <systemd/sd-daemon.h>

namespace irods
{

    namespace
    {
        static inline irods::error notify_service_manager_impl(const char* _msg)
        {
            int ret = sd_notify(0, _msg);
            if (ret < 0) {
                return ERROR(SYS_LIBRARY_ERROR + ret, "sd_notify error");
            }
            return SUCCESS();
        }
    } //namespace

    static inline irods::error notify_service_manager(const std::string& _msg)
    {
        return notify_service_manager_impl(_msg.data());
    }

    template <typename... Args>
    static inline irods::error notify_service_manager(const fmt::format_string<Args...>& _format, Args&&... _args)
    {
        // early check to avoid unneeded fmt evaluation
        const char* sm_socket_path = std::getenv("NOTIFY_SOCKET");
        if (!sm_socket_path) {
            // if NOTIFY_SOCKET is not set, we're done
            return SUCCESS();
        }

        auto msg = fmt::format(_format, std::forward<Args>(_args)...);
        return notify_service_manager_impl(msg.data());
    }

    template <typename CompiledFormat, typename... Args>
        requires fmt::detail::is_compiled_string<CompiledFormat>::value ||
                 fmt::detail::is_compiled_format<CompiledFormat>::value
    static inline irods::error notify_service_manager(const CompiledFormat& _format, Args&&... _args)
    {
        // early check to avoid unneeded fmt evaluation
        const char* sm_socket_path = std::getenv("NOTIFY_SOCKET");
        if (!sm_socket_path) {
            // if NOTIFY_SOCKET is not set, we're done
            return SUCCESS();
        }

        auto msg = fmt::format(_format, std::forward<Args>(_args)...);
        return notify_service_manager_impl(msg.data());
    }
} //namespace irods

#else

#include <cstdlib>
#include <cstring>

#include <sys/socket.h>
#include <sys/un.h>

#include <boost/asio.hpp>

namespace irods
{
    namespace
    {
        // We have to reimplement boost::asio::local::datagram_protocol to add SOCK_CLOEXEC
        class sm_socket_protocol
        {
          public:
            int type() const noexcept
            {
                return (SOCK_DGRAM | SOCK_CLOEXEC);
            }

            int protocol() const noexcept
            {
                return 0;
            }

            int family() const noexcept
            {
                return AF_UNIX;
            }

            typedef boost::asio::local::basic_endpoint<sm_socket_protocol> endpoint;
            typedef boost::asio::basic_datagram_socket<sm_socket_protocol> socket;
        };

        static inline irods::error notify_service_manager_impl(const char* _msg,
                                                               const std::size_t _msg_length,
                                                               const char* _sm_socket_path)
        {
            if (!_msg) {
                return ERROR(SYS_INTERNAL_NULL_INPUT_ERR, "_msg is not a valid pointer");
            }
            if (_msg_length == 0) {
                return ERROR(SYS_INTERNAL_NULL_INPUT_ERR, "_msg is empty");
            }

            if (_sm_socket_path[0] != '/' && _sm_socket_path[0] != '@') {
                // Only AF_UNIX is supported, with path or abstract sockets
                return ERROR(SYS_NOT_SUPPORTED, "Socket in NOTIFY_SOCKET not supported");
            }

            boost::asio::io_context io_context;
            const sm_socket_protocol::endpoint sm_socket_path(_sm_socket_path);
            sm_socket_protocol::socket sm_socket(io_context);

            sm_socket.open();
            std::size_t written = sm_socket.send_to(boost::asio::buffer(_msg, _msg_length), sm_socket_path);
            if (written < _msg_length) {
                return ERROR(SYS_SOCK_WRITE_ERR, "could not write entire message to socket");
            }

            return SUCCESS();
        }
    } //namespace

    static inline irods::error notify_service_manager(const std::string& _msg)
    {
        const char* sm_socket_path = std::getenv("NOTIFY_SOCKET");
        if (!sm_socket_path) {
            // if NOTIFY_SOCKET is not set, we're done
            return SUCCESS();
        }

        return notify_service_manager_impl(_msg.data(), _msg.size(), sm_socket_path);
    }

    template <typename... Args>
    static inline irods::error notify_service_manager(const fmt::format_string<Args...>& _format, Args&&... _args)
    {
        const char* sm_socket_path = std::getenv("NOTIFY_SOCKET");
        if (!sm_socket_path) {
            // if NOTIFY_SOCKET is not set, we're done
            return SUCCESS();
        }

        auto msg = fmt::format(_format, std::forward<Args>(_args)...);

        return notify_service_manager_impl(msg.data(), msg.size(), sm_socket_path);
    }

    template <typename CompiledFormat, typename... Args>
        requires fmt::detail::is_compiled_string<CompiledFormat>::value ||
                 fmt::detail::is_compiled_format<CompiledFormat>::value
    static inline irods::error notify_service_manager(const CompiledFormat& _format, Args&&... _args)
    {
        const char* sm_socket_path = std::getenv("NOTIFY_SOCKET");
        if (!sm_socket_path) {
            // if NOTIFY_SOCKET is not set, we're done
            return SUCCESS();
        }

        auto msg = fmt::format(_format, std::forward<Args>(_args)...);

        return notify_service_manager_impl(msg.data(), msg.size(), sm_socket_path);
    }

} //namespace irods

#endif
