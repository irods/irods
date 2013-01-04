/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _stacktrace_H_
#define _stacktrace_H_

#include "eirods_error.h"

#include <list>
#include <string>

namespace eirods {

/**
 * @brief Class for generating and manipulating a stack trace
 */
    class stacktrace {
    public:

        typedef std::list<std::string> stacklist;
        
        /// @brief constructor
        stacktrace(void);
        virtual ~stacktrace(void);

        /// @brief Generates the actual stack trace
        error trace(void);

        /// @brief Dumps the current stack to stderr
        error dump(void);

    private:
        stacklist stack_;
    };
}; // namespace eirods

#endif // _stacktrace_H_
