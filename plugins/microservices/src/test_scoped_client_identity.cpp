/// \file

#include "irods/getRodsEnv.h"
#include "irods/irods_exception.hpp"
#include "irods/irods_ms_plugin.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/msParam.h"
#include "irods/scoped_client_identity.hpp"

#include "msi_assertion_macros.hpp"

#include <exception>
#include <string>

namespace
{
    auto test_basic_usage(RuleExecInfo& _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("scoped_client_identity - basic usage")

        const std::string username = "rods";
        const std::string zone = "tempZone";

        RsComm comm{};
        std::strcpy(comm.clientUser.userName, username.c_str());
        std::strcpy(comm.clientUser.rodsZone, zone.c_str());

        // Show that the RsComm represents rods#tempZone.
        IRODS_MSI_ASSERT(comm.clientUser.userName == username)
        IRODS_MSI_ASSERT(comm.clientUser.rodsZone == zone)

        {
            const std::string new_username = "otherrods";
            irods::experimental::scoped_client_identity sci{comm, new_username};

            // Show that the RsComm represents otherrods#tempZone.
            IRODS_MSI_ASSERT(comm.clientUser.userName == new_username)
            IRODS_MSI_ASSERT(comm.clientUser.rodsZone == zone)
        }

        // Show that the original identity associated with the RsComm has been restored
        // (i.e. rods#tempZone).
        IRODS_MSI_ASSERT(comm.clientUser.userName == username)
        IRODS_MSI_ASSERT(comm.clientUser.rodsZone == zone)

        IRODS_MSI_TEST_END
    } // test_basic_usage

    auto test_updates_zone(RuleExecInfo& _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("#6268: scoped_client_identity updates the zone")

        rodsEnv env;
        _getRodsEnv(env);

        const std::string username = "rods";
        const std::string zone = env.rodsZone;

        RsComm comm{};
        std::strcpy(comm.clientUser.userName, username.c_str());

        // Show that using the two-argument constructor results in the user
        // being identified as a member of the local zone.
        {
            const std::string new_username = "alice";
            irods::experimental::scoped_client_identity sci{comm, new_username};
            IRODS_MSI_ASSERT(comm.clientUser.userName == new_username)
            IRODS_MSI_ASSERT(comm.clientUser.rodsZone == zone)
        }

        // Show that the RsComm was restored to rods#<empty_string>.
        IRODS_MSI_ASSERT(comm.clientUser.userName == username)
        IRODS_MSI_ASSERT(std::strlen(comm.clientUser.rodsZone) == 0)

        std::strcpy(comm.clientUser.rodsZone, zone.c_str());

        // Show that the RsComm represents rods#tempZone.
        IRODS_MSI_ASSERT(comm.clientUser.userName == username)
        IRODS_MSI_ASSERT(comm.clientUser.rodsZone == zone)

        // Show that scoped_client_identity updates both the username and zone.
        {
            const std::string new_username = "otherrods";
            const std::string new_zone = "otherZone";
            irods::experimental::scoped_client_identity sci{comm, new_username, new_zone};
            IRODS_MSI_ASSERT(comm.clientUser.userName == new_username)
            IRODS_MSI_ASSERT(comm.clientUser.rodsZone == new_zone)
        }

        // Show that the identity associated with the RsComm has been restored.
        IRODS_MSI_ASSERT(comm.clientUser.userName == username)
        IRODS_MSI_ASSERT(comm.clientUser.rodsZone == zone)

        IRODS_MSI_TEST_END
    } // test_updates_zone

    auto test_invalid_input(RuleExecInfo& _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("scoped_client_identity throws on invalid input")

        const std::string username = "rods";
        const std::string zone = "tempZone";

        RsComm comm{};
        std::strcpy(comm.clientUser.userName, username.c_str());
        std::strcpy(comm.clientUser.rodsZone, zone.c_str());

        IRODS_MSI_THROWS_MSG((irods::experimental::scoped_client_identity{comm, ""}),
                             irods::exception,
                             "scoped_client_identity: username is empty.")

        const auto* too_long = "64_bytes_long___________________________________________________";

        IRODS_MSI_THROWS_MSG((irods::experimental::scoped_client_identity{comm, too_long}),
                             irods::exception,
                             "scoped_client_identity: username is too long.")

        IRODS_MSI_THROWS_MSG((irods::experimental::scoped_client_identity{comm, "otherrods", too_long}),
                             irods::exception,
                             "scoped_client_identity: zone is too long.")

        IRODS_MSI_TEST_END
    } // test_updates_zone

    auto msi_impl([[maybe_unused]] RuleExecInfo* _rei) -> int
    {
        IRODS_MSI_TEST_CASE(test_basic_usage, *_rei)
        IRODS_MSI_TEST_CASE(test_updates_zone, *_rei)
        IRODS_MSI_TEST_CASE(test_invalid_input, *_rei)

        return 0;
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
    return make_msi<>("msi_test_scoped_client_identity", msi_impl);
} // plugin_factory
