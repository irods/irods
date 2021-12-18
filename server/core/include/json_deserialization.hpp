#ifndef IRODS_JSON_DESERIALIZATION_HPP
#define IRODS_JSON_DESERIALIZATION_HPP

/// \file

#include "rodsErrorTable.h"
#include "irods_error.hpp"

#include <nlohmann/json.hpp>

struct MsParam;
struct KeyValPair;
struct AuthInfo;
struct UserOtherInfo;
struct UserInfo;
struct MsParamArray;
struct RuleExecInfo;

namespace irods
{
    auto to_key_value_pair(const nlohmann::json& _json) -> KeyValPair*;

    auto to_ms_param(const nlohmann::json& _json) -> MsParam*;

    auto to_ms_param_array(const nlohmann::json& _json) -> MsParamArray*;

    auto to_auth_info(const nlohmann::json& _json) -> AuthInfo;

    auto to_user_other_info(const nlohmann::json& _json) -> UserOtherInfo;

    auto to_user_info(const nlohmann::json& _json) -> UserInfo*;

    auto to_rule_execution_info(const std::string_view _json_string) -> RuleExecInfo;
} // namespace irods

#endif // IRODS_JSON_DESERIALIZATION_HPP

