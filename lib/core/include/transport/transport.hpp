#ifndef IRODS_IO_TRANSPORT_HPP
#define IRODS_IO_TRANSPORT_HPP

#include "filesystem/path.hpp"
#include "acl_operation.hpp"
#include "metadata_operation.hpp"

#include <ios>
#include <string>
#include <vector>

namespace irods::experimental::io
{
    struct on_close_success
    {
        bool update_catalog = true;
        std::vector<irods::experimental::metadata_operation> metadata_ops;
        std::vector<irods::experimental::acl_operation> acl_ops;
    };

    template <typename CharT,
              typename Traits = std::char_traits<CharT>>
    class transport
    {
    public:
        // clang-format off
        using char_type   = CharT;
        using traits_type = Traits;
        using int_type    = typename traits_type::int_type;
        using pos_type    = typename traits_type::pos_type;
        using off_type    = typename traits_type::off_type;
        // clang-format on

        virtual ~transport() {}

        virtual bool open(const irods::experimental::filesystem::path& _p,
                          std::ios_base::openmode _mode) = 0;

        virtual bool open(const irods::experimental::filesystem::path& _p,
                          int _replica_number,
                          std::ios_base::openmode _mode) = 0;

        virtual bool open(const irods::experimental::filesystem::path& _p,
                          const std::string& _resource_name,
                          std::ios_base::openmode _mode) = 0;

        virtual bool close(const on_close_success* _on_close_success = nullptr) = 0;

        virtual std::streamsize receive(char_type* _buffer, std::streamsize _buffer_size) = 0;

        virtual std::streamsize send(const char_type* _buffer, std::streamsize _buffer_size) = 0;

        virtual pos_type seekpos(off_type _offset, std::ios_base::seekdir _dir) = 0;

        virtual bool is_open() const noexcept = 0;

        virtual int file_descriptor() const noexcept = 0;

        virtual std::string resource_name() const = 0;

        virtual std::string resource_hierarchy() const = 0;

        virtual int replica_number() const = 0;
    };
} // irods::experimental::io

#endif // IRODS_IO_TRANSPORT_HPP
