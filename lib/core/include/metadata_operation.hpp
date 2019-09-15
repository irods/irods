#ifndef IRODS_METADATA_OPERATION_HPP
#define IRODS_METADATA_OPERATION_HPP

#include <string>

#include "json.hpp"

namespace irods::experimental
{
    struct metadata_operation
    {
        std::string entity_name;
        std::string entity_type;
        std::string operation;
        std::string attribute;
        std::string value;
        std::string units;
    };

    auto to_json(nlohmann::json& _json, const metadata_operation& op) -> void;

    auto from_json(const nlohmann::json& _json, metadata_operation& _op) -> void;
} // namespace irods::experimental

#endif // IRODS_METADATA_OPERATION_HPP

