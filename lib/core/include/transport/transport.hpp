#ifndef IRODS_IO_TRANSPORT_HPP
#define IRODS_IO_TRANSPORT_HPP

#include "filesystem/path.hpp"

#include <ios>
#include <string>

namespace irods {
namespace experimental {
namespace io {

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

        virtual bool close() = 0;

        virtual std::streamsize receive(char_type* _buffer, std::streamsize _buffer_size) = 0;

        virtual std::streamsize send(const char_type* _buffer, std::streamsize _buffer_size) = 0;

        virtual pos_type seekpos(off_type _offset, std::ios_base::seekdir _dir) = 0;

        virtual bool is_open() const noexcept = 0;

        virtual int file_descriptor() const noexcept = 0;
    };

} // io
} // experimental
} // irods

#endif // IRODS_IO_TRANSPORT_HPP

