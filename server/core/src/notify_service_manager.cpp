#include "irods/notify_service_manager.hpp"

#include "irods/irods_logger.hpp"

#include <cstddef>

#ifdef IRODS_USE_LIBSYSTEMD

#  include <systemd/sd-daemon.h>

namespace irods
{
    void do_notify_service_manager(const char* _msg,
                                   [[maybe_unused]] const std::size_t _msg_size,
                                   [[maybe_unused]] const char* _socket_path)
    {
        irods::experimental::log::server::debug("Notifying service manager: [{}]", _msg);
        const auto ret = sd_notify(0, _msg);
        if (ret < 0) {
            irods::experimental::log::server::warn("sd_notify failed with error [{}].", ret);
        }
    }
} //namespace irods

#else // IRODS_USE_LIBSYSTEMD

#  include <cstdlib>
#  include <cstring>

#  include <sys/socket.h>
#  include <sys/un.h>

#  include <boost/asio.hpp>

namespace irods
{
    namespace
    {
        // We have to reimplement boost::asio::local::datagram_protocol to add SOCK_CLOEXEC
        class sm_socket_protocol
        {
          public:
            // NOLINTNEXTLINE(modernize-use-nodiscard,readability-convert-member-functions-to-static)
            int type() const noexcept
            {
                return (SOCK_DGRAM | SOCK_CLOEXEC);
            }

            // NOLINTNEXTLINE(modernize-use-nodiscard,readability-convert-member-functions-to-static)
            int protocol() const noexcept
            {
                return 0;
            }

            // NOLINTNEXTLINE(modernize-use-nodiscard,readability-convert-member-functions-to-static)
            int family() const noexcept
            {
                return AF_UNIX;
            }

            using endpoint = boost::asio::local::basic_endpoint<sm_socket_protocol>;
            using socket = boost::asio::basic_datagram_socket<sm_socket_protocol>;
        }; //class sm_socket_protocol
    } //namespace

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    void do_notify_service_manager(const char* _msg, const std::size_t _msg_size, const char* _socket_path)
    {
        if (nullptr == _msg) {
            irods::experimental::log::server::error("Null message pointer passed to notify_service_manager.");
            return;
        }
        if (_msg_size == 0) {
            irods::experimental::log::server::error("Empty message passed to notify_service_manager.");
            return;
        }

        irods::experimental::log::server::debug("Notifying service manager at [{}] with [{}].", _msg, _socket_path);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (_socket_path[0] != '/' && _socket_path[0] != '@') {
            // Only AF_UNIX is supported, with path or abstract sockets
            irods::experimental::log::server::warn("Socket in NOTIFY_SOCKET not supported: [{}]", _socket_path);
            return;
        }

        boost::asio::io_context io_context;
        const sm_socket_protocol::endpoint sm_socket_path(_socket_path);
        sm_socket_protocol::socket sm_socket(io_context);

        sm_socket.open();
        const std::size_t written = sm_socket.send_to(boost::asio::buffer(_msg, _msg_size), sm_socket_path);
        if (written < _msg_size) {
            irods::experimental::log::server::warn(
                "Partial message written to NOTIFY_SOCKET (wrote {} of {}).", written, _msg_size);
        }
    } // do_notify_service_manager
} //namespace irods

#endif // IRODS_USE_LIBSYSTEMD
