#ifndef IRODS_ACL_OPERATION_HPP
#define IRODS_ACL_OPERATION_HPP

#include <string>

#include "json.hpp"

namespace irods::experimental
{
    struct acl_operation
    {
        std::string entity_name;
        std::string entity_type;
        std::string operation;
        std::string permission;
    };

    auto to_json(nlohmann::json& _json, const acl_operation& _op) -> void;

    auto from_json(const nlohmann::json& _json, acl_operation& _op) -> void;
} // namespace irods::experimental

#endif // IRODS_ACL_OPERATION_HPP

