#ifndef IRODS_EXCEPTION_HPP
#define IRODS_EXCEPTION_HPP

#include <exception>
#include <string>
#include <sstream>
#include <vector>
#include <inttypes.h>
#include <boost/format.hpp>

#include <irods_stacktrace.hpp>

namespace irods {

    class exception : public std::exception {
        public:
            exception(
                const int64_t      _code,
                const std::string& _message,
                const std::string& _file_name,
                const uint32_t     _line_number,
                const std::string& _function_name );
            exception(
                const int64_t        _code,
                const boost::format& _message,
                const std::string&   _file_name,
                const uint32_t       _line_number,
                const std::string&   _function_name );
            exception( const exception& );
            virtual ~exception() throw();

            // This is used internally, not in the icommands
            virtual const char* what() const throw();

            // This should be used by icommands, instead of what()
            virtual const char* client_display_what() const throw();

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
            // Assemble the what_ string depending on what kind of what() was called...
            void assemble_full_display_what() const throw();
            void assemble_client_display_what() const throw();

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

#define THROW( _code, _msg ) ( throw irods::exception( _code, _msg, __FILE__, __LINE__, __PRETTY_FUNCTION__ ) )
#define RE_THROW( _msg, _excp ) _excp.add_message( _msg ); throw _excp;
#define CATCH_EXC_AND_RETURN( throwing_expression ) try { throwing_expression; } catch (const irods::exception& e) { rodsLog( LOG_ERROR, e.code(), "%s encountered an exception on line %d in file %s:\n%s", __PRETTY_FUNCTION__, __LINE__, __FILE__, e.what()); return e.code() }
#define CATCH_EXC_AND_LOG( throwing_expression ) try { throwing_expression; } catch (const irods::exception& e) { rodsLog( LOG_ERROR, e.code(), "%s encountered an exception on line %d in file %s:\n%s", __PRETTY_FUNCTION__, __LINE__, __FILE__, e.what()); }

#endif // IRODS_EXCEPTION_HPP
