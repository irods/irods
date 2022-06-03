#include "irods/irods_server_properties.hpp"
#include "irods/irods_ms_plugin.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/msParam.h"
#include "irods/rodsErrorTable.h"
#include "irods/irods_error.hpp"
#include "irods/irods_logger.hpp"

#include <exception>
#include <functional>
#include <string>

namespace
{
    using log_msi = irods::experimental::log::microservice;

    auto msi_impl(msParam_t* _in_name, msParam_t* _out_value, ruleExecInfo_t* rei) -> int
    {
        if (!rei || !rei->rsComm) {
            log_msi::error("msi_get_server_property: input rei or rsComm is NULL.");
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }

        // ensure that this user is an admin
        if (rei->uoic->authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
            log_msi::error("msi_get_server_property: User {} is not local admin. status = {}",
                           rei->uoic->userName,
                           CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
            return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
        }

        try {
            const std::string name = parseMspForStr(_in_name);
            const nlohmann::json* json_value = nullptr;
            const auto& config = irods::server_properties::server_properties::instance().map();

            try {
                if (name.find("/") != std::string::npos) {
                    json_value = &config.at(nlohmann::json::json_pointer(name));
                }
                else {
                    json_value = &config.at(name);
                }
            }
            catch (nlohmann::json::out_of_range& e) {
                log_msi::error("Property not found. An error occurred {}", e.what());
                return KEY_NOT_FOUND;
            }
            catch (nlohmann::json::parse_error& e) {
                log_msi::error("Parse error in key. {}", e.what());
                return KEY_NOT_FOUND;
            }

            const std::string value = json_value->dump();
            fillStrInMsParam(_out_value, value.c_str());
            return 0;
        }
        catch (const irods::exception& e) {
            log_msi::error(e.what());
            return e.code();
        }
        catch (const std::exception& e) {
            log_msi::error(e.what());
            return SYS_INTERNAL_ERR;
        }
        catch (...) {
            log_msi::error("An unknown error occurred while processing the request.");
            return SYS_UNKNOWN_ERROR;
        }
    } // msi_impl

    template <typename... Args, typename Function>
    auto make_msi(const std::string& name, Function func) -> irods::ms_table_entry*
    {
        auto* msi = new irods::ms_table_entry{sizeof...(Args)};
        msi->add_operation(name, std::function<int(Args..., ruleExecInfo_t*)>(func));
        return msi;
    } // make_msi
} // anonymous namespace

extern "C" auto plugin_factory() -> irods::ms_table_entry*
{
    return make_msi<msParam_t*, msParam_t*>("msi_get_server_property", msi_impl);
}

#ifdef IRODS_FOR_DOXYGEN
/// \brief Gets the value of a value in the server_properties map. Requires administrative
/// permissions.
///
/// \param[in]      in_name   The name of the property to retrieve, may be a json pointer or a simple
///                           name.
/// \param[out]     out_value The value of the property. It is in serialized form if it is an object,
///                           otherwise it is converted to a string.
/// \param[in,out]  rei       The rule execution context. Ignore this parameter as a user.
///
/// \return An integer.
/// \retval 0        On success
/// \retval Non-zero On failure
/// \retval CAT_INSUFFICIENT_PRIVILEGE_LEVEL The calling user isn't an administrator
/// \retval KEY_NOT_FOUND The key provided does not have a set value in the configuration
/// \retval INVALID_OBJECT_NAME The key provided is an invalid json pointer
///
/// \since 4.3.1
auto msi_get_server_property(msParam_t* in_name, msParam_t out_value, ruleExecInfo_t* rei) -> int;
#endif // IRODS_FOR_DOXYGEN
