#include "irods/connection_pool.hpp"

#include "irods/irods_query.hpp"
#include "irods/thread_pool.hpp"

#include <stdexcept>
#include <thread>
#include <tuple> // For std::ignore.

namespace irods
{
    //
    // Connection Proxy Implementation
    //

    connection_pool::connection_proxy::connection_proxy()
        : pool_{}
        , conn_{}
        , index_{uninitialized_index}
    {
    } // default constructor

    connection_pool::connection_proxy::connection_proxy(connection_proxy&& _other) noexcept
        : pool_{_other.pool_}
        , conn_{_other.conn_}
        , index_{_other.index_}
    {
        _other.pool_ = nullptr;
        _other.conn_ = nullptr;
        _other.index_ = uninitialized_index;
    } // move constructor

    connection_pool::connection_proxy& connection_pool::connection_proxy::operator=(connection_proxy&& _other) noexcept
    {
        pool_ = _other.pool_;
        conn_ = _other.conn_;
        index_ = _other.index_;

        _other.pool_ = nullptr;
        _other.conn_ = nullptr;
        _other.index_ = uninitialized_index;

        return *this;
    } // operator=

    connection_pool::connection_proxy::~connection_proxy()
    {
        // NOLINTNEXTLINE(readability-implicit-bool-conversion)
        if (pool_ && uninitialized_index != index_) {
            pool_->return_connection(index_);
        }
    } // destructor

    connection_pool::connection_proxy::operator bool() const noexcept
    {
        return nullptr != conn_;
    } // operator bool

    connection_pool::connection_proxy::operator RcComm&() const
    {
        if (!conn_) { // NOLINT(readability-implicit-bool-conversion)
            throw std::runtime_error{"Invalid connection object"};
        }

        return *conn_;
    } // operator RcComm&

    connection_pool::connection_proxy::operator RcComm*() const noexcept
    {
        return conn_;
    } // operator RcComm*

    RcComm* connection_pool::connection_proxy::release()
    {
        pool_->release_connection(index_);
        auto* conn = conn_;
        conn_ = nullptr;
        return conn;
    } // release

    connection_pool::connection_proxy::connection_proxy(connection_pool& _pool, RcComm& _conn, int _index) noexcept
        : pool_{&_pool}
        , conn_{&_conn}
        , index_{_index}
    {
    } // constructor

    //
    // Connection Pool Implementation
    //

    connection_pool::connection_pool(int _size,
                                     const std::string& _host,
                                     const int _port,
                                     const std::string& _name,
                                     const std::string& _zone,
                                     const int _refresh_time)
        : connection_pool{_size, _host, _port, std::nullopt, {_name, _zone}, _refresh_time, {}}
    {
    } // constructor

    connection_pool::connection_pool(int _size,
                                     const std::string& _host,
                                     const int _port,
                                     experimental::fully_qualified_username _username,
                                     const int _refresh_time)
        : connection_pool{_size, _host, _port, std::nullopt, std::move(_username), _refresh_time, {}}
    {
    } // constructor

    connection_pool::connection_pool(int _size,
                                     const std::string& _host,
                                     const int _port,
                                     experimental::fully_qualified_username _username,
                                     const int _refresh_time,
                                     std::function<void(RcComm&)> _auth_func)
        : connection_pool{_size, _host, _port, std::nullopt, std::move(_username), _refresh_time, std::move(_auth_func)}
    {
    } // constructor

    connection_pool::connection_pool(int _size,
                                     std::string_view _host,
                                     const int _port,
                                     std::optional<experimental::fully_qualified_username> _proxy_username,
                                     experimental::fully_qualified_username _username,
                                     const int _refresh_time,
                                     std::function<void(RcComm&)> _auth_func)
        : host_{_host}
        , port_{_port}
        , proxy_username_{std::move(_proxy_username)}
        , username_{std::move(_username)}
        , refresh_time_(_refresh_time)
        , auth_func_{std::move(_auth_func)}
        , conn_ctxs_(_size)
    {
        if (_size < 1) {
            throw std::runtime_error{"Invalid connection pool size"}; // TODO These should be irods::exceptions.
        }

        // Always initialize the first connection to guarantee that the
        // network plugin is loaded. This guarantees that asynchronous calls
        // to rcConnect do not cause a segfault.
        create_connection(
            0,
            [] { throw std::runtime_error{"Connect error"}; },
            [] { throw std::runtime_error{"Client login error"}; });

        // If the size of the pool is one, then return immediately.
        if (_size == 1) {
            return;
        }

        // Initialize the rest of the connection pool asynchronously.

        irods::thread_pool thread_pool{std::min<int>(_size, std::thread::hardware_concurrency())};

        std::atomic<bool> connect_error{};
        std::atomic<bool> login_error{};

        const auto on_connect_error = [&connect_error] { connect_error.store(true); };
        const auto on_login_error = [&login_error] { login_error.store(true); };

        for (int i = 1; i < _size; ++i) {
            irods::thread_pool::post(
                thread_pool, [this, i, &connect_error, &login_error, &on_connect_error, &on_login_error] {
                    if (connect_error.load() || login_error.load()) {
                        return;
                    }

                    create_connection(i, on_connect_error, on_login_error);
                });
        }

        thread_pool.join();

        if (connect_error.load()) {
            throw std::runtime_error{"Connect error"};
        }

        if (login_error.load()) {
            throw std::runtime_error{"Client login error"};
        }
    } // constructor

    void connection_pool::create_connection(int _index,
                                            const std::function<void()>& _on_connect_error,
                                            const std::function<void()>& _on_login_error)
    {
        auto& ctx = conn_ctxs_[_index];
        ctx.creation_time = std::time(nullptr);

        if (proxy_username_.has_value()) {
            ctx.conn.reset(_rcConnect(host_.c_str(),
                                      port_,
                                      proxy_username_->name().c_str(),
                                      proxy_username_->zone().c_str(),
                                      username_.name().c_str(),
                                      username_.zone().c_str(),
                                      &ctx.error,
                                      1,
                                      NO_RECONN));
        }
        else {
            ctx.conn.reset(rcConnect(
                host_.c_str(), port_, username_.name().c_str(), username_.zone().c_str(), NO_RECONN, &ctx.error));
        }

        if (!ctx.conn) {
            _on_connect_error();
            return;
        }

        if (auth_func_) {
            auth_func_(*ctx.conn);
            return;
        }

        if (clientLogin(ctx.conn.get()) != 0) {
            _on_login_error();
        }
    } // create_connection

    bool connection_pool::verify_connection(int _index)
    {
        auto& ctx = conn_ctxs_[_index];

        if (!ctx.conn) {
            return false;
        }

        try {
            // NOLINTNEXTLINE(bugprone-unused-raii)
            query<RcComm>{ctx.conn.get(), "select ZONE_NAME where ZONE_TYPE = 'local'"};
            if (std::time(nullptr) - ctx.creation_time > refresh_time_) {
                return false;
            }
        }
        catch (const std::exception&) {
            return false;
        }

        return true;
    } // verify_connection

    RcComm* connection_pool::refresh_connection(int _index)
    {
        auto& ctx = conn_ctxs_[_index];
        ctx.error = {};

        if (ctx.refresh) {
            ctx.refresh = false;
        }

        if (!verify_connection(_index)) {
            create_connection(
                _index,
                [] { throw std::runtime_error{"Connect error"}; },
                [] { throw std::runtime_error{"Client login error"}; });
        }

        return ctx.conn.get();
    } // refresh_connection

    connection_pool::connection_proxy connection_pool::get_connection()
    {
        for (int i = 0;; i = (i + 1) % static_cast<int>(conn_ctxs_.size())) {
            std::unique_lock<std::mutex> lock{conn_ctxs_[i].mutex, std::defer_lock};

            if (lock.try_lock()) {
                if (!conn_ctxs_[i].in_use.load()) {
                    conn_ctxs_[i].in_use.store(true);
                    return {*this, *refresh_connection(i), i};
                }
            }
        }
    } // get_connection

    void connection_pool::return_connection(int _index)
    {
        conn_ctxs_[_index].in_use.store(false);
    } // return_connection

    void connection_pool::release_connection(int _index)
    {
        conn_ctxs_[_index].refresh = true;
        std::ignore = conn_ctxs_[_index].conn.release();
    } // release_connection

    std::shared_ptr<connection_pool> make_connection_pool(int _size)
    {
        rodsEnv env{};
        _getRodsEnv(env);
        return std::make_shared<irods::connection_pool>(
            _size, env.rodsHost, env.rodsPort, env.rodsUserName, env.rodsZone, env.irodsConnectionPoolRefreshTime);
    } // make_connection_pool
} // namespace irods
