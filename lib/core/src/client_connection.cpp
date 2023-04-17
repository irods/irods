#include "irods/client_connection.hpp"

#include "irods/rcConnect.h"
#include "irods/rodsErrorTable.h"
#include "irods/irods_exception.hpp"

namespace irods::experimental
{
    client_connection::client_connection()
        : conn_{nullptr, rcDisconnect}
    {
        rodsEnv env{};
        _getRodsEnv(env);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        connect_and_login(env.rodsHost, env.rodsPort, {env.rodsUserName, env.rodsZone});
    } // default constructor

    client_connection::client_connection(struct defer_authentication) // NOLINT(readability-named-parameter)
        : conn_{nullptr, rcDisconnect}
    {
        rodsEnv env{};
        _getRodsEnv(env);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        only_connect(env.rodsHost, env.rodsPort, {env.rodsUserName, env.rodsZone});
    } // constructor

    client_connection::client_connection(const std::string_view _host,
                                         const int _port,
                                         const fully_qualified_username& _username)
        : conn_{nullptr, rcDisconnect}
    {
        connect_and_login(std::string{_host}, _port, _username);
    } // constructor

    client_connection::client_connection(struct defer_authentication, // NOLINT(readability-named-parameter)
                                         const std::string_view _host,
                                         const int _port,
                                         const fully_qualified_username& _username)
        : conn_{nullptr, rcDisconnect}
    {
        only_connect(std::string{_host}, _port, _username);
    } // constructor

    client_connection::client_connection(const std::string_view _host,
                                         const int _port,
                                         const fully_qualified_username& _proxy_username,
                                         const fully_qualified_username& _username)
        : conn_{nullptr, rcDisconnect}
    {
        connect_and_login(std::string{_host}, _port, _proxy_username, _username);
    } // constructor

    client_connection::client_connection(struct defer_authentication, // NOLINT(readability-named-parameter)
                                         const std::string_view _host,
                                         const int _port,
                                         const fully_qualified_username& _proxy_username,
                                         const fully_qualified_username& _username)
        : conn_{nullptr, rcDisconnect}
    {
        only_connect(std::string{_host}, _port, _proxy_username, _username);
    } // constructor

    client_connection::client_connection(RcComm& _conn)
        : conn_{&_conn, rcDisconnect}
    {
    } // constructor

    client_connection::client_connection(struct defer_connection) // NOLINT(readability-named-parameter)
        : conn_{nullptr, rcDisconnect}
    {
    } // constructor

    auto client_connection::connect() -> void
    {
        rodsEnv env{};
        _getRodsEnv(env);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        connect_and_login(env.rodsHost, env.rodsPort, {env.rodsUserName, env.rodsZone});
    } // connect

    auto client_connection::connect(struct defer_authentication) -> void // NOLINT(readability-named-parameter)
    {
        rodsEnv env{};
        _getRodsEnv(env);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        only_connect(env.rodsHost, env.rodsPort, {env.rodsUserName, env.rodsZone});
    } // connect

    auto client_connection::connect(const std::string_view _host,
                                    const int _port,
                                    const fully_qualified_username& _username) -> void
    {
        connect_and_login(std::string{_host}, _port, _username);
    } // connect

    auto client_connection::connect(struct defer_authentication, // NOLINT(readability-named-parameter)
                                    const std::string_view _host,
                                    const int _port,
                                    const fully_qualified_username& _username) -> void
    {
        only_connect(std::string{_host}, _port, _username);
    } // connect

    auto client_connection::connect(struct defer_authentication, // NOLINT(readability-named-parameter)
                                    const std::string_view _host,
                                    const int _port,
                                    const fully_qualified_username& _proxy_username,
                                    const fully_qualified_username& _username) -> void
    {
        only_connect(std::string{_host}, _port, _proxy_username, _username);
    } // connect

    // NOLINTNEXTLINE(readability-named-parameter)
    auto client_connection::connect(struct defer_authentication, const fully_qualified_username& _proxy_username)
        -> void
    {
        rodsEnv env{};
        _getRodsEnv(env);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        only_connect(env.rodsHost, env.rodsPort, _proxy_username, {env.rodsUserName, env.rodsZone});
    } // connect

    auto client_connection::disconnect() noexcept -> void
    {
        conn_.reset();
    } // disconnect

    client_connection::operator bool() const noexcept
    {
        return static_cast<bool>(conn_);
    } // operator bool

    client_connection::operator RcComm&() const
    {
        if (!conn_) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(USER_SOCK_CONNECT_ERR, "Invalid client_connection object");
        }

        return *conn_;
    } // operator RcComm&

    client_connection::operator RcComm*() const noexcept
    {
        return conn_.get();
    } // operator RcComm*

    auto client_connection::connect_and_login(const std::string& _host,
                                              const int _port,
                                              const fully_qualified_username& _username) -> void
    {
        only_connect(_host, _port, _username);

        if (clientLogin(conn_.get()) != 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(USER_SOCK_CONNECT_ERR, "Client login error");
        }
    } // connect_and_login

    auto client_connection::connect_and_login(const std::string& _host,
                                              const int _port,
                                              const fully_qualified_username& _proxy_username,
                                              const fully_qualified_username& _username) -> void
    {
        only_connect(_host, _port, _proxy_username, _username);

        if (clientLogin(conn_.get()) != 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(USER_SOCK_CONNECT_ERR, "Client login error");
        }
    } // connect_and_login

    auto client_connection::only_connect(const std::string& _host,
                                         const int _port,
                                         const fully_qualified_username& _username) -> void
    {
        rErrMsg_t error{};
        conn_.reset(
            rcConnect(_host.c_str(), _port, _username.name().c_str(), _username.zone().c_str(), NO_RECONN, &error));

        if (!conn_) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(USER_SOCK_CONNECT_ERR, "Connect error");
        }
    } // only_connect

    auto client_connection::only_connect(const std::string& _host,
                                         const int _port,
                                         const fully_qualified_username& _proxy_username,
                                         const fully_qualified_username& _username) -> void
    {
        rErrMsg_t error{};
        conn_.reset(_rcConnect(_host.c_str(),
                               _port,
                               _proxy_username.name().c_str(),
                               _proxy_username.zone().c_str(),
                               _username.name().c_str(),
                               _username.zone().c_str(),
                               &error,
                               1,
                               NO_RECONN));

        if (!conn_) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(USER_SOCK_CONNECT_ERR, "Connect error");
        }
    } // only_connect
} // namespace irods::experimental
