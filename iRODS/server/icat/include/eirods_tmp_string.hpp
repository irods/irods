/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _tmp_string_H_
#define _tmp_string_H_

#include <string>

/// @brief This class provides a non-const char* that is automatically cleaned up when it goes out of scope
namespace eirods {

    class tmp_string {
    public:
        tmp_string(const char* orig);
        virtual ~tmp_string(void);

        /// @brief provides the non-const char*. However its memory will be freed when the tmp_string goes out of scope
        char* str(void) { return string_; }
        
    private:
        char* string_;
    };
    
}; // namespace eirods

#endif // _tmp_string_H_
