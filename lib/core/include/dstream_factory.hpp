#ifndef IRODS_IO_DSTREAM_FACTORY
#define IRODS_IO_DSTREAM_FACTORY

#include "connection_pool.hpp"
#include "dstream.hpp"
#include "transport/default_transport.hpp"

#include <string>
#include <iostream>
#include <memory>

namespace irods::experimental::io
{
    class dstream_factory
    {
    public:
        class stream final
            : public std::iostream
        {
        public:
            stream(irods::connection_pool::connection_proxy&& _conn,
                   const std::string& _data_object_name,
                   std::ios_base::openmode _mode)
                : std::iostream{nullptr}
                , conn_{std::move(_conn)}
                , transport_{std::make_unique<io::client::default_transport>(conn_)}
                , stream_{*transport_, _data_object_name, _mode}
            {
                std::iostream::rdbuf(stream_.rdbuf());
            }

            stream(irods::connection_pool::connection_proxy&& _conn,
                   const std::string& _data_object_name,
                   int _replica_number,
                   std::ios_base::openmode _mode)
                : std::iostream{nullptr}
                , conn_{std::move(_conn)}
                , transport_{std::make_unique<io::client::default_transport>(conn_)}
                , stream_{*transport_, _data_object_name, _replica_number, _mode}
            {
                std::iostream::rdbuf(stream_.rdbuf());
            }

            stream(irods::connection_pool::connection_proxy&& _conn,
                   const std::string& _data_object_name,
                   const std::string& _resource_name,
                   std::ios_base::openmode _mode)
                : std::iostream{nullptr}
                , conn_{std::move(_conn)}
                , transport_{std::make_unique<io::client::default_transport>(conn_)}
                , stream_{*transport_, _data_object_name, _resource_name, _mode}
            {
                std::iostream::rdbuf(stream_.rdbuf());
            }

            stream(stream&& _other)
                : std::iostream{std::move(_other)}
                , conn_{std::move(_other.conn_)}
                , transport_{std::move(_other.transport_)}
                , stream_{std::move(_other.stream_)}
            {
                this->set_rdbuf(stream_.rdbuf());
            }

            stream& operator=(stream&& _other)
            {
                std::iostream::operator=(std::move(_other));
                conn_ = std::move(_other.conn_);
                transport_ = std::move(_other.transport_);
                stream_ = std::move(_other.stream_);
                this->set_rdbuf(stream_.rdbuf());
                
                return *this;
            }

            io::data_object_buf* rdbuf() const
            {
                return const_cast<io::data_object_buf*>(stream_.rdbuf());
            }

            std::string resource_name() const
            {
                return stream_.resource_name();
            }

            std::string resource_hierarchy() const
            {
                return stream_.resource_hierarchy();
            }

            int replica_number() const
            {
                return stream_.replica_number();
            }

            void close(const irods::experimental::io::on_close_success* _p)
            {
                stream_.close(_p);
            }

        private:
            irods::connection_pool::connection_proxy conn_;
            std::unique_ptr<io::client::default_transport> transport_;
            io::dstream stream_;
        };

        using stream_type = stream;

        dstream_factory(irods::connection_pool& _conn_pool,
                        const std::string& _data_object_name)
            : conn_pool_{_conn_pool}
            , data_object_name_{_data_object_name}
        {
        }

        auto operator()(std::ios_base::openmode _mode) const -> stream_type
        {
            return stream{conn_pool_.get_connection(), data_object_name_, _mode};
        }

        auto operator()(int _replica_number, std::ios_base::openmode _mode) const -> stream_type
        {
            return stream{conn_pool_.get_connection(), data_object_name_, _replica_number, _mode};
        }

        auto operator()(const std::string& _resource_name, std::ios_base::openmode _mode) const -> stream_type
        {
            return stream{conn_pool_.get_connection(), data_object_name_, _resource_name, _mode};
        }

        auto object_name() const noexcept -> const std::string&
        {
            return data_object_name_;
        }

        static constexpr auto sync_with_physical_object() noexcept -> bool
        {
            return true;
        }

    private:
        irods::connection_pool& conn_pool_;
        std::string data_object_name_;
    };
} // namespace irods::experimental::io

#endif // IRODS_IO_DSTREAM_FACTORY
