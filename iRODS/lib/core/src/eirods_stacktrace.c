/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "eirods_stacktrace.h"

#include <iostream>

#include <execinfo.h>
#include <stdlib.h>

namespace eirods {

    static const int max_stack_size = 50;
    
    stacktrace::stacktrace(void) {
        
    }

    stacktrace::~stacktrace(void) {
        // TODO - stub
    }

    error stacktrace::trace(void) {
        error result = SUCCESS();
        void** buffer = new void*[max_stack_size];
        stack_.clear();
        int size = backtrace(buffer, max_stack_size);
        if(size) {
            char** symbols = backtrace_symbols(buffer, size);
            if(symbols) {
                for(int i = 0; i < size; ++i) {
                    char* symbol = symbols[i];
                    if(symbol) {
                        stack_.push_back(std::string(symbol));
                    } else {
                        result = ERROR(-1, "Corrupt stack trace. Symbol is NULL.");
                    }
                }
                free(symbols);
            } else {
                result = ERROR(-1, "Cannot generate stack symbols");
            }
        } else {
            result = ERROR(-1, "Stack trace is empty");
        }
        delete [] buffer;
        return result;
    }

    error stacktrace::dump(void) {
        error result = SUCCESS();
        int frame = 0;
        std::cerr << std::endl << "Dumping stack trace" << std::endl;
        for(stacklist::const_iterator it = stack_.begin(); it != stack_.end(); ++it) {
            std::cerr << "<" << frame << ">";
            std::cerr << " " << *it;
            std::cerr << std::endl;
            ++frame;
        }
        std::cerr << std::endl;
        return result;
    }

}; // namespace eirods
