#include "irods_exception.hpp"
#include "irods_stacktrace.hpp"
#include "rodsLog.h"

namespace irods {

    exception::exception(
        const int64_t      _code,
        const std::string& _message,
        const std::string& _file_name,
        const uint32_t     _line_number,
        const std::string& _function_name ) :
        std::exception(),
        code_( _code ),
        line_number_( _line_number ),
        stacktrace_() {
        message_stack_.push_back( _message );
        file_name_     = _file_name;
        function_name_ = _function_name;

    } // exception

    exception::exception(
        const int64_t        _code,
        const boost::format& _message,
        const std::string&   _file_name,
        const uint32_t       _line_number,
        const std::string&   _function_name ) :
        exception::exception(_code, boost::str(_message), _file_name, _line_number, _function_name) {}

    exception::exception( const exception& _rhs ) :
        std::exception( _rhs ),
        code_( _rhs.code_ ),
        message_stack_( _rhs.message_stack_ ),
        line_number_( _rhs.line_number_ ),
        function_name_( _rhs.function_name_ ),
        file_name_( _rhs.file_name_ ),
        stacktrace_( _rhs.stacktrace_ ) {
    }

    exception::~exception() throw() {

    } // ~exception

    // This "should not" be called from the icommands
    const char* exception::what() const throw()
    {
           assemble_full_display_what();
        return what_.c_str();
    }

    const char* exception::client_display_what() const throw()
    {
        assemble_client_display_what();
        return what_.c_str();
    }

    // Modifies the what_ string for a full what() message
    void exception::assemble_full_display_what() const throw()
    {
        std::stringstream what_ss;

        what_ss << "iRODS Exception:"
                << "\n    file: " << file_name_
                << "\n    function: " << function_name_
                << "\n    line: " << line_number_
                << "\n    code: " << code_ << " (" << rodsErrorName( static_cast<int>(code_), NULL ) << ")"
                << "\n    message:" << "\n";

        for ( auto entry: message_stack_)
        {
            what_ss << "        " << entry << "\n";
        }

        what_ss << "stack trace:" << "\n"
                << "--------------" << "\n"
                << stacktrace_.dump() << std::endl;

        what_ = what_ss.str();
    }

    // Modifies the what_ string for a client display suitable what() message
    void exception::assemble_client_display_what() const throw()
    {
        std::stringstream what_ss;

        what_ss << rodsErrorName( static_cast<int>(code_), NULL ) << ": ";

        for ( auto entry: message_stack_)
        {
            what_ss << entry << "\n";
        }
        what_ss << std::endl;
        what_ = what_ss.str();
    }

}; // namespace irods
