/// \file

#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_auth_constants.hpp"
#include "irods/client_connection.hpp"
#include "irods/irods_exception.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_ms_plugin.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/msParam.h"
#include "irods/rcConnect.h"

#include "msi_assertion_macros.hpp"

#define IRODS_USER_ADMINISTRATION_ENABLE_SERVER_SIDE_API
#include "irods/user_administration.hpp"

#include <exception>
#include <functional>
#include <string>

#include <nlohmann/json.hpp>

namespace
{
    auto msi_impl(RuleExecInfo* _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("#7208: user administration library can change passwords inside the server")

        namespace adm = irods::experimental::administration;

        // Create a test user.
        const adm::user test_user{"test_user_user_admin_lib"};
        IRODS_MSI_NOTHROW(adm::server::add_user(*_rei->rsComm, test_user));
        irods::at_scope_exit remove_user{[&] { adm::server::remove_user(*_rei->rsComm, test_user); }};

        // Show the password of the test user can be changed from within the server.
        const adm::user_password_property prop{test_user.name};
        IRODS_MSI_NOTHROW(adm::server::modify_user(*_rei->rsComm, test_user, prop));

        // Show we can connect to the server as the test user.
        const auto& env = _rei->rsComm->myEnv;
        irods::experimental::client_connection conn{
            irods::experimental::defer_authentication, env.rodsHost, env.rodsPort, {test_user.name, env.rodsZone}};
        const auto ctx = nlohmann::json{{irods::AUTH_PASSWORD_KEY, prop.value.data()}};
        IRODS_MSI_ASSERT(clientLogin(static_cast<RcComm*>(conn), ctx.dump().c_str()) == 0);

        IRODS_MSI_TEST_END
    } // msi_impl

    template <typename... Args, typename Function>
    auto make_msi(const std::string& _name, Function _func) -> irods::ms_table_entry*
    {
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        auto* msi = new irods::ms_table_entry{sizeof...(Args)};
        msi->add_operation(_name, std::function<int(Args..., ruleExecInfo_t*)>(_func));
        return msi;
    } // make_msi
} // anonymous namespace

extern "C" auto plugin_factory() -> irods::ms_table_entry*
{
    return make_msi<>("msi_test_user_administration", msi_impl);
} // plugin_factory
