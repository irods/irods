



// =-=-=-=-=-=-=-
// My Includes
#include "eirods_error.h"

// =-=-=-=-=-=-=-
// Boost Includes
#include <boost/lexical_cast.hpp>
#include <iostream>

namespace eirods {

    // =-=-=-=-=-=-=-
	// public - default constructor
    error::error( ) : status_(false), code_(0), message_("") { 
	} // ctor

    // =-=-=-=-=-=-=-
	// public - useful constructor
    error::error( bool _status, int _code, std::string _msg, int _line, 
	               std::string _file ) : status_(_status), code_(_code),
				                         message_(_msg) { 
		// =-=-=-=-=-=-=-
		// decorate message based on status
		std::string result;
        if( status_ ) {
			result = "[+]\t";
		} else {
			result = "[-]\t";
		}

		// =-=-=-=-=-=-=-
		// compose resulting message given all components
		result += _file 
		       + ":" 
		       + boost::lexical_cast<std::string>( _line ) 
		       + " -- message [" + message_ + "]  "  
               + " status  (" + boost::lexical_cast<std::string>( code_ ) + ") ";
	
		// =-=-=-=-=-=-=-
		// cache message on message stack
        result_stack_.push_back( result ); 

	} // ctor

    // =-=-=-=-=-=-=-
	// public - useful constructor
    error::error( bool _status, int _code, std::string _msg, int _line, 
	              std::string _file, const error& _rhs ) : 
				       status_(_status), code_(_code), message_(_msg) { 
		// =-=-=-=-=-=-=-
		// cache RHS vector into our vector first
        result_stack_ = _rhs.result_stack_;

		// =-=-=-=-=-=-=-
		// decorate message based on status
		std::string result;
        if( status_ ) {
			result = "[+]\t";
		} else {
			result = "[-]\t";
		}

		// =-=-=-=-=-=-=-
		// compose resulting message given all components
		result += _file 
		       + ":" 
		       + boost::lexical_cast<std::string>( _line ) 
		       + " -- message [" + message_ + "]  "  
               + " status  (" + boost::lexical_cast<std::string>( code_ ) + ") ";
	
		// =-=-=-=-=-=-=-
		// cache message on message stack
        result_stack_.push_back( result ); 

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

	} // assignment operator

    // =-=-=-=-=-=-=-
	// public - return the status of this error object
	int error::status(  ) {
		return status_;

	} // status

    // =-=-=-=-=-=-=-
	// public - return the code of this error object
	int error::code(  ) {
		return code_;

	} // code

    // =-=-=-=-=-=-=-
	// public - return the composite result for logging, etc.
	std::string error::result(  ) {
	
        // =-=-=-=-=-=-=-
		// tack on tabs based on stack depth
		for( int i = 0; i < result_stack_.size(); ++i ) {
			
			std::string tabs = "";
		    for( int j = i+1; j < result_stack_.size(); ++j ) {
				tabs += "\t";

			} // for j
			    
			result_stack_[i] = tabs + result_stack_[i]; 

		} // for i
		
        // =-=-=-=-=-=-=-
		// tack on newlines
		for( int i = 0; i < result_stack_.size(); ++i ) {
			result_stack_[i] = "\n" + result_stack_[i]; 
		
		} // for i

		// =-=-=-=-=-=-=-
		// compose single string of the result stack for print out
		std::string result;
		for( int i = result_stack_.size()-1; i >= 0; --i ) {
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



