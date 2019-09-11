#ifndef IRODS_IO_DSTREAM_FACTORY
#define IRODS_IO_DSTREAM_FACTORY

#include "connection_pool.hpp"
#include "dstream.hpp"
#include "transport/default_transport.hpp"
#include "transport/transport_factory.hpp"

#include <iosfwd>
#include <string>
#include <iostream>
#include <memory>

namespace irods::experimental::io
{
    /// A stream factory that creates ::basic_dstream objects.
    ///
    /// Primarily meant to be used with the parallel_transfer_engine.
    ///
    /// \since 4.2.9
    template <typename CharT,
              typename Traits = std::char_traits<CharT>>
    class basic_dstream_factory
    {
    public:
        using transport_factory_type = transport_factory<CharT, Traits>;

        class stream final
            : public std::iostream
        {
        public:
            using transport_type = transport<CharT, Traits>;

            stream(std::unique_ptr<transport_type> _transport,
                   const std::string& _data_object_name,
                   std::ios_base::openmode _mode)
                : std::iostream{nullptr}
                , transport_{std::move(_transport)}
                , stream_{*transport_, _data_object_name, _mode}
            {
                std::iostream::rdbuf(stream_.rdbuf());
                this->setstate(stream_.rdstate());
            }

            stream(std::unique_ptr<transport_type> _transport,
                   const std::string& _data_object_name,
                   const replica_number& _replica_number,
                   std::ios_base::openmode _mode)
                : std::iostream{nullptr}
                , transport_{std::move(_transport)}
                , stream_{*transport_, _data_object_name, _replica_number, _mode}
            {
                std::iostream::rdbuf(stream_.rdbuf());
                this->setstate(stream_.rdstate());
            }

            stream(std::unique_ptr<transport_type> _transport,
                   const std::string& _data_object_name,
                   const root_resource_name& _root_resource_name,
                   std::ios_base::openmode _mode)
                : std::iostream{nullptr}
                , transport_{std::move(_transport)}
                , stream_{*transport_, _data_object_name, _root_resource_name, _mode}
            {
                std::iostream::rdbuf(stream_.rdbuf());
                this->setstate(stream_.rdstate());
            }

            stream(std::unique_ptr<transport_type> _transport,
                   const std::string& _data_object_name,
                   const leaf_resource_name& _leaf_resource_name,
                   std::ios_base::openmode _mode)
                : std::iostream{nullptr}
                , transport_{std::move(_transport)}
                , stream_{*transport_, _data_object_name, _leaf_resource_name, _mode}
            {
                std::iostream::rdbuf(stream_.rdbuf());
                this->setstate(stream_.rdstate());
            }

            stream(std::unique_ptr<transport_type> _transport,
                   const replica_token& _replica_token,
                   const std::string& _data_object_name,
                   const replica_number& _replica_number,
                   std::ios_base::openmode _mode)
                : std::iostream{nullptr}
                , transport_{std::move(_transport)}
                , stream_{*transport_, _replica_token, _data_object_name, _replica_number, _mode}
            {
                std::iostream::rdbuf(stream_.rdbuf());
                this->setstate(stream_.rdstate());
            }

            stream(std::unique_ptr<transport_type> _transport,
                   const replica_token& _replica_token,
                   const std::string& _data_object_name,
                   const leaf_resource_name& _leaf_resource_name,
                   std::ios_base::openmode _mode)
                : std::iostream{nullptr}
                , transport_{std::move(_transport)}
                , stream_{*transport_, _replica_token, _data_object_name, _leaf_resource_name, _mode}
            {
                std::iostream::rdbuf(stream_.rdbuf());
                this->setstate(stream_.rdstate());
            }

            stream(stream&& _other)
                : std::iostream{std::move(_other)}
                , transport_{std::move(_other.transport_)}
                , stream_{std::move(_other.stream_)}
            {
                this->set_rdbuf(stream_.rdbuf());
                this->setstate(stream_.rdstate());
            }

            stream& operator=(stream&& _other)
            {
                std::iostream::operator=(std::move(_other));
                transport_ = std::move(_other.transport_);
                stream_ = std::move(_other.stream_);
                this->set_rdbuf(stream_.rdbuf());
                this->setstate(stream_.rdstate());
                
                return *this;
            }

            void close(const irods::experimental::io::on_close_success* _p = nullptr)
            {
                stream_.close(_p);
            }

            io::data_object_buf* rdbuf() const
            {
                return const_cast<io::data_object_buf*>(stream_.rdbuf());
            }

            const root_resource_name& root_resource_name() const
            {
                return stream_.root_resource_name();
            }

            const leaf_resource_name& leaf_resource_name() const
            {
                return stream_.leaf_resource_name();
            }

            const replica_number& replica_number() const
            {
                return stream_.replica_number();
            }

            const replica_token& replica_token() const
            {
                return stream_.replica_token();
            }

        private:
            std::unique_ptr<transport_type> transport_;
            io::dstream stream_;
        }; // class stream

        using stream_type = stream;

        basic_dstream_factory(transport_factory_type& _transport_factory,
                              const std::string& _data_object_name)
            : transport_factory_{_transport_factory}
            , data_object_name_{_data_object_name}
        {
        }

        auto operator()(std::ios_base::openmode _mode) const -> stream_type
        {
            std::unique_ptr<typename stream_type::transport_type> tp{transport_factory_()};
            return stream{std::move(tp), data_object_name_, _mode};
        }

        auto operator()(const replica_number& _replica_number, std::ios_base::openmode _mode) const -> stream_type
        {
            std::unique_ptr<typename stream_type::transport_type> tp{transport_factory_()};
            return stream{std::move(tp), data_object_name_, _replica_number, _mode};
        }

        auto operator()(const root_resource_name& _root_resource_name, std::ios_base::openmode _mode) const -> stream_type
        {
            std::unique_ptr<typename stream_type::transport_type> tp{transport_factory_()};
            return stream{std::move(tp), data_object_name_, _root_resource_name, _mode};
        }

        auto operator()(const leaf_resource_name& _leaf_resource_name, std::ios_base::openmode _mode) const -> stream_type
        {
            std::unique_ptr<typename stream_type::transport_type> tp{transport_factory_()};
            return stream{std::move(tp), data_object_name_, _leaf_resource_name, _mode};
        }

        auto operator()(const replica_token& _replica_token,
                        const replica_number& _replica_number,
                        std::ios_base::openmode _mode) const -> stream_type
        {
            std::unique_ptr<typename stream_type::transport_type> tp{transport_factory_()};
            return stream{std::move(tp), _replica_token, data_object_name_, _replica_number, _mode};
        }

        auto operator()(const replica_token& _replica_token,
                        const leaf_resource_name& _leaf_resource_name,
                        std::ios_base::openmode _mode) const -> stream_type
        {
            std::unique_ptr<typename stream_type::transport_type> tp{transport_factory_()};
            return stream{std::move(tp), _replica_token, data_object_name_, _leaf_resource_name, _mode};
        }

        /// Returns the name of the data object.
        auto object_name() const noexcept -> const std::string&
        {
            return data_object_name_;
        }

        /// Signals to the parallel_transfer_engine whether the stream objects created
        /// by the factory support dstream::close.
        ///
        /// \return A boolean.
        /// \retval true  Tells the parallel_transfer_engine that the streams support dstream::close.
        /// \retval false Otherwise.
        static constexpr auto sync_with_physical_object() noexcept -> bool
        {
            return true;
        }

    private:
        transport_factory_type& transport_factory_;
        std::string data_object_name_;
    }; // class dstream_factory

    using dstream_factory = basic_dstream_factory<char>;
} // namespace irods::experimental::io

#endif // IRODS_IO_DSTREAM_FACTORY
