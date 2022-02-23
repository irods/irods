#ifndef IRODS_JSON_SERIALIZATION_HPP
#define IRODS_JSON_SERIALIZATION_HPP

/// \file

#include "irods/rodsErrorTable.h"
#include "irods/irods_error.hpp"

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
    auto to_json(char* _p[], int _size) -> nlohmann::json;

    auto to_json(const KeyValPair* _p) -> nlohmann::json;

    auto to_json(const AuthInfo* _p) -> nlohmann::json;

    auto to_json(const UserOtherInfo* _p) -> nlohmann::json;

    auto to_json(const UserInfo* _p) -> nlohmann::json;

    auto to_json(const MsParam* _p) -> nlohmann::json;

    auto to_json(const MsParamArray* _p) -> nlohmann::json;

    auto to_json(const RuleExecInfo* _p) -> nlohmann::json;
} // namespace irods

#endif // IRODS_JSON_SERIALIZATION_HPP

