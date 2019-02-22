#include "connection_pool.hpp"

#include "irods_query.hpp"
#include "thread_pool.hpp"

#include <stdexcept>
#include <thread>

namespace irods {

connection_pool::connection_proxy::~connection_proxy()
{
    if(pool_ && conn_ && uninitialized_index != index_) {
        pool_->return_connection(index_);
    }
}

connection_pool::connection_proxy::connection_proxy(
    connection_proxy&& _rhs)
    : pool_{_rhs.pool_}
    , conn_{_rhs.conn_}
    , index_{_rhs.index_}
{
    _rhs.pool_ = nullptr;
    _rhs.conn_ = nullptr;
    _rhs.index_ = uninitialized_index;
}

connection_pool::connection_proxy& connection_pool::connection_proxy::operator=(
    connection_proxy&& _rhs) {
    pool_ = _rhs.pool_;
    conn_ = _rhs.conn_;
    index_ = _rhs.index_;
    _rhs.pool_ = nullptr;
    _rhs.conn_ = nullptr;
    _rhs.index_ = uninitialized_index;
    return *this;
}

connection_pool::connection_proxy::operator rcComm_t&() const noexcept
{
    return *conn_;
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

    // Always initialize the first connection to guarantee that the
    // network plugin is loaded. This guarantees that asynchronous calls
    // to rcConnect do not cause a segfault.
    create_connection(0,
            [] { throw std::runtime_error{"connect error"}; },
            [] { throw std::runtime_error{"client login error"}; });

    // If the size of the pool is one, then return immediately.
    if (_size == 1) {
        return;
    }

    for (int i = 1; i < _size; ++i) {
        create_connection(i,
                [] { throw std::runtime_error{"connect error"}; },
                [] { throw std::runtime_error{"client login error"}; });
    }
}

void connection_pool::create_connection(
    int _index,
    std::function<void()> _on_conn_err,
    std::function<void()> _on_login_err)
{
    auto& context = conn_ctxs_[_index];
    context.creation_time = std::time(nullptr);
    context.conn.reset(rcConnect(host_.c_str(), port_, username_.c_str(), zone_.c_str(), NO_RECONN, &context.error));

    if (!context.conn) {
        _on_conn_err();
        return;
    }

    if (clientLogin(context.conn.get()) != 0) {
        _on_login_err();
    }
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

bool connection_pool::verify_connection(int _index) {
    auto& context = conn_ctxs_[_index];
    if(!context.conn) {
        return false;
    }
    try {
        irods::query<rcComm_t> qobj{context.conn.get(),
            "SELECT ZONE_NAME WHERE ZONE_TYPE = 'local'"};
        if(std::time(nullptr) - context.creation_time > refresh_time_) {
            return false;
        }
    } catch(const irods::exception&) {
        return false;
    }
    return true;
}

rcComm_t* connection_pool::refresh_connection(int _index) {
    // Reset rError stack to clear from possible previous use
    conn_ctxs_[_index].error = {};
    if(!verify_connection(_index)) {
        create_connection(_index,
            [] { throw std::runtime_error{"connect error"}; },
            [] { throw std::runtime_error{"login error"}; });
    }
    return conn_ctxs_[_index].conn.get();
}

} // namespace irods

