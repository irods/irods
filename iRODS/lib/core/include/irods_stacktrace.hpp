#ifndef _IRODS_STACKTRACE_HPP_
#define _IRODS_STACKTRACE_HPP_

#include "irods_error.hpp"

#include <list>
#include <string>
#include <iostream>

namespace irods {

    /**
     * @brief Class for generating and manipulating a stack trace
     */
    class stacktrace {
        public:

            /// @brief constructor
            stacktrace( void );
            virtual ~stacktrace( void );

            /// @brief Generates the actual stack trace
            error trace( void );

            /// @brief Dumps the current stack to stderr
            error dump( std::ostream& strm_ = std::cerr );

        private:
            /// @brief function to demangle the c++ function names
            error demangle_symbol( const std::string& _symbol, std::string& _rtn_name, std::string& _rtn_offset );

            typedef struct stack_entry_s {
                std::string function;
                std::string offset;
                void* address;
            } stack_entry_t;

            typedef std::list<stack_entry_t> stacklist;

            stacklist stack_;
    };
}; // namespace irods

#endif // _IRODS_STACKTRACE_HPP_
