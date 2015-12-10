#ifndef __IRODS_ERROR_HPP__
#define __IRODS_ERROR_HPP__

// =-=-=-=-=-=-=-
// STL Includes
#include <string>
#include <sstream>
#include <vector>
#include <cstdarg>

// =-=-=-=-=-=-=-
// irods includes
#include "rodsType.h"

// =-=-=-=-=-=-=-
// boost assertion handling macro, needed everywhere
// see http://stackoverflow.com/questions/1067226/c-multi-line-macro-do-while0-vs-scope-block
// for the WHY!? regarding the while(0) loop
#ifndef BOOST_ASSERT_MSG
#define BOOST_ASSERT_MSG( cond, msg ) do \
{ if (!(cond)) { std::ostringstream str; str << msg; std::cerr << str.str(); std::abort(); } \
} while(0)
#endif
#include <boost/assert.hpp>

namespace irods {
/// =-=-=-=-=-=-=-
/// @brief error stack object which holds error history
    class error {
        public:
            // =-=-=-=-=-=-=-
            // Constructors
            error();
            error(
                bool,          // status
                long long,     // error code
                std::string,   // message
                std::string,   // file name
                int,           // line number
                std::string ); // function
            error(                  // deprecated since 4.0.3
                bool,           // status
                long long,      // error code
                std::string,    // message
                std::string,    // file name
                int,            // line number
                std::string,    // function
                const error& ); // previous error
            error(
                std::string,    // message
                std::string,    // file name
                int,            // line number
                std::string,    // function
                const error& ); // previous error
            error( const error& );

            // =-=-=-=-=-=-=-
            // Destructor
            ~error();

            // =-=-=-=-=-=-=-
            // Operators
            error& operator=( const error& );

            // =-=-=-=-=-=-=-
            // Members
            bool        status() const;
            long long   code() const;
            std::string result() const;
            bool        ok();          // deprecated since 4.0.3
            bool        ok() const;

            // =-=-=-=-=-=-=-
            // Mutators
            void code( long long _code ) {
                code_   = _code;
            }
            void status( bool      _status ) {
                status_ = _status;
            }
            void message( std::string _message ) {
                message_ = _message;
            }

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

    error assert_error( bool expr_, long long code_, const std::string& file_, const std::string& function_, const std::string& format_, int line_, ... );
    error assert_pass( const error& _error, const std::string& _file, const std::string& _function, const std::string& _format, int line_, ... );

}; // namespace irods



#define ERROR( code_, message_ ) ( irods::error( false, code_, message_, __FILE__, __LINE__, __PRETTY_FUNCTION__ ) )
#define PASS( prev_error_ ) (irods::error( "", __FILE__, __LINE__, __PRETTY_FUNCTION__, prev_error_ ) )
#define PASSMSG( message_, prev_error_ ) (irods::error( message_, __FILE__, __LINE__, __PRETTY_FUNCTION__, prev_error_ ) )
#define CODE( code_ ) ( irods::error( true, code_, "", __FILE__, __LINE__, __PRETTY_FUNCTION__ ) )
#define SUCCESS( ) ( irods::error( true, 0, "", __FILE__, __LINE__, __PRETTY_FUNCTION__ ) )

#define ASSERT_ERROR(expr_, code_, format_, ...)  (irods::assert_error(expr_, code_, __FILE__, __PRETTY_FUNCTION__, format_, __LINE__, ##__VA_ARGS__))
#define ASSERT_PASS(prev_error_, format_, ...) (irods::assert_pass(prev_error_, __FILE__, __PRETTY_FUNCTION__, format_, __LINE__, ##__VA_ARGS__))

#endif // __IRODS_ERROR_HPP__
