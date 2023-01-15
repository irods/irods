/// \file

#include "client_connection.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_exception.hpp"
#include "irods_ms_plugin.hpp"
#include "irods_re_structs.hpp"
#include "msParam.h"

#include "msi_assertion_macros.hpp"

#include "dstream.hpp"
#include "transport/default_transport.hpp"

#define IRODS_IO_TRANSPORT_ENABLE_SERVER_SIDE_API
#include "transport/default_transport.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

#include <string>
#include <string_view>
#include <exception>

namespace
{
    auto msi_impl(RuleExecInfo* _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("#6829: client and server dstream interfaces can be used in the same translation unit")

        namespace fs = irods::experimental::filesystem;
        namespace io = irods::experimental::io;

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        const auto* username = _rei->rsComm->clientUser.userName;

        fs::path path = "/";
        path /= _rei->rsComm->myEnv.rodsZone; // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        path /= "home";
        path /= username; // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        path /= "test_data_object_6829.txt";

        irods::at_scope_exit remove_data_object{
            [&] { fs::server::remove(*_rei->rsComm, path, fs::remove_options::no_trash); }};

        const std::string_view contents = "Important data for #6829.";

        // Create a new data object using the server-side API.
        {
            io::server::native_transport tp{*_rei->rsComm};
            io::odstream{tp, path} << contents;
            IRODS_MSI_ASSERT(fs::server::is_data_object(*_rei->rsComm, path));
        }

        //
        // Read the newly created data object's contents using the client-side API.
        //

        const auto& env = _rei->rsComm->myEnv;

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        irods::experimental::client_connection conn{env.rodsHost, env.rodsPort, username, env.rodsZone};
        IRODS_MSI_ASSERT(conn);

        io::client::native_transport tp{conn};
        io::idstream in{tp, path};
        IRODS_MSI_ASSERT(in.is_open());

        std::string line;
        IRODS_MSI_ASSERT(std::getline(in, line));
        IRODS_MSI_ASSERT(contents == line);

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
    return make_msi<>("msi_test_issue_6829", msi_impl);
} // plugin_factory
