#include "miscServerFunct.hpp"
#include "irods_load_plugin.hpp"
#include "irods_server_properties.hpp"
#include "irods_resource_manager.hpp"

#include "json.hpp"

namespace irods {

    error add_plugin_type_to_json_array(
        const std::string _plugin_type,
        const char*       _type_name,
        nlohmann::json&   _json_array);

    error get_plugin_array(nlohmann::json& _plugins);

    error serialize_resource_plugin_to_json(
        const resource_ptr& _resc,
        nlohmann::json&     _entry);

}; // namespace irods



