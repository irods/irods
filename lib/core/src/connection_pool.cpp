#include "connection_pool.hpp"

#include "thread_pool.hpp"

#include <stdexcept>
#include <thread>

namespace irods {

connection_pool::connection_proxy::~connection_proxy()
{
    pool_.return_connection(index_);
}

connection_pool::connection_proxy::operator rcComm_t&() const noexcept
{
    return conn_;
}

connection_pool::connection_proxy::connection_proxy(connection_pool& _pool,
                                                    rcComm_t& _conn,
                                                    int _index) noexcept
    : pool_{_pool}
    , conn_{_conn}
    , index_{_index}
{
}

connection_pool::connection_pool(int _size,
                                 const std::string& _host,
                                 const int _port,
                                 const std::string& _username,
                                 const std::string& _password,
                                 const std::string& _zone)
    : conn_ctxs_(_size)
{
    if (_size < 1) {
        throw std::runtime_error{"invalid connection pool size"};
    }

    const auto connect = [&](auto& _conn, auto _on_connect_error, auto _on_login_error)
    {
        rErrMsg_t errors;

        _conn.reset(rcConnect(_host.c_str(), _port, _username.c_str(), _zone.c_str(), 0, &errors));

        if (!_conn) {
            _on_connect_error();
            return;
        }

        if (clientLoginWithPassword(_conn.get(), const_cast<char*>(_password.c_str())) != 0) {
            _on_login_error();
        }
    };

    // Always initialize the first connection to guarantee that the
    // network plugin is loaded. This guarantees that asynchronous calls
    // to rcConnect do not cause a segfault.
    connect(conn_ctxs_[0].conn,
            [] { throw std::runtime_error{"connect error"}; },
            [] { throw std::runtime_error{"client login error"}; });

    // If the size of the pool is one, then return immediately.
    if (_size == 1) {
        return;
    }

    // Initialize the rest of the connection pool asynchronously.

    irods::thread_pool thread_pool{std::min<int>(_size, std::thread::hardware_concurrency())};

    std::atomic<bool> connect_error{};
    std::atomic<bool> login_error{};

    for (int i = 1; i < _size; ++i) {
        auto& ctx = conn_ctxs_[i];

        irods::thread_pool::post(thread_pool, [&connect, &connect_error, &login_error, &ctx] {
            if (connect_error.load() || login_error.load()) {
                return;
            }

            connect(ctx.conn,
                    [&connect_error] { connect_error.store(true); },
                    [&login_error] { login_error.store(true); });
        });
    }

    thread_pool.join();

    if (connect_error.load()) {
        throw std::runtime_error{"connect error"};
    }

    if (login_error.load()) {
        throw std::runtime_error{"client login error"};
    }
}

connection_pool::connection_proxy connection_pool::get_connection()
{
    for (int i = 0;; i = ++i % conn_ctxs_.size()) {
        std::unique_lock lock{conn_ctxs_[i].mutex, std::defer_lock};

        if (lock.try_lock()) {
            if (!conn_ctxs_[i].in_use.load()) {
                conn_ctxs_[i].in_use.store(true);
                return {*this, *conn_ctxs_[i].conn, i};
            }
        }
    }
}

void connection_pool::return_connection(int _index)
{
    conn_ctxs_[_index].in_use.store(false);
}

} // namespace irods

