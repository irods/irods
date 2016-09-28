#include "jansson.h"
#include "irods_resource_manager.hpp"

namespace irods {
    error serialize_resource_plugin_to_json(
        const resource_ptr& _resc,
        json_t*             _entry );

}; // namespace irods

