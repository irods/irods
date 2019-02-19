#ifndef IRODS_CONNECTION_POOL_HPP
#define IRODS_CONNECTION_POOL_HPP

#include "rcConnect.h"

#include <memory>
#include <vector>
#include <mutex>
#include <atomic>
#include <string>
#include <functional>

namespace irods
{
    class connection_pool
    {
    public:
        // A wrapper around a connection in the pool.
        // On destruction, the underlying connection is immediately returned
        // to the pool.
        class connection_proxy
        {
        public:
            friend class connection_pool;

            connection_proxy(connection_proxy&&);
            connection_proxy& operator=(connection_proxy&&);

            ~connection_proxy();

            operator rcComm_t&() const noexcept;

        private:
            connection_proxy(connection_pool& _pool, rcComm_t& _conn, int _index) noexcept;

            static constexpr int uninitialized_index = -1;

            connection_pool* pool_;
            rcComm_t* conn_;
            int index_;
        };

        connection_pool(int _size,
                        const std::string& _host,
                        const int _port,
                        const std::string& _username,
                        const std::string& _zone,
                        const int _refresh_time);

        connection_pool(const connection_pool&) = delete;
        connection_pool& operator=(const connection_pool&) = delete;

        connection_proxy get_connection();

    private:
        using connection_pointer = std::unique_ptr<rcComm_t, int(*)(rcComm_t*)>;

        struct connection_context
        {
            std::mutex mutex{};
            std::atomic<bool> in_use{};
            connection_pointer conn{nullptr, rcDisconnect};
            rErrMsg_t error{};
            std::time_t creation_time{};
        };

        void create_connection(int _index,
                               std::function<void()> _on_connect_error,
                               std::function<void()> _on_login_error);

        rcComm_t* refresh_connection(int _index);

        bool verify_connection(int _index);

        void return_connection(int _index);

        const std::string host_;
        const int port_;
        const std::string username_;
        const std::string zone_;
        const int refresh_time_;
        std::vector<connection_context> conn_ctxs_;
    };
} // namespace irods

#endif // IRODS_CONNECTION_POOL_HPP

