#ifndef IRODS_CATALOG_UTILITIES_HPP
#define IRODS_CATALOG_UTILITIES_HPP

#include "rcConnect.h"

#include "nanodbc/nanodbc.h"

#include <map>
#include <string>

namespace irods::experimental::catalog {

    enum class entity_type {
        data_object,
        collection,
        user,
        resource,
        zone
    };

    const std::map<std::string, entity_type> entity_type_map{
        {"data_object", entity_type::data_object},
        {"collection", entity_type::collection},
        {"user", entity_type::user},
        {"resource", entity_type::resource},
        {"zone", entity_type::zone}
    };

    auto user_has_permission_to_modify_metadata(rsComm_t& _comm,
                                                nanodbc::connection& _db_conn,
                                                int _object_id,
                                                const entity_type _entity) -> bool;
}

#endif // #ifndef IRODS_CATALOG_UTILITIES_HPP
