#ifndef IRODS_IO_DEFAULT_TRANSPORT_HPP
#define IRODS_IO_DEFAULT_TRANSPORT_HPP

#undef NAMESPACE_IMPL
#undef rxComm
#undef rxDataObjOpen
#undef rxDataObjRead
#undef rxDataObjWrite
#undef rxDataObjClose
#undef rxDataObjLseek

// clang-format off
#ifdef IRODS_IO_TRANSPORT_ENABLE_SERVER_SIDE_API
    #include "rsDataObjOpen.hpp"
    #include "rsDataObjRead.hpp"
    #include "rsDataObjWrite.hpp"
    #include "rsDataObjClose.hpp"
    #include "rsDataObjLseek.hpp"

    #define NAMESPACE_IMPL      server

    #define rxComm              rsComm_t

    #define rxDataObjOpen       rsDataObjOpen
    #define rxDataObjRead       rsDataObjRead
    #define rxDataObjWrite      rsDataObjWrite
    #define rxDataObjClose      rsDataObjClose
    #define rxDataObjLseek      rsDataObjLseek
#else
    #include "dataObjOpen.h"
    #include "dataObjRead.h"
    #include "dataObjWrite.h"
    #include "dataObjClose.h"
    #include "dataObjLseek.h"

    #define NAMESPACE_IMPL      client

    #define rxComm              rcComm_t

    #define rxDataObjOpen       rcDataObjOpen
    #define rxDataObjRead       rcDataObjRead
    #define rxDataObjWrite      rcDataObjWrite
    #define rxDataObjClose      rcDataObjClose
    #define rxDataObjLseek      rcDataObjLseek
#endif // IRODS_IO_TRANSPORT_ENABLE_SERVER_SIDE_API
// clang-format on

#include "rcMisc.h"
#include "transport/transport.hpp"

#include <string>

namespace irods::experimental::io::NAMESPACE_IMPL
{
    template <typename CharT>
    class basic_transport : public transport<CharT>
    {
    public:
        // clang-format off
        using char_type   = typename transport<CharT>::char_type;
        using traits_type = typename transport<CharT>::traits_type;
        using int_type    = typename traits_type::int_type;
        using pos_type    = typename traits_type::pos_type;
        using off_type    = typename traits_type::off_type;
        // clang-format on

    private:
        // clang-format off
        inline static constexpr auto uninitialized_file_descriptor = -1;
        inline static constexpr auto minimum_valid_file_descriptor = 3;

        // Errors
        inline static constexpr auto translation_error             = -1;
        inline static const     auto seek_error                    = pos_type{off_type{-1}};
        // clang-format on

    public:
        explicit basic_transport(rxComm& _comm)
            : transport<CharT>{}
            , comm_{&_comm}
            , fd_{uninitialized_file_descriptor}
        {
        }

        bool open(const irods::experimental::filesystem::path& _p,
                  std::ios_base::openmode _mode) override
        {
            return !is_open()
                ? open_impl(_p, _mode, [](auto&) {})
                : false;
        }

        bool open(const irods::experimental::filesystem::path& _p,
                  int _replica_number,
                  std::ios_base::openmode _mode) override
        {
            if (is_open()) {
                return false;
            }

            return open_impl(_p, _mode, [_replica_number](auto& _input) {
                const auto replica = std::to_string(_replica_number);
                addKeyVal(&_input.condInput, REPL_NUM_KW, replica.c_str());
            });
        }

        bool open(const irods::experimental::filesystem::path& _p,
                  const std::string& _resource_name,
                  std::ios_base::openmode _mode) override
        {
            if (is_open()) {
                return false;
            }

            return open_impl(_p, _mode, [&_resource_name](auto& _input) {
                addKeyVal(&_input.condInput, RESC_NAME_KW, _resource_name.c_str());
            });
        }

        bool close() override
        {
            openedDataObjInp_t input{};
            input.l1descInx = fd_;

            if (const auto ec = rxDataObjClose(comm_, &input); ec < 0) {
                return false;
            }

            fd_ = uninitialized_file_descriptor;

            return true;
        }

        std::streamsize receive(char_type* _buffer, std::streamsize _buffer_size) override
        {
            openedDataObjInp_t input{};

            input.l1descInx = fd_;
            input.len = _buffer_size;

            bytesBuf_t output{};

            output.len = input.len;
            output.buf = _buffer;

            return rxDataObjRead(comm_, &input, &output);
        }

        std::streamsize send(const char_type* _buffer, std::streamsize _buffer_size) override
        {
            openedDataObjInp_t input{};

            input.l1descInx = fd_;
            input.len = _buffer_size;

            bytesBuf_t input_buffer{};

            input_buffer.len = input.len;
            input_buffer.buf = const_cast<char_type*>(_buffer);

            return rxDataObjWrite(comm_, &input, &input_buffer);
        }

        pos_type seekpos(off_type _offset, std::ios_base::seekdir _dir) override
        {
            if (!is_open()) {
                return seek_error;
            }

            openedDataObjInp_t input{};

            input.l1descInx = fd_;
            input.offset = _offset;

            switch (_dir) {
                case std::ios_base::beg:
                    input.whence = SEEK_SET;
                    break;

                case std::ios_base::cur:
                    input.whence = SEEK_CUR;
                    break;

                case std::ios_base::end:
                    input.whence = SEEK_END;
                    break;

                default:
                    return seek_error;
            }

            fileLseekOut_t* output{};

            if (const auto ec = rxDataObjLseek(comm_, &input, &output); ec < 0) {
                return seek_error;
            }

            return output->offset;
        }

        bool is_open() const noexcept override
        {
            return fd_ >= minimum_valid_file_descriptor;
        }

        int file_descriptor() const noexcept override
        {
            return fd_;
        }

    private:
        int make_open_flags(std::ios_base::openmode _mode) noexcept
        {
            using std::ios_base;

            const auto m = _mode & ~(ios_base::ate | ios_base::binary);

            if (ios_base::in == m) {
                return O_RDONLY;
            }
            else if (ios_base::out == m || (ios_base::out | ios_base::trunc) == m) {
                return O_CREAT | O_WRONLY | O_TRUNC;
            }
            else if (ios_base::app == m || (ios_base::out | ios_base::app) == m) {
                return O_CREAT | O_WRONLY | O_APPEND;
            }
            else if ((ios_base::out | ios_base::in) == m) {
                return O_CREAT | O_RDWR;
            }
            else if ((ios_base::out | ios_base::in | ios_base::trunc) == m) {
                return O_CREAT | O_RDWR | O_TRUNC;
            }
            else if ((ios_base::out | ios_base::in | ios_base::app) == m ||
                     (ios_base::in | ios_base::app) == m)
            {
                return O_CREAT | O_RDWR | O_APPEND | O_TRUNC;
            }

            return translation_error;
        }

        bool seek_to_end_if_required(std::ios_base::openmode _mode)
        {
            if (std::ios_base::ate & _mode) {
                if (seek_error == seekpos(0, std::ios_base::end)) {
                    return false;
                }
            }

            return true;
        }

        template <typename Function>
        bool open_impl(const filesystem::path& _p, std::ios_base::openmode _mode, Function _func)
        {
            const auto flags = make_open_flags(_mode);

            if (flags == translation_error) {
                return false;
            }

            dataObjInp_t input{};

            input.createMode = 0600;
            input.openFlags = flags;
            rstrcpy(input.objPath, _p.c_str(), sizeof(input.objPath));

            _func(input);

            // TODO Modularize the block of code below.

            const auto fd = rxDataObjOpen(comm_, &input);

            if (fd < minimum_valid_file_descriptor) {
                return false;
            }

            fd_ = fd;

            if (!seek_to_end_if_required(_mode)) {
                close();
                return false;
            }

            return true;
        }

        rxComm* comm_;
        int fd_;
    }; // basic_transport

    using default_transport = basic_transport<char>;
} // irods::experimental::io::NAMESPACE_IMPL

#endif // IRODS_IO_DEFAULT_TRANSPORT_HPP
