#include "client_connection.hpp"

#include "rcConnect.h"
#include "rodsErrorTable.h"
#include "irods_exception.hpp"

namespace irods::experimental
{
    client_connection::client_connection()
        : conn_{nullptr, rcDisconnect}
    {
        rodsEnv env{};
        _getRodsEnv(env);

        connect_and_login(env.rodsHost, env.rodsPort, env.rodsUserName, env.rodsZone);
    }

    client_connection::client_connection(const std::string_view _host,
                                         const int _port,
                                         const std::string_view _username,
                                         const std::string_view _zone)
        : conn_{nullptr, rcDisconnect}
    {
        connect_and_login(_host, _port, _username, _zone);
    }

    client_connection::client_connection(const std::string_view _host,
                                         const int _port,
                                         const std::string_view _proxy_user,
                                         const std::string_view _proxy_zone,
                                         const std::string_view _username,
                                         const std::string_view _zone)
        : conn_{nullptr, rcDisconnect}
    {
        connect_and_login(_host, _port, _proxy_user,
                          _proxy_zone, _username,
                          _zone);
    }

    client_connection::client_connection(RcComm& _conn)
        : conn_{&_conn, rcDisconnect}
    {
    }

    client_connection::client_connection(struct defer_connection)
        : conn_{nullptr, rcDisconnect}
    {
    }

    auto client_connection::connect() -> void
    {
        rodsEnv env{};
        _getRodsEnv(env);

        connect_and_login(env.rodsHost, env.rodsPort, env.rodsUserName, env.rodsZone);
    }

    auto client_connection::connect(const std::string_view _host,
                                    const int _port,
                                    const std::string_view _username,
                                    const std::string_view _zone) -> void
    {
        connect_and_login(_host, _port, _username, _zone);
    }

    auto client_connection::connect(const std::string_view _proxy_user,
                                    const std::string_view _proxy_zone) -> void
    {
        rodsEnv env{};
        _getRodsEnv(env);

        connect_and_login(env.rodsHost, env.rodsPort, _proxy_user, _proxy_zone,
                          env.rodsUserName, env.rodsZone);
    }

    auto client_connection::disconnect() noexcept -> void
    {
        conn_.reset();
    }

    client_connection::operator bool() const noexcept
    {
        return static_cast<bool>(conn_);
    }

    client_connection::operator RcComm&() const
    {
        if (!conn_) {
            THROW(USER_SOCK_CONNECT_ERR, "invalid client_connection object");
        }

        return *conn_;
    }

    client_connection::operator RcComm*() const noexcept
    {
        return conn_.get();
    }

    auto client_connection::connect_and_login(const std::string_view _host,
                                              const int _port,
                                              const std::string_view _username,
                                              const std::string_view _zone) -> void
    {
        rErrMsg_t error{};
        conn_.reset(rcConnect(_host.data(),
                              _port,
                              _username.data(),
                              _zone.data(),
                              NO_RECONN,
                              &error));

        if (!conn_) {
            THROW(USER_SOCK_CONNECT_ERR, "connect error");
        }

        if (clientLogin(conn_.get()) != 0) {
            THROW(USER_SOCK_CONNECT_ERR, "client login error");
        }
    }

    auto client_connection::connect_and_login(const std::string_view _host,
                                              const int _port,
                                              const std::string_view _proxy_user,
                                              const std::string_view _proxy_zone,
                                              const std::string_view _username,
                                              const std::string_view _zone) -> void
    {
        rErrMsg_t error{};
        conn_.reset(_rcConnect(_host.data(),
                               _port,
                               _proxy_user.data(),
                               _proxy_zone.data(),
                               _username.data(),
                               _zone.data(),
                               &error,
                               1,
                               NO_RECONN));
        
        if (!conn_) {
            THROW(USER_SOCK_CONNECT_ERR, "connect error");
        }

        if (clientLogin(conn_.get()) != 0) {
            THROW(USER_SOCK_CONNECT_ERR, "client login error");
        }
    }
} // namespace irods::experimental

