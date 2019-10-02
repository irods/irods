#include "connection_pool.hpp"

#include "irods_query.hpp"

#include <stdexcept>

namespace irods {

connection_pool::connection_proxy::~connection_proxy()
{
    if (pool_ && uninitialized_index != index_) {
        pool_->return_connection(index_);
    }
}

connection_pool::connection_proxy::connection_proxy(connection_proxy&& _other)
    : pool_{_other.pool_}
    , conn_{_other.conn_}
    , index_{_other.index_}
{
    _other.pool_ = nullptr;
    _other.conn_ = nullptr;
    _other.index_ = uninitialized_index;
}

connection_pool::connection_proxy& connection_pool::connection_proxy::operator=(connection_proxy&& _other)
{
    pool_ = _other.pool_;
    conn_ = _other.conn_;
    index_ = _other.index_;

    _other.pool_ = nullptr;
    _other.conn_ = nullptr;
    _other.index_ = uninitialized_index;

    return *this;
}

connection_pool::connection_proxy::operator bool() const noexcept
{
    return nullptr != conn_;
}

connection_pool::connection_proxy::operator rcComm_t&() const
{
    if (!conn_) {
        throw std::runtime_error{"Invalid connection object"};
    }

    return *conn_;
}

connection_pool::connection_proxy::operator rcComm_t*() const noexcept
{
    return conn_;
}

rcComm_t* connection_pool::connection_proxy::release()
{
    pool_->release_connection(index_);
    auto conn = conn_;
    conn_ = nullptr;
    return conn;
}

connection_pool::connection_proxy::connection_proxy(connection_pool& _pool,
                                                    rcComm_t& _conn,
                                                    int _index) noexcept
    : pool_{&_pool}
    , conn_{&_conn}
    , index_{_index}
{
}

connection_pool::connection_pool(int _size,
                                 const std::string& _host,
                                 const int _port,
                                 const std::string& _username,
                                 const std::string& _zone,
                                 const int _refresh_time)
    : host_{_host}
    , port_{_port}
    , username_{_username}
    , zone_{_zone}
    , refresh_time_(_refresh_time)
    , conn_ctxs_(_size)
{
    if (_size < 1) {
        throw std::runtime_error{"invalid connection pool size"};
    }

    for (int i = 0; i < _size; ++i) {
        create_connection(i,
                          [] { throw std::runtime_error{"connect error"}; },
                          [] { throw std::runtime_error{"client login error"}; });
    }
}

void connection_pool::create_connection(int _index,
                                        std::function<void()> _on_connect_error,
                                        std::function<void()> _on_login_error)
{
    auto& ctx = conn_ctxs_[_index];
    ctx.creation_time = std::time(nullptr);
    ctx.conn.reset(rcConnect(host_.c_str(),
                             port_,
                             username_.c_str(),
                             zone_.c_str(),
                             NO_RECONN,
                             &ctx.error));

    if (!ctx.conn) {
        _on_connect_error();
        return;
    }

    if (clientLogin(ctx.conn.get()) != 0) {
        _on_login_error();
    }
}

bool connection_pool::verify_connection(int _index)
{
    auto& ctx = conn_ctxs_[_index];

    if (!ctx.conn) {
        return false;
    }

    try {
        query<rcComm_t>{ctx.conn.get(), "select ZONE_NAME where ZONE_TYPE = 'local'"};
        if (std::time(nullptr) - ctx.creation_time > refresh_time_) {
            return false;
        }
    }
    catch (const std::exception&) {
        return false;
    }

    return true;
}

rcComm_t* connection_pool::refresh_connection(int _index)
{
    auto& ctx = conn_ctxs_[_index];
    ctx.error = {};

    if (ctx.refresh) {
        ctx.refresh = false;
        ctx.conn.release();
    }

    if (!verify_connection(_index)) {
        create_connection(_index,
                          [] { throw std::runtime_error{"connect error"}; },
                          [] { throw std::runtime_error{"client login error"}; });
    }

    return ctx.conn.get();
}

connection_pool::connection_proxy connection_pool::get_connection()
{
    for (int i = 0;; i = ++i % conn_ctxs_.size()) {
        std::unique_lock<std::mutex> lock{conn_ctxs_[i].mutex, std::defer_lock};

        if (lock.try_lock()) {
            if (!conn_ctxs_[i].in_use.load()) {
                conn_ctxs_[i].in_use.store(true);
                return {*this, *refresh_connection(i), i};
            }
        }
    }
}

void connection_pool::return_connection(int _index)
{
    conn_ctxs_[_index].in_use.store(false);
}

void connection_pool::release_connection(int _index)
{
    conn_ctxs_[_index].refresh = true;
}

} // namespace irods

