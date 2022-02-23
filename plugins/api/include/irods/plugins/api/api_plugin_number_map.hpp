#ifndef IRODS_API_PLUGIN_NUMBER_MAP_HPP
#define IRODS_API_PLUGIN_NUMBER_MAP_HPP

#include <string>
#include <unordered_map>

#define API_PLUGIN_NUMBER(NAME, VALUE) {VALUE, #NAME},

namespace irods
{
    const std::unordered_map<int, std::string> api_plugin_number_names{
        #include "irods/plugins/api/api_plugin_number_data.h"
    };
} // namespace irods

#undef API_PLUGIN_NUMBER

#endif // IRODS_API_PLUGIN_NUMBER_MAP_HPP

