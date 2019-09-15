#include "metadata_operation.hpp"

using json = nlohmann::json;

namespace irods::experimental
{
    auto to_json(json& _json, const metadata_operation& _op) -> void
    {
        _json = json{
            {"entity_name", _op.entity_name},
            {"entity_type", _op.entity_type},
            {"operation", _op.operation},
            {"attribute", _op.attribute},
            {"value", _op.value},
            {"units", _op.units}
        };
    }

    auto from_json(const json& _json, metadata_operation& _op) -> void
    {
        _op.entity_name = _json.at("entity_name").get<std::string>();
        _op.entity_type = _json.at("entity_type").get<std::string>();
        _op.operation = _json.at("operation").get<std::string>();
        _op.attribute = _json.at("attribute").get<std::string>();
        _op.value = _json.at("value").get<std::string>();
        _op.units = _json.at("units").get<std::string>();
    }
} // namespace irods::experimental

