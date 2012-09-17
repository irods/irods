


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
        error( bool, int, std::string, int, std::string );  
        error( bool, int, std::string, int, std::string, const error& );  
        error( const error& );   

		// =-=-=-=-=-=-=-
		// Destructor
        ~error();
		 
		// =-=-=-=-=-=-=-
		// Operators
		error& operator=( const error& );
		
		// =-=-=-=-=-=-=-
		// Members
		int         status();
		int         code();
		std::string result();
        bool        ok();

        private:
		// =-=-=-=-=-=-=-
		// Attributes
		bool        status_;
		int         code_;
		std::string message_;
        std::vector< std::string > result_stack_;
		
    }; // class error

}; // namespace eirods



#define ERROR( status_, code_, message_ ) ( eirods::error( status_, code_, message_, __LINE__, __FILE__ ) )
#define PASS( status_, code_, message_, prev_error_ ) ( eirods::error( status_, code_, message_, __LINE__, __FILE__, prev_error_ ) )
#define CODE( code_ ) ( eirods::error( true, code_, "", __LINE__, __FILE__ ) )
#define SUCCESS( ) ( eirods::error( true, 0, "", __LINE__, __FILE__ ) )


#endif // __EIRODS_ERROR_H__



