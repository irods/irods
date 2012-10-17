/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */


#include "eirods_tmp_string.h"

#include <string.h>
#include <malloc.h>

namespace eirods {

    tmp_string::tmp_string(
	const std::string& orig) {

	string_ = strdup(orig.c_str());
    }

    tmp_string::~tmp_string(void) {
	free(string_);
    }

}; // namespace eirods
