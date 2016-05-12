#include "irods_exception.hpp"
#include "irods_stacktrace.hpp"

namespace irods {

    exception::exception(
        const int64_t      _code,
        const std::string& _message,
        const std::string& _file_name,
        const uint32_t     _line_number,
        const std::string& _function_name ) :
        std::exception(),
        code_( _code ),
        line_number_( _line_number ) {
        message_stack_.push_back( _message );
        file_name_     = _file_name;
        function_name_ = _function_name;

        std::stringstream ss;
        stacktrace st;
        st.trace();
        st.dump( ss );
        stack_trace_ = ss.str();

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
        stack_trace_( _rhs.stack_trace_ ) {
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
        what_ss << "iRODS Exception:" << std::endl
                << "    file: " << file_name_ << std::endl
                << "    function: " << function_name_ << std::endl
                << "    line: " << line_number_ << std::endl
                << "    code: " << code_ << std::endl
                << "    message:" << std::endl
                << message
                << "stack trace:" << std::endl
                << "--------------" << std::endl
                << stack_trace_ << std::endl;

        what_ = what_ss.str();

        return what_.c_str();

    } // what

}; // namespace irods
