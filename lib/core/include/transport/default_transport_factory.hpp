#ifndef IRODS_DEFAULT_TRANSPORT_FACTORY_HPP
#define IRODS_DEFAULT_TRANSPORT_FACTORY_HPP

#include "transport_factory.hpp"
#include "transport/default_transport.hpp"

#include "connection_pool.hpp"

namespace irods::experimental::io
{
    template <typename CharT,
              typename Traits = std::char_traits<CharT>>
    class basic_transport_factory
        : public transport_factory<CharT, Traits>
    {
    public:
        class transport final
            : public io::client::native_transport
        {
        public:
            explicit transport(connection_pool::connection_proxy conn)
                : io::client::native_transport{conn}
                , conn_{std::move(conn)}
            {
            }

        private:
            connection_pool::connection_proxy conn_;
        }; // class transport

        explicit basic_transport_factory(irods::connection_pool& conn_pool)
            : conn_pool_{conn_pool}
        {
        }

        transport* operator()() override
        {
            return new transport{conn_pool_.get_connection()};
        }

    private:
        irods::connection_pool& conn_pool_;
    }; // class basic_transport_factory

    // clang-format off
    using default_transport_factory = basic_transport_factory<char>;
    using native_transport_factory  = basic_transport_factory<char>;
    // clang-format on
} // namespace irods::experimental::io

#endif // IRODS_DEFAULT_TRANSPORT_FACTORY_HPP
