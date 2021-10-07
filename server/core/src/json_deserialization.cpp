#include "json_deserialization.hpp"

#include "objInfo.h"
#include "msParam.h"
#include "irods_re_structs.hpp"

using json = nlohmann::json;

namespace irods
{
    auto to_key_value_pair(const nlohmann::json& _json) -> KeyValPair*
    {
        if (_json.is_null()) {
            return nullptr;
        }

        auto* p = static_cast<KeyValPair*>(std::malloc(sizeof(KeyValPair)));

        std::memset(p, 0, sizeof(KeyValPair));

        p->len = _json.size();
        p->keyWord = static_cast<char**>(std::malloc(sizeof(char*) * p->len));
        p->value = static_cast<char**>(std::malloc(sizeof(char*) * p->len));

        if (_json.is_object()) {
            int i = 0;
            for (auto&& [k, v] : _json.items()) {
                p->keyWord[i] = strdup(k.data());
                p->value[i] = strdup(v.is_null() ? "" : v.get_ref<const std::string&>().data());
                ++i;
            }
        }
        else {
            // This block handles the case where the KeyValPair type was serialized
            // into an array of objects holding a key and a value (e.g. [{key, value}, ...]).
            // This block allows for a clean transition from the old wasteful format to a better,
            // more memory efficient format.
            for (int i = 0; i < p->len; ++i) {
                const auto& object = _json.at(i);
                p->keyWord[i] = strdup(object.at("key").get_ref<const std::string&>().data());
                p->value[i] = strdup(object.at("value").get_ref<const std::string&>().data());
            }
        }

        return p;
    }

    auto to_ms_param(const nlohmann::json& _json) -> MsParam*
    {
        if (_json.is_null()) {
            return nullptr;
        }

        auto* p = static_cast<MsParam*>(std::malloc(sizeof(MsParam)));

        std::memset(p, 0, sizeof(MsParam));

        p->label = strdup(_json.at("label").get_ref<const std::string&>().data());

        // Input variables will have a type associated with them.
        // Output variables will NOT have a type associated with them.
        if (const auto& type = _json.at("type"); type.is_string()) {
            p->type = strdup(type.get_ref<const std::string&>().data());
        }

        if (const auto& value = _json.at("in_out_struct"); !value.empty()) {
            if (p->type) {
                if (std::string_view{STR_MS_T} == p->type) {
                    p->inOutStruct = strdup(value.get_ref<const std::string&>().data());
                }
                else if (std::string_view{INT_MS_T} == p->type) {
                    auto* v = static_cast<int*>(std::malloc(sizeof(int)));
                    *v = value.get<int>();
                    p->inOutStruct = v;
                }
                else if (std::string_view{DOUBLE_MS_T} == p->type) {
                    auto* v = static_cast<int*>(std::malloc(sizeof(double)));
                    *v = value.get<double>();
                    p->inOutStruct = v;
                }
                else if (std::string_view{FLOAT_MS_T} == p->type) {
                    auto* v = static_cast<int*>(std::malloc(sizeof(float)));
                    *v = value.get<float>();
                    p->inOutStruct = v;
                }
                else if (std::string_view{KeyValPair_MS_T} == p->type) {
                    p->inOutStruct = to_key_value_pair(value);
                }
                else {
                    rodsLog(LOG_WARNING, "Microservice parameter type [%s] is not supported. Ignoring parameter.",
                            p->type);
                }
            }
            else {
                rodsLog(LOG_WARNING, "Cannot read microservice parameter (MsParam::inOutStruct). "
                                     "No type information available.");
            }
        }

        return p;
    }

    auto to_ms_param_array(const nlohmann::json& _json) -> MsParamArray*
    {
        if (_json.is_null()) {
            return nullptr;
        }

        auto* p = static_cast<MsParamArray*>(std::malloc(sizeof(MsParamArray)));

        std::memset(p, 0, sizeof(MsParamArray));

        p->len = _json.at("ms_params").size();
        p->oprType = _json.at("operation_type").get<int>();
        p->msParam = static_cast<MsParam**>(std::malloc(sizeof(MsParam*) * p->len));

        const auto& params = _json.at("ms_params");

        for (int i = 0; i < p->len; ++i) {
            p->msParam[i] = to_ms_param(params.at(i));
        }

        return p;
    }

    auto to_auth_info(const nlohmann::json& _json) -> AuthInfo
    {
        AuthInfo info{};

        info.authFlag = _json.at("auth_flag").get<int>();
        info.flag = _json.at("flag").get<int>();
        info.ppid = _json.at("ppid").get<int>();

        rstrcpy(info.authScheme, _json.at("scheme").get_ref<const std::string&>().data(), sizeof(AuthInfo::authScheme));
        rstrcpy(info.host, _json.at("host").get_ref<const std::string&>().data(), sizeof(AuthInfo::host));
        rstrcpy(info.authStr, _json.at("auth_string").get_ref<const std::string&>().data(), sizeof(AuthInfo::authStr));

        return info;
    }

    auto to_user_other_info(const nlohmann::json& _json) -> UserOtherInfo
    {
        UserOtherInfo info{};

        rstrcpy(info.userInfo, _json.at("info").get_ref<const std::string&>().data(), sizeof(UserOtherInfo::userInfo));
        rstrcpy(info.userComments, _json.at("comments").get_ref<const std::string&>().data(), sizeof(UserOtherInfo::userComments));
        rstrcpy(info.userCreate, _json.at("ctime").get_ref<const std::string&>().data(), sizeof(UserOtherInfo::userCreate));
        rstrcpy(info.userModify, _json.at("mtime").get_ref<const std::string&>().data(), sizeof(UserOtherInfo::userModify));

        return info;
    }

    auto to_user_info(const nlohmann::json& _json) -> UserInfo*
    {
        if (_json.is_null()) {
            return nullptr;
        }

        auto* p = static_cast<UserInfo*>(std::malloc(sizeof(UserInfo)));

        std::memset(p, 0, sizeof(UserInfo));

        rstrcpy(p->userName, _json.at("name").get_ref<const std::string&>().data(), sizeof(UserInfo::userName));
        rstrcpy(p->userType, _json.at("type").get_ref<const std::string&>().data(), sizeof(UserInfo::userType));
        rstrcpy(p->rodsZone, _json.at("zone").get_ref<const std::string&>().data(), sizeof(UserInfo::rodsZone));

        p->sysUid = _json.at("system_uid").get<int>();
        p->authInfo = to_auth_info(_json.at("auth_info"));
        p->userOtherInfo = to_user_other_info(_json.at("other_info"));

        return p;
    }

    auto to_rule_execution_info(const std::string_view _json_string) -> RuleExecInfo
    {
        try {
            const auto json_data = json::parse(_json_string);

            RuleExecInfo rei{};

            rstrcpy(rei.ruleName, json_data.at("rule_name").get_ref<const std::string&>().data(), sizeof(rei.ruleName));
            rstrcpy(rei.pluginInstanceName, json_data.at("plugin_instance_name").get_ref<const std::string&>().data(), sizeof(rei.pluginInstanceName));
            rstrcpy(rei.rescName, json_data.at("resource_name").get_ref<const std::string&>().data(), sizeof(rei.rescName));

            rei.msParamArray = to_ms_param_array(json_data.at("ms_param_array"));
            rei.uoic = to_user_info(json_data.at("user_info_client"));
            rei.uoip = to_user_info(json_data.at("user_info_proxy"));
            rei.uoio = to_user_info(json_data.at("user_info_other"));
            rei.condInputData = to_key_value_pair(json_data.at("conditional_input"));

            return rei;
        }
        catch (const json::exception&) {
            THROW(SYS_INVALID_INPUT_PARAM, "Failed to parse JSON string into object.");
        }
    }
} // namespace irods

