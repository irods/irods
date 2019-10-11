#ifndef IRODS_IO_DSTREAM_HPP
#define IRODS_IO_DSTREAM_HPP

#include "filesystem/path.hpp"
#include "transport/transport.hpp"

#include <streambuf>
#include <type_traits>
#include <array>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <utility>

namespace irods::experimental::io
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
        inline static constexpr auto buffer_size          = 4096;

        // Errors
        inline static constexpr auto external_write_error = -1;
        inline static const     auto seek_error           = pos_type{off_type{-1}};
        // clang-format on

    public:
        basic_data_object_buf()
            : base_type{}
            , buf_{}
            , transport_{}
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
            swap(transport_, _other.transport_);
            swap(buf_, _other.buf_);
        }

        friend void swap(basic_data_object_buf& _lhs, basic_data_object_buf& _rhs)
        {
            _lhs.swap(_rhs);
        }

        bool is_open() const noexcept
        {
            return transport_ && transport_->is_open();
        }
	
        basic_data_object_buf* open(transport<char_type>& _transport,
                                    const filesystem::path& _p,
                                    std::ios_base::openmode _mode)
        {
            transport_ = &_transport;

            if (!transport_->open(_p, _mode)) {
                return nullptr;
            }

            init_get_or_put_area(_mode);

            return this;
        }

        basic_data_object_buf* open(transport<char_type>& _transport,
                                    const filesystem::path& _p,
                                    int _replica_number,
                                    std::ios_base::openmode _mode)
        {
            transport_ = &_transport;

            if (!transport_->open(_p, _replica_number, _mode)) {
                return nullptr;
            }

            init_get_or_put_area(_mode);

            return this;
        }

        basic_data_object_buf* open(transport<char_type>& _transport,
                                    const filesystem::path& _p,
                                    const std::string& _resource_name,
                                    std::ios_base::openmode _mode)
        {
            transport_ = &_transport;

            if (!transport_->open(_p, _resource_name, _mode)) {
                return nullptr;
            }

            init_get_or_put_area(_mode);

            return this;
        }

        basic_data_object_buf* close()
        {
            if (!transport_ || !transport_->is_open()) {
                return nullptr;
            }

            auto* sb = this;

            if (this->sync()) {
                sb = nullptr;
            }

            if (!transport_->close()) {
                sb = nullptr;
            }

            return sb;
        }

        int file_descriptor() const noexcept
        {
            return transport_->file_descriptor();;
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

            const auto bytes_read = transport_->receive(buf_.data(), buf_.size() * sizeof(char_type));

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

            return transport_->receive(_buffer + bytes_to_copy, (_buffer_size - bytes_to_copy) * sizeof(char_type));
        }

        std::streamsize xsputn(const char_type* _buffer, std::streamsize _buffer_size) override
        {
            prepare_for_output();

            if (flush_buffer() == external_write_error) {
                return external_write_error;
            }

            return transport_->send(_buffer, _buffer_size * sizeof(char_type));
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
            if (this->sync() != 0) {
                return seek_error;
            }

            return transport_->seekpos(_off, _dir);
        }

        pos_type seekpos(pos_type _pos, std::ios_base::openmode _which = std::ios_base::in | std::ios_base::out) override
        {
            if (this->sync() != 0) {
                return seek_error;
            }

            return transport_->seekpos(_pos, std::ios_base::beg);
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

        int flush_buffer()
        {
            const auto bytes_to_send = this->pptr() - this->pbase();

            if (bytes_to_send == 0) {
                return 0;
            }

            const auto bytes_written = transport_->send(buf_.data(), bytes_to_send * sizeof(char_type));

            if (bytes_written < 0) {
                return external_write_error;
            }

            this->pbump(-bytes_written);

            return 0;
        }

        std::array<char_type, buffer_size> buf_;
        transport<char_type>* transport_;
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

        basic_dstream(transport<char_type>& _transport,
                      const filesystem::path& _p,
                      std::ios_base::openmode _mode = default_openmode<GeneralStream>)
            : basic_dstream{}
        {
            open(_transport, _p, _mode);
        }

        basic_dstream(transport<char_type>& _transport,
                      const filesystem::path& _p,
                      int _replica_number,
                      std::ios_base::openmode _mode = default_openmode<GeneralStream>)
            : basic_dstream{}
        {
            open(_transport, _p, _replica_number, _mode);
        }

        basic_dstream(transport<char_type>& _transport,
                      const filesystem::path& _p,
                      const std::string& _resource_name,
                      std::ios_base::openmode _mode = default_openmode<GeneralStream>)
            : basic_dstream{}
        {
            open(_transport, _p, _resource_name, _mode);
        }

        basic_dstream(basic_dstream&& _other)
            : GeneralStream{std::move(_other)}
            , buf_{std::move(_other.buf_)}
        {
            this->set_rdbuf(&buf_);
        }

        basic_dstream& operator=(basic_dstream&& _other)
        {
            GeneralStream::operator=(std::move(_other));
            buf_ = std::move(_other.buf_);
            return *this;
        }

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

        basic_data_object_buf<char_type, traits_type>* rdbuf() const
        {
            return const_cast<basic_data_object_buf<char_type, traits_type>*>(&buf_);
        }

        bool is_open() const noexcept
        {
            return buf_.is_open();
        }

        void open(transport<char_type>& _transport,
                  const filesystem::path& _p,
                  std::ios_base::openmode _mode = default_openmode<GeneralStream>)
        {
            if (!buf_.open(_transport, _p, _mode | mandatory_openmode<GeneralStream>)) {
                this->setstate(std::ios_base::failbit);
            }
            else {
                this->clear();
            }
        }

        void open(transport<char_type>& _transport,
                  const filesystem::path& _p,
                  int _replica_number,
                  std::ios_base::openmode _mode = default_openmode<GeneralStream>)
        {
            if (!buf_.open(_transport, _p, _replica_number, _mode | mandatory_openmode<GeneralStream>)) {
                this->setstate(std::ios_base::failbit);
            }
            else {
                this->clear();
            }
        }

        void open(transport<char_type>& _transport,
                  const filesystem::path& _p,
                  const std::string& _resource_name,
                  std::ios_base::openmode _mode = default_openmode<GeneralStream>)
        {
            if (!buf_.open(_transport, _p, _resource_name, _mode | mandatory_openmode<GeneralStream>)) {
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
} // namespace irods::experimental::io

#endif // IRODS_IO_DSTREAM_HPP

