// =-=-=-=-=-=-=-
#include "irods_error.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "rodsLog.h"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/lexical_cast.hpp>

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>

namespace irods {
// =-=-=-=-=-=-=-
// private - helper fcn to build the result string
    std::string error::build_result_string(
        std::string _file,
        int         _line,
        std::string _fcn ) {
        // =-=-=-=-=-=-=-
        // decorate message based on status
        std::string result;
        if ( status_ ) {
            result = "[+]\t";
        }
        else {
            result = "[-]\t";
        }

        // =-=-=-=-=-=-=-
        // only keep the file name the path back to iRODS
        std::string line_info;
        try { //replace with std::to_string when we have c++14
            line_info = _file + ":" + boost::lexical_cast<std::string>( _line ) + ":" + _fcn;
        }
        catch ( const boost::bad_lexical_cast& ) {
            line_info = _file + ":<unknown line number>:" + _fcn;
        }

        size_t pos = line_info.find( "iRODS" );
        if ( std::string::npos != pos ) {
            line_info = line_info.substr( pos );
        }

        // =-=-=-=-=-=-=-
        // get the rods error and errno string
        char* errno_str = 0;
        char* irods_err = rodsErrorName( code_, &errno_str );

        // =-=-=-=-=-=-=-
        // compose resulting message given all components
        result += line_info + " : " +
                  + " status [" + irods_err + "]  errno [" + errno_str + "]"
                  + " -- message [" + message_ + "]";

        return result;

    } // build_result_string

// =-=-=-=-=-=-=-
// public - default constructor
    error::error( ) : status_( false ), code_( 0 ), message_( "" ) {
    } // ctor

// =-=-=-=-=-=-=-
// public - useful constructor
    error::error(
        bool        _status,
        long long   _code,
        std::string _msg,
        std::string _file,
        int         _line,
        std::string _fcn ) :
        status_( _status ),
        code_( _code ),
        message_( _msg ) {

        // =-=-=-=-=-=-=-
        // cache message on message stack
        if ( !_msg.empty() ) {
            result_stack_.push_back( build_result_string( _file, _line, _fcn ) );
        }

    } // ctor

// =-=-=-=-=-=-=-
// public - deprecated constructor - since 4.0.3
    error::error(
        bool         ,
        long long    ,
        std::string  _msg,
        std::string  _file,
        int          _line,
        std::string  _fcn,
        const error& _rhs ) :
        status_( _rhs.status() ),
        code_( _rhs.code() ),
        message_( _msg ) {
        // =-=-=-=-=-=-=-
        // cache RHS vector into our vector first
        result_stack_ = _rhs.result_stack_;

        // =-=-=-=-=-=-=-
        // cache message on message stack
        result_stack_.push_back( build_result_string( _file, _line, _fcn ) );

    } // ctor

// =-=-=-=-=-=-=-
// public - useful constructor
    error::error(
        std::string  _msg,
        std::string  _file,
        int          _line,
        std::string  _fcn,
        const error& _rhs ) :
        status_( _rhs.status() ),
        code_( _rhs.code() ),
        message_( _msg ) {
        // =-=-=-=-=-=-=-
        // cache RHS vector into our vector first
        result_stack_ = _rhs.result_stack_;

        // =-=-=-=-=-=-=-
        // cache message on message stack
        result_stack_.push_back( build_result_string( _file, _line, _fcn ) );

    } // ctor

// =-=-=-=-=-=-=-
// public - copy constructor
    error::error( const error& _rhs ) {
        status_           = _rhs.status_;
        code_             = _rhs.code_;
        message_          = _rhs.message_;
        result_stack_     = _rhs.result_stack_;
    } // cctor

// =-=-=-=-=-=-=-
// public - Destructor
    error::~error() {
    } // dtor

// =-=-=-=-=-=-=-
// public - Assignment Operator
    error& error::operator=( const error& _rhs ) {
        status_           = _rhs.status_;
        code_             = _rhs.code_;
        message_          = _rhs.message_;
        result_stack_     = _rhs.result_stack_;
        return *this;
    } // assignment operator

// =-=-=-=-=-=-=-
// public - return the status of this error object
    bool error::status( ) const {
        return status_;

    } // status

// =-=-=-=-=-=-=-
// public - return the code of this error object
    long long error::code( ) const {
        return code_;

    } // code

// =-=-=-=-=-=-=-
// public - return the composite result for logging, etc.
    std::string error::result() const {

        // compose single string of the result stack for print out
        std::string result;
        std::string tabs = "";
        for ( size_t i = 0; i < result_stack_.size(); ++i ) {
            if ( i != 0 ) {
                result += "\n";
            }
            result += tabs;
            result += result_stack_[ result_stack_.size() - 1 - i ];
            tabs += "\t";

        } // for i

        // add extra newline for formatting
        result += "\n\n";
        return result;

    } // result

// =-=-=-=-=-=-=-
// public - return the status_ - deprecated in 4.0.3
    bool error::ok() {
        return status_;
    } // ok

// =-=-=-=-=-=-=-
// public - return the status_
    bool error::ok() const {
        return status_;
    } // ok


    error assert_error(
        bool expr_,
        long long code_,
        const std::string& file_,
        const std::string& function_,
        const std::string& format_,
        int line_,
        ... ) {
        error result = SUCCESS();
        if ( !expr_ ) {
            va_list ap;
            va_start( ap, line_ );
            const int buffer_size = 4096;
            char buffer[buffer_size];
            vsnprintf( buffer, buffer_size, format_.c_str(), ap );
            va_end( ap );
            std::stringstream msg;
            msg << buffer;
            result = error( false, code_, msg.str(), file_, line_, function_ );
        }
        return result;
    }

    error assert_pass(
        const error& _prev_error,
        const std::string& _file,
        const std::string& _function,
        const std::string& _format,
        int _line,
        ... ) {
        std::stringstream msg;
        if ( !_prev_error.ok() ) {
            va_list ap;
            va_start( ap, _line );
            const int buffer_size = 4096;
            char buffer[buffer_size];
            vsnprintf( buffer, buffer_size, _format.c_str(), ap );
            va_end( ap );
            msg << buffer;
        }
        return error( msg.str(), _file, _line, _function, _prev_error );
    }

}; // namespace irods



