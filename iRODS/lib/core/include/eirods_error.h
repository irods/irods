/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef __EIRODS_ERROR_H__
#define __EIRODS_ERROR_H__

// =-=-=-=-=-=-=-
// STL Includes
#include <string>
#include <sstream>
#include <vector>

namespace eirods {

    class error {
    public:
        // =-=-=-=-=-=-=-
        // Constructors
        // status, code, message, line number, file
        error();
        error( bool,          // status
               long long,     // code
               std::string,   // message
               std::string,   // file
               int,           // line number
               std::string ); // function 
        error( bool,          // status
               long long,     // code
               std::string,   // message
               std::string,   // file
               int,           // line
               std::string,   // function
               const error& );// prev error
        error( const error& );   

        // =-=-=-=-=-=-=-
        // Destructor
        ~error();
		 
        // =-=-=-=-=-=-=-
        // Operators
        error& operator=( const error& );
		
        // =-=-=-=-=-=-=-
        // Members
        bool        status();
        long long   code();
        std::string result();
        bool        ok();

    private:
        // =-=-=-=-=-=-=-
        // Attributes
        bool        status_;
        long long   code_;
        std::string message_;
        std::vector< std::string > result_stack_;
        
        // =-=-=-=-=-=-=-
        // Members
        std::string build_result_string( std::string, int, std::string );
		
    }; // class error

}; // namespace eirods



#define ERROR( code_, message_ ) ( eirods::error( false, code_, message_, __FILE__, __LINE__, __FUNCTION__ ) )
#define PASS( prev_error_ ) (eirods::error( prev_error_.status(), prev_error_.code(), "", __FILE__, __LINE__, __FUNCTION__, prev_error_ ) )
#define PASSMSG( message_, prev_error_ ) (eirods::error( prev_error_.status(), prev_error_.code(), message_, __FILE__, __LINE__, __FUNCTION__, prev_error_ ) )
#define CODE( code_ ) ( eirods::error( true, code_, "", __FILE__, __LINE__, __FUNCTION__ ) )
#define SUCCESS( ) ( eirods::error( true, 0, "", __FILE__, __LINE__, __FUNCTION__ ) )

#endif // __EIRODS_ERROR_H__



