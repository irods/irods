
#include "miscServerFunct.hpp"
#include "jansson.h"
#include "irods_load_plugin.hpp"
#include "irods_server_properties.hpp"

namespace irods {

    error add_plugin_type_to_json_array(
        const std::string _plugin_type,
        const char*        _type_name,
        json_t*&           _json_array );

    error get_plugin_array(
        json_t*& _plugins );

}; // namespace irods



