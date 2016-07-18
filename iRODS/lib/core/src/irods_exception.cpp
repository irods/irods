#include "irods_exception.hpp"
#include "irods_stacktrace.hpp"

namespace irods {

    exception::exception(
        const int64_t     _code,
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

    const char* exception::what() const throw() {
        std::string message;
        for ( size_t i = 0;
                i < message_stack_.size();
                ++i ) {
            message += "        ";
            message += message_stack_[i];
            message += "\n";

        }

        std::stringstream what_ss;
        what_ss << "=-=-=-=-=-=-=-" << std::endl
                << "iRODS Exception:" << std::endl
                << "    code: " << code_ << std::endl
                << "    message:" << std::endl
                << message
                << "stack trace:" << std::endl
                << "--------------" << std::endl
                << stacktrace_.dump()
                << "=-=-=-=-=-=-=-" << std::endl;

        what_ = what_ss.str();

        return what_.c_str();

    } // what

}; // namespace irods
