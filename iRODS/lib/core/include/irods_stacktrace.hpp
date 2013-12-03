/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _stacktrace_H_
#define _stacktrace_H_

#include "eirods_error.hpp"

#include <list>
#include <string>

namespace eirods {

/**
 * @brief Class for generating and manipulating a stack trace
 */
    class stacktrace {
    public:

        /// @brief constructor
        stacktrace(void);
        virtual ~stacktrace(void);

        /// @brief Generates the actual stack trace
        error trace(void);

        /// @brief Dumps the current stack to stderr
        error dump(void);

    private:
        /// @brief function to demangle the c++ function names
        error demangle_symbol(const std::string& _symbol, std::string& _rtn_name, std::string& _rtn_offset);

        typedef struct stack_entry_s {
            std::string function;
            std::string offset;
            void* address;
        } stack_entry_t;
        
        typedef std::list<stack_entry_t> stacklist;
        
        stacklist stack_;
    };
}; // namespace eirods

#endif // _stacktrace_H_
