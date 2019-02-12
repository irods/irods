#ifndef IRODS_DSTREAM_HPP
#define IRODS_DSTREAM_HPP

// clang-format off
#if defined(RODS_SERVER) || defined(RODS_CLERVER)
    #include "rsDataObjOpen.hpp"
    #include "rsDataObjRead.hpp"
    #include "rsDataObjWrite.hpp"
    #include "rsDataObjClose.hpp"
    #include "rsDataObjLseek.hpp"

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

    #define rxComm              rcComm_t

    #define rxDataObjOpen       rcDataObjOpen
    #define rxDataObjRead       rcDataObjRead
    #define rxDataObjWrite      rcDataObjWrite
    #define rxDataObjClose      rcDataObjClose
    #define rxDataObjLseek      rcDataObjLseek
#endif // defined(RODS_SERVER) || defined(RODS_CLERVER)
// clang-format on

#include "filesystem/path.hpp"

#include <streambuf>
#include <type_traits>
#include <array>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <utility>

namespace irods::experimental
{
    // Details about what each virtual function in this template are required to
    // do can be found at the following link:
    //
    //      https://en.cppreference.com/w/cpp/io/basic_streambuf
    //
    template <typename CharT,
              typename Traits = std::char_traits<CharT>>
    class basic_data_object_buf final
        : public std::basic_streambuf<CharT, Traits>
    {
    public:
        // clang-format off
        using char_type   = CharT;
        using traits_type = Traits;
        using int_type    = typename traits_type::int_type;
        using pos_type    = typename traits_type::pos_type;
        using off_type    = typename traits_type::off_type;
        // clang-format on

        static_assert(std::is_same_v<char_type, char>, R"(character type must be "char")");

    private:
        using base_type = std::basic_streambuf<CharT, Traits>;

        // clang-format off
        inline static constexpr auto uninitialized_file_descriptor = -1;
        inline static constexpr auto minimum_valid_file_descriptor = 3;
        inline static constexpr auto buffer_size                   = 4096;

        // Errors
        inline static constexpr auto translation_error             = -1;
        inline static constexpr auto external_write_error          = -1;
        inline static const     auto seek_error                    = pos_type{off_type{-1}};
        // clang-format on

    public:
        basic_data_object_buf()
            : base_type{}
            , buf_{}
            , comm_{}
            , fd_{uninitialized_file_descriptor}
        {
        }

        basic_data_object_buf(basic_data_object_buf&& _other)
            : basic_data_object_buf{}
        {
            swap(_other);
        }

        basic_data_object_buf& operator=(basic_data_object_buf&& _other)
        {
            close();
            swap(_other);
            return *this;
        }

        ~basic_data_object_buf()
        {
            close();
        }

        void swap(basic_data_object_buf& _other)
        {
            using std::swap;

            base_type::swap(_other);
            swap(buf_, _other.buf_);
            swap(comm_, _other.comm_);
            swap(fd_, _other.fd_);
        }

        friend void swap(basic_data_object_buf& _lhs, basic_data_object_buf& _rhs)
        {
            _lhs.swap(_rhs);
        }

        bool is_open() const noexcept
        {
            return fd_ >= minimum_valid_file_descriptor;
        }

        bool open(rxComm& _comm,
                  const filesystem::path& _p,
                  std::ios_base::openmode _mode)
        {
            if (is_open()) {
                return false;
            }

            comm_ = &_comm;

            init_get_or_put_area(_mode);

            return open_impl(_p, _mode, [](auto&) {});
        }

        bool open(rxComm& _comm,
                  const filesystem::path& _p,
                  int _replica_number,
                  std::ios_base::openmode _mode)
        {
            if (is_open()) {
                return false;
            }

            comm_ = &_comm;

            init_get_or_put_area(_mode);

            return open_impl(_p, _mode, [_replica_number](auto& _input) {
                const auto replica = std::to_string(_replica_number);
                addKeyVal(&_input.condInput, REPL_NUM_KW, replica.c_str());
            });
        }

        bool open(rxComm& _comm,
                  const filesystem::path& _p,
                  const std::string& _resource_name,
                  std::ios_base::openmode _mode)
        {
            if (is_open()) {
                return false;
            }

            comm_ = &_comm;

            init_get_or_put_area(_mode);

            return open_impl(_p, _mode, [&_resource_name](auto& _input) {
                addKeyVal(&_input.condInput, RESC_NAME_KW, _resource_name.c_str());
            });
        }

        bool close()
        {
            if (!is_open()) {
                return false;
            }

            this->sync();

            openedDataObjInp_t input{};
            input.l1descInx = fd_;

            if (const auto ec = rxDataObjClose(comm_, &input); ec < 0) {
                return false;
            }

            comm_ = nullptr;
            fd_ = uninitialized_file_descriptor;

            return true;
        }

        int file_descriptor() const noexcept
        {
            return fd_;
        }

    protected:
        int_type underflow() override
        {
            prepare_for_input();

            // If the "Get" area has not been fully consumed, then return the
            // current character.
            if (this->gptr() < this->egptr()) {
                return traits_type::to_int_type(*this->gptr());
            }

            // The "Get" area has been consumed. Fill the internal buffer with
            // new data from the data object.

            const auto bytes_read = read_bytes(buf_.data(), buf_.size());

            if (bytes_read <= 0) {
                return traits_type::eof();
            }

            auto* pbase = buf_.data();
            this->setg(pbase, pbase, pbase + bytes_read);

            return traits_type::to_int_type(*this->gptr());
        }

        int_type overflow(int_type _c = traits_type::eof()) override
        {
            prepare_for_output();

            if (flush_buffer() == external_write_error) {
                return traits_type::eof();
            }

            if (!traits_type::eq_int_type(_c, traits_type::eof())) {
                return this->sputc(_c);
            }

            return traits_type::not_eof(_c);
        }

        std::streamsize xsgetn(char_type* _buffer, std::streamsize _buffer_size) override
        {
            prepare_for_input();

            const auto bytes_to_copy = this->egptr() - this->gptr(); 

            // If there are bytes in the internal buffer that haven't been consumed,
            // then copy those bytes from the internal buffer into "_buffer".
            if (bytes_to_copy > 0) {
                std::memcpy(_buffer, this->gptr(), bytes_to_copy * sizeof(char_type));
                this->gbump(bytes_to_copy);
            }

            return read_bytes(_buffer + bytes_to_copy, _buffer_size - bytes_to_copy);
        }

        std::streamsize xsputn(const char_type* _buffer, std::streamsize _buffer_size) override
        {
            prepare_for_output();

            if (flush_buffer() == external_write_error) {
                return external_write_error;
            }

            return send_bytes(_buffer, _buffer_size * sizeof(char_type));
        }

        int sync() override
        {
            if (this->pptr()) {
                return flush_buffer();
            }

            return 0;
        }

        int_type pbackfail(int_type _c = traits_type::eof()) override
        {
            // If the "next" pointer of the "Get" area points to the beginning of the
            // "Get" area, then return immediately.
            if (this->gptr() == this->eback()) {
                return traits_type::eof();
            }

            // Decrement the "next" pointer.
            this->gbump(-1);

            // If the "next" pointer is not pointing to "EOF", then store "_c" at the location
            // pointed at by the "next" pointer.
            if (!traits_type::eq_int_type(_c, traits_type::eof())) {
                *this->gptr() = traits_type::to_char_type(_c);
            }

            return traits_type::not_eof(_c);
        }

        pos_type seekoff(off_type _off,
                         std::ios_base::seekdir _dir,
                         std::ios_base::openmode _which = std::ios_base::in | std::ios_base::out) override
        {
            if (!is_open() || this->sync() != 0) {
                return seek_error;
            }

            openedDataObjInp_t input{};

            input.l1descInx = fd_;
            input.offset = _off;

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

            return pos_type{output->offset};
        }

        pos_type seekpos(pos_type _pos, std::ios_base::openmode _which = std::ios_base::in | std::ios_base::out) override
        {
            if (!is_open() || this->sync() != 0) {
                return seek_error;
            }

            openedDataObjInp_t input{};

            input.l1descInx = fd_;
            input.offset = _pos;
            input.whence = SEEK_SET;

            fileLseekOut_t* output{};

            if (const auto ec = rxDataObjLseek(comm_, &input, &output); ec < 0) {
                return seek_error;
            }

            return pos_type{output->offset};
        }

    private:
        void prepare_for_input()
        {
            // Return if the pointers of "Get" area have already been set up.
            if (this->gptr()) {
                return;
            }

            // Flush and clear the contents of the "Put" area.
            this->sync();
            this->setp(nullptr, nullptr);

            // Setup the "Get" area.
            auto* pbase = buf_.data();
            this->setg(pbase, pbase, pbase);
        }

        void prepare_for_output()
        {
            // Return if the pointers of "Put" area have already been set up.
            if (this->pptr()) {
                return;
            }

            // Clear the contents of the "Get" area.
            this->setg(nullptr, nullptr, nullptr);

            // Setup the "Put" area.
            auto* pbase = buf_.data();
            this->setp(pbase, pbase + buf_.size());
        }

        void init_get_or_put_area(std::ios_base::openmode _mode) noexcept
        {
            using std::ios_base;

            const auto m = ios_base::in | ios_base::out;

            if ((_mode & m) == m || _mode & ios_base::in) {
                prepare_for_input();
            }
            else if (_mode & ios_base::out) {
                prepare_for_output();
            }
        }

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
                if (seek_error == this->seekoff(0, std::ios_base::end)) {
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

        int flush_buffer()
        {
            const auto bytes_to_send = this->pptr() - this->pbase();

            if (bytes_to_send == 0) {
                return 0;
            }

            const auto bytes_written = send_bytes(buf_.data(), bytes_to_send);

            if (bytes_written < 0) {
                return external_write_error;
            }

            this->pbump(-bytes_written);

            return 0;
        }

        int read_bytes(char_type* _buffer, int _buffer_size) const noexcept
        {
            openedDataObjInp_t input{};

            input.l1descInx = fd_;
            input.len = _buffer_size;

            bytesBuf_t output{};

            output.len = input.len;
            output.buf = _buffer;

            return rxDataObjRead(comm_, &input, &output);
        }

        int send_bytes(const char_type* _buffer, int _buffer_size) const noexcept
        {
            openedDataObjInp_t input{};

            input.l1descInx = fd_;
            input.len = _buffer_size;

            bytesBuf_t input_buffer{};

            input_buffer.len = input.len;
            input_buffer.buf = const_cast<char_type*>(_buffer);

            return rxDataObjWrite(comm_, &input, &input_buffer);
        }

        std::array<char_type, buffer_size> buf_;
        rxComm* comm_;
        int fd_;
    }; // basic_data_object_buf

    // Provides a default openmode for basic_dstream constructors and open()
    // member functions based on the type "T". "T" must be one of the general
    // stream types defined by the C++ standard (i.e. basic_istream, basic_ostream,
    // or basic_iostream).
    template <typename>
    constexpr std::ios_base::openmode default_openmode{};

    template <typename T>
    constexpr auto default_openmode<std::basic_istream<T>> = std::ios_base::in;

    template <typename T>
    constexpr auto default_openmode<std::basic_ostream<T>> = std::ios_base::out;

    template <typename T>
    constexpr auto default_openmode<std::basic_iostream<T>> = std::ios_base::in | std::ios_base::out;

    // A concrete stream class template that wraps a basic_data_object_buf object.
    // The general stream used to instantiate this type must use "char" for the underlying
    // character type. Using wchar_t or anything else to instantiate this template is undefined.
    // This class is modeled after the C++ standard file stream classes.
    template <typename GeneralStream>
    class basic_dstream final
        : public GeneralStream
    {
    private:
        template <typename>
        inline static constexpr std::ios_base::openmode mandatory_openmode{};

        template <typename T>
        inline static constexpr auto mandatory_openmode<std::basic_istream<T>> = std::ios_base::in;

        template <typename T>
        inline static constexpr auto mandatory_openmode<std::basic_ostream<T>> = std::ios_base::out;

    public:
        // clang-format off
        using char_type   = typename GeneralStream::char_type;
        using traits_type = typename GeneralStream::traits_type;
        using int_type    = typename traits_type::int_type;
        using pos_type    = typename traits_type::pos_type;
        using off_type    = typename traits_type::off_type;
        // clang-format on

        static_assert(std::is_same_v<char_type, char>, R"(character type must be "char")");

        basic_dstream()
            : GeneralStream{&buf_}
            , buf_{}
        {
        }

        basic_dstream(rxComm& _comm,
                           const filesystem::path& _p,
                           std::ios_base::openmode _mode = default_openmode<GeneralStream>)
            : basic_dstream{}
        {
            open(_comm, _p, _mode);
        }

        basic_dstream(rxComm& _comm,
                           const filesystem::path& _p,
                           int _replica_number,
                           std::ios_base::openmode _mode = default_openmode<GeneralStream>)
            : basic_dstream{}
        {
            open(_comm, _p, _replica_number, _mode);
        }

        basic_dstream(rxComm& _comm,
                           const filesystem::path& _p,
                           const std::string& _resource_name,
                           std::ios_base::openmode _mode = default_openmode<GeneralStream>)
            : basic_dstream{}
        {
            open(_comm, _p, _resource_name, _mode);
        }

        basic_dstream(basic_dstream&&) = default;
        basic_dstream& operator=(basic_dstream&&) = default;

        ~basic_dstream() = default;

        void swap(basic_dstream& _other)
        {
            GeneralStream::swap(_other);
            buf_.swap(_other.buf_);
        }

        friend void swap(basic_dstream& _lhs, basic_dstream& _rhs)
        {
            _lhs.swap(_rhs);
        }

        basic_data_object_buf<char_type, traits_type>* rdbuf() const noexcept
        {
            return &buf_;
        }

        bool is_open() const noexcept
        {
            return buf_.is_open();
        }

        void open(rxComm& _comm,
                  const filesystem::path& _p,
                  std::ios_base::openmode _mode = default_openmode<GeneralStream>)
        {
            if (!buf_.open(_comm, _p, _mode | mandatory_openmode<GeneralStream>)) {
                this->setstate(std::ios_base::failbit);
            }
            else {
                this->clear();
            }
        }

        void open(rxComm& _comm,
                  const filesystem::path& _p,
                  int _replica_number,
                  std::ios_base::openmode _mode = default_openmode<GeneralStream>)
        {
            if (!buf_.open(_comm, _p, _replica_number, _mode | mandatory_openmode<GeneralStream>)) {
                this->setstate(std::ios_base::failbit);
            }
            else {
                this->clear();
            }
        }

        void open(rxComm& _comm,
                  const filesystem::path& _p,
                  const std::string& _resource_name,
                  std::ios_base::openmode _mode = default_openmode<GeneralStream>)
        {
            if (!buf_.open(_comm, _p, _resource_name, _mode | mandatory_openmode<GeneralStream>)) {
                this->setstate(std::ios_base::failbit);
            }
            else {
                this->clear();
            }
        }

        void close()
        {
            if (!buf_.close()) {
                this->setstate(std::ios_base::failbit);
            }
            else {
                this->clear();
            }
        }

        int file_descriptor() const noexcept
        {
            return buf_.file_descriptor();
        }

    private:
        basic_data_object_buf<char_type, traits_type> buf_;
    }; // basic_dstream

    // clang-format off
    //
    // The concrete stream types.
    // For details on how to use these types, refer to the following links:
    //
    //    https://en.cppreference.com/w/cpp/io/basic_fstream
    //    https://en.cppreference.com/w/cpp/io/basic_ifstream
    //    https://en.cppreference.com/w/cpp/io/basic_ofstream
    //
    using data_object_buf = basic_data_object_buf<char>;
    using idstream        = basic_dstream<std::basic_istream<char>>;
    using odstream        = basic_dstream<std::basic_ostream<char>>;
    using dstream         = basic_dstream<std::basic_iostream<char>>;
    // clang-format on
} // namespace irods::experimental

#endif // IRODS_DSTREAM_HPP

