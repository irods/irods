
#include "miscServerFunct.hpp"
#include "jansson.h"
#include "irods_load_plugin.hpp"
#include "irods_server_properties.hpp"
#include "irods_resource_manager.hpp"

namespace irods {

    error add_plugin_type_to_json_array(
        const std::string _plugin_type,
        const char*        _type_name,
        json_t*&           _json_array );

    error get_plugin_array(
        json_t*& _plugins );

    error serialize_resource_plugin_to_json(
        const resource_ptr& _resc,
        json_t*             _entry );

}; // namespace irods



