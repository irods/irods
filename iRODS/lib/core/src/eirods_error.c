/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_error.h"

// =-=-=-=-=-=-=-
// irods includes
#include "rodsLog.h"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/lexical_cast.hpp>

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>

namespace eirods {
    // =-=-=-=-=-=-=-
    // private - helper fcn to build the result string
    std::string error::build_result_string( 
        std::string _file,
        int         _line,
        std::string _fcn ) {
        // =-=-=-=-=-=-=-
        // decorate message based on status
        std::string result;
        if( status_ ) {
            result = "[+]\t";
        } else {
            result = "[-]\t";
        }

        // =-=-=-=-=-=-=-
        // only keep the file name the path back to iRODS
        std::string line_info = _file + ":" + boost::lexical_cast<std::string>( _line ) + ":" + _fcn;
        size_t pos = line_info.find( "iRODS" );
        if( std::string::npos != pos ) {
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
    error::error( ) : status_(false), code_(0), message_("") { 
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
        status_(_status), 
        code_(_code),
        message_(_msg) { 

        // =-=-=-=-=-=-=-
        // cache message on message stack
        result_stack_.push_back( build_result_string( _file, _line, _fcn ) ); 

    } // ctor

    // =-=-=-=-=-=-=-
    // public - useful constructor
    error::error( 
        bool         _status, 
        long long    _code, 
        std::string  _msg, 
        std::string  _file,
        int          _line,
        std::string  _fcn,
        const error& _rhs ) : 
        status_(_status), 
        code_(_code), 
        message_(_msg) { 
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
    bool error::status(  ) {
        return status_;

    } // status

    // =-=-=-=-=-=-=-
    // public - return the code of this error object
    long long error::code(  ) {
        return code_;

    } // code

    // =-=-=-=-=-=-=-
    // public - return the composite result for logging, etc.
    std::string error::result(  ) {
        // =-=-=-=-=-=-=-
        // tack on tabs based on stack depth
        for( size_t i = 0; i < result_stack_.size(); ++i ) {
			
            std::string tabs = "";
            for( size_t j = i+1; j < result_stack_.size(); ++j ) {
                tabs += "\t";

            } // for j
			    
            result_stack_[i] = tabs + result_stack_[i]; 

        } // for i
		
        // =-=-=-=-=-=-=-
        // tack on newlines
        for( size_t i = 0; i < result_stack_.size(); ++i ) {
            result_stack_[i] = "\n" + result_stack_[i]; 
		
        } // for i

        // =-=-=-=-=-=-=-
        // compose single string of the result stack for print out
        std::string result;
        for( int i = static_cast<int>( result_stack_.size() )-1; i >= 0; --i ) {
            result += result_stack_[ i ];
		
        } // for i

        // =-=-=-=-=-=-=-
        // add extra newline for formatting
        result += "\n\n";

        return result;

    } // result

    // =-=-=-=-=-=-=-
    // public - return the status_
    bool error::ok(  ) {
        return status_;

    } // ok

}; // namespace eirods



