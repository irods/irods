#ifndef IRODS_EXCEPTION_HPP
#define IRODS_EXCEPTION_HPP

#include <exception>
#include <string>
#include <sstream>
#include <vector>
#include <inttypes.h>

#include <irods_stacktrace.hpp>

namespace irods {

    class exception : public std::exception {
        public:
            exception(
                const int64_t     _code,
                const std::string& _message,
                const std::string& _file_name,
                const uint32_t     _line_number,
                const std::string& _function_name );
            exception( const exception& );
            virtual ~exception() throw();

            virtual const char* what() const throw();

            // accessors
            int64_t    code() const { return code_; }
            std::vector< std::string > message_stack() const { return message_stack_; }
            std::string file_name() const { return file_name_; }
            uint32_t    line_number() const { return line_number_; }
            std::string function_name() const { return function_name_; }
            irods::stacktrace stacktrace() const { return stacktrace_; }

            // mutators
            void add_message( const std::string& _m ) { message_stack_.push_back( _m ); }

        private:
            int64_t                   code_;
            std::vector< std::string > message_stack_;
            uint32_t                   line_number_;
            std::string                function_name_;
            std::string                file_name_;
            irods::stacktrace          stacktrace_;
            mutable std::string        what_;

    }; // class exception


}; // namespace irods

#define THROW( _code, _msg ) ( throw irods::exception( _code, _msg, __FILE__, __LINE__, __FUNCTION__ ) )
#define RE_THROW( _msg, _excep ) ( _excp.add_message( _msg ); throw;

#endif // IRODS_EXCEPTION_HPP
