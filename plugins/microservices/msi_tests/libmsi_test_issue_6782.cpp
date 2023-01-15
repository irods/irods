/// \file

#include "client_connection.hpp"
#include "irods_exception.hpp"
#include "irods_ms_plugin.hpp"
#include "irods_re_structs.hpp"
#include "msParam.h"

#include "msi_assertion_macros.hpp"

#include "filesystem.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

#include <string>
#include <exception>

namespace
{
    auto msi_impl(RuleExecInfo* _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("#6782: client and server filesystem interfaces can be used in the same translation unit")

        namespace fs = irods::experimental::filesystem;

        fs::path path = "/";
        path /= _rei->rsComm->myEnv.rodsZone; // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        path /= "home";
        path /= _rei->rsComm->clientUser.userName; // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

        IRODS_MSI_ASSERT(fs::server::is_collection(*_rei->rsComm, path))

        irods::experimental::client_connection conn;
        IRODS_MSI_ASSERT(fs::client::is_collection(conn, path))

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
    return make_msi<>("msi_test_issue_6782", msi_impl);
} // plugin_factory
