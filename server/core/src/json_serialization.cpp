#include "irods/json_serialization.hpp"

#include "irods/objInfo.h"
#include "irods/msParam.h"
#include "irods/irods_re_structs.hpp"
#include "irods/irods_logger.hpp"

namespace logger = irods::experimental::log;

namespace
{
    using json = nlohmann::json;

    auto to_string(MsParam& _p) -> char*
    {
        auto* s = parseMspForStr(&_p);

        if (!s) {
            THROW(SYS_INVALID_INPUT_PARAM, "Failed to convert microservice argument to string.");
        }

        return s;
    }

    template <typename T>
    constexpr auto to_floating_point(const MsParam& _p) -> T
    {
        T v = 0;

        if constexpr (std::is_same_v<T, double>) {
            if (const auto ec = parseMspForDouble(const_cast<MsParam*>(&_p), &v); ec != 0) {
                THROW(SYS_INVALID_INPUT_PARAM, "Failed to convert microservice argument to double.");
            }
        }
        else if constexpr (std::is_same_v<T, float>) {
            if (const auto ec = parseMspForFloat(const_cast<MsParam*>(&_p), &v); ec != 0) {
                THROW(SYS_INVALID_INPUT_PARAM, "Failed to convert microservice argument to float.");
            }
        }
        else {
            THROW(SYS_INVALID_INPUT_PARAM, "Invalid floating point type.");
        }

        return v;
    }
} // anonymous namespace

namespace irods
{
    auto to_json(char* _p[], int _size) -> nlohmann::json
    {
        auto argv = json::array();

        for (int i = 0; i < _size; ++i) {
            argv.push_back(_p[i]);
        }

        return argv;
    }

    auto to_json(const KeyValPair* _p) -> nlohmann::json
    {
        if (!_p) {
            return nullptr;
        }

        auto json_object = json::object();

        for (int i = 0; i < _p->len; ++i) {
            json_object[_p->keyWord[i]] = _p->value[i];
        }

        return json_object;
    }

    auto to_json(const AuthInfo* _p) -> nlohmann::json
    {
        if (!_p) {
            return nullptr;
        }

        return {
            {"scheme", _p->authScheme},
            {"auth_flag", _p->authFlag},
            {"flag", _p->flag},
            {"ppid", _p->ppid},
            {"host", _p->host},
            {"auth_string", _p->authStr}
        };
    }

    auto to_json(const UserOtherInfo* _p) -> nlohmann::json
    {
        if (!_p) {
            return nullptr;
        }

        return {
            {"info", _p->userInfo},
            {"comments", _p->userComments},
            {"ctime", _p->userCreate},
            {"mtime", _p->userModify}
        };
    }

    auto to_json(const UserInfo* _p) -> nlohmann::json
    {
        if (!_p) {
            return nullptr;
        }

        return {
            {"name", _p->userName},
            {"type", _p->userType},
            {"zone", _p->rodsZone},
            {"system_uid", _p->sysUid},
            {"auth_info", to_json(&_p->authInfo)},
            {"other_info", to_json(&_p->userOtherInfo)},
        };
    }

    auto to_json(const MsParam* _p) -> nlohmann::json
    {
        if (!_p) {
            return nullptr;
        }

        json param{
            {"label", _p->label},
            {"type", nullptr},
            {"in_out_struct", nullptr}
        };

        // Input variables will have a type associated with them (most likely STR_PI).
        // Output variables will NOT have a type associated with them.
        if (_p->inOutStruct) {
            if (_p->type) {
                if (std::string_view{STR_MS_T} == _p->type) {
                    auto* v = to_string(const_cast<msParam_t&>(*_p));
                    param["type"] = _p->type;
                    param["in_out_struct"] = v;
                }
                else if (std::string_view{INT_MS_T} == _p->type) {
                    param["type"] = _p->type;
                    param["in_out_struct"] = parseMspForPosInt(const_cast<MsParam*>(_p));
                }
                else if (std::string_view{DOUBLE_MS_T} == _p->type) {
                    param["type"] = _p->type;
                    param["in_out_struct"] = to_floating_point<double>(*_p);
                }
                else if (std::string_view{FLOAT_MS_T} == _p->type) {
                    param["type"] = _p->type;
                    param["in_out_struct"] = to_floating_point<float>(*_p);
                }
                else if (std::string_view{KeyValPair_MS_T} == _p->type) {
                    param["type"] = _p->type;
                    param["in_out_struct"] = to_json(static_cast<KeyValPair*>(_p->inOutStruct));
                }
                else {
                    logger::microservice::warn(
                        "Microservice parameter type [{}] is not supported. Ignoring parameter.",
                        _p->type);
                }
            }
            else {
                logger::microservice::warn(
                    "Cannot store microservice parameter (MsParam::inOutStruct). "
                    "No type information available.");
            }
        }

        if (_p->inpOutBuf) {
            logger::microservice::warn("Cannot store microservice parameter (MsParam::inpOutBuf).");
        }

        return param;
    }

    auto to_json(const MsParamArray* _p) -> nlohmann::json
    {
        if (!_p) {
            return nullptr;
        }

        json param_array{
            {"operation_type", _p->oprType}
        };

        auto params = json::array();

        for (int i = 0; i < _p->len; ++i) {
            params.push_back(to_json(_p->msParam[i]));
        }

        param_array["ms_params"] = params;

        return param_array;
    }

    auto to_json(const RuleExecInfo* _p) -> nlohmann::json
    {
        if (!_p) {
            return nullptr;
        }

        return {
            {"rule_name", _p->ruleName},
            {"plugin_instance_name", _p->pluginInstanceName},
            {"ms_param_array", to_json(_p->msParamArray)},
            {"resource_name", _p->rescName},
            {"user_info_client", to_json(_p->uoic)},
            {"user_info_proxy", to_json(_p->uoip)},
            {"user_info_other", to_json(_p->uoio)},
            {"conditional_input", to_json(_p->condInputData)}
        };
    }
} // namespace irods

