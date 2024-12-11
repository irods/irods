#ifndef IRODS_NOTIFY_SERVICE_MANAGER_HPP
#define IRODS_NOTIFY_SERVICE_MANAGER_HPP

/// \file

#include <fmt/format.h>

#include <cstddef>
#include <ctime>
#include <string>
#include <utility>

namespace irods
{
    /// \brief Internal library function for communicating status to system service manager.
    ///
    /// \warning This function is not to be called directly. Use notify_service_manager() instead.
    ///
    /// \param[in] _msg         Null-terminated string containing the status message to send to the service manager.
    /// \param[in] _msg_size    Size in bytes of \p _msg.
    /// \param[in] _socket_path Null-terminated string containing the path to the service manager communication socket.
    ///
    /// \since 5.0.0
    void do_notify_service_manager(const char* _msg, const std::size_t _msg_size, const char* _socket_path);

    /// \brief Sends a status message in the form of a string to the service manager.
    ///
    /// This function and its overloads allow the server to inform a service manager of its status via an
    /// environment-block-like string. See systemd's documentation for
    /// <a href="https://www.freedesktop.org/software/systemd/man/latest/sd_notify.html">\c sd_notify</a> for more
    /// details.
    ///
    /// \param[in] _msg String containing the status message to send to the service manager.
    ///
    /// \since 5.0.0
    static inline void notify_service_manager(const std::string& _msg)
    {
        const char* sm_socket_path = std::getenv("NOTIFY_SOCKET");
        if (nullptr == sm_socket_path) {
            // if NOTIFY_SOCKET is not set, we're done
            return;
        }

        do_notify_service_manager(_msg.data(), _msg.size(), sm_socket_path);
    }

    /// \brief Sends a status message in the form of a formatted string to the service manager.
    ///
    /// This function and its overloads allow the server to inform a service manager of its status via an
    /// environment-block-like string. See systemd's documentation for
    /// <a href="https://www.freedesktop.org/software/systemd/man/latest/sd_notify.html">\c sd_notify</a> for more
    /// details.
    ///
    /// \tparam Args The list of types matching the arguments passed.
    ///
    /// \param[in] _format Format string to use to generate the status message.
    /// \param[in] _args   Format arguments to inject into the format string.
    ///
    /// \since 5.0.0
    template <typename... Args>
    static inline void notify_service_manager(const fmt::format_string<Args...>& _format, Args&&... _args)
    {
        const char* sm_socket_path = std::getenv("NOTIFY_SOCKET");
        if (nullptr == sm_socket_path) {
            // if NOTIFY_SOCKET is not set, we're done
            return;
        }

        auto msg = fmt::format(_format, std::forward<Args>(_args)...);
        do_notify_service_manager(msg.data(), msg.size(), sm_socket_path);
    }
} //namespace irods

#endif // IRODS_NOTIFY_SERVICE_MANAGER_HPP
