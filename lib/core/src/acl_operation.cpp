#include "acl_operation.hpp"

using json = nlohmann::json;

namespace irods::experimental
{
    auto to_json(json& _json, const acl_operation& _op) -> void
    {
        _json = json{
            {"entity_name", _op.entity_name},
            {"entity_type", _op.entity_type},
            {"operation", _op.operation},
            {"permission", _op.permission}
        };
    }

    auto from_json(const json& _json, acl_operation& _op) -> void
    {
        _op.entity_name = _json.at("entity_name").get<std::string>();
        _op.entity_type = _json.at("entity_type").get<std::string>();
        _op.operation = _json.at("operation").get<std::string>();
        _op.permission = _json.at("permission").get<std::string>();
    }
} // namespace irods::experimental

