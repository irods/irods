/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */


#include "eirods_tmp_string.h"

#include <string.h>
#include <malloc.h>

namespace eirods {

    tmp_string::tmp_string(
        const char* orig) : string_(0){

        if(orig != 0) {
            string_ = strdup(orig);
        }
    }

    
    tmp_string::~tmp_string(void) {
        if(string_ != 0) {
            free(string_);
        }
    }

}; // namespace eirods
