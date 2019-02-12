#ifndef IRODS_CONNECTION_POOL_HPP
#define IRODS_CONNECTION_POOL_HPP

#include "rcConnect.h"

#include <memory>
#include <vector>
#include <mutex>
#include <atomic>

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

            connection_proxy(const connection_proxy&) = delete;
            connection_proxy& operator=(const connection_proxy&) = delete;

            ~connection_proxy();

            operator rcComm_t&() const noexcept;

        private:
            connection_proxy(connection_pool& _pool, rcComm_t& _conn, int _index) noexcept;

            connection_pool& pool_;
            rcComm_t& conn_;
            int index_;
        };

        connection_pool(int _size,
                        const std::string& _host,
                        const int _port,
                        const std::string& _username,
                        const std::string& _password,
                        const std::string& _zone);

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
        };

        void return_connection(int _index);

        std::vector<connection_context> conn_ctxs_;
    };
} // namespace irods

#endif // IRODS_CONNECTION_POOL_HPP

