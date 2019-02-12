#ifndef IRODS_API_NUMBER_MAP_H
#define IRODS_API_NUMBER_MAP_H

#include <string>
#include <unordered_map>

#define API_NUMBER(NAME, VALUE) {VALUE, #NAME},

namespace irods
{
    const std::unordered_map<int, std::string> api_number_names{
        #include "apiNumberData.h"
    };
} // namespace irods

#undef API_NUMBER

#endif  // IRODS_API_NUMBER_MAP_H
