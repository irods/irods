#include <catch2/catch.hpp>

#include "irods/client_connection.hpp"
#include "irods/dataObjInpOut.h"
#include "irods/dstream.hpp"
#include "irods/filesystem.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_exception.hpp"
#include "irods/key_value_proxy.hpp"
#include "irods/objInfo.h"
#include "irods/rcMisc.h"
#include "irods/replica.hpp"
#include "irods/replica_proxy.hpp"
#include "irods/resource_administration.hpp"
#include "irods/rodsClient.h"
#include "irods/rodsDef.h"
#include "irods/rodsErrorTable.h"
#include "irods/transport/default_transport.hpp"
#include "unit_test_utils.hpp"

#include <fmt/format.h>

#include <cstring>
#include <string>
#include <string_view>
#include <thread>

using namespace std::chrono_literals;

// clang-format off
namespace adm	 = irods::experimental::administration;
namespace fs	  = irods::experimental::filesystem;
namespace replica = irods::experimental::replica;
// clang-format on

TEST_CASE("truncate_locked_data_object__issue_7104")
{
    try {
        load_client_api_plugins();

        const std::string test_resc = "test_resc";
        const std::string vault_name = "test_resc_vault";

        // Create a new resource
        {
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);
            unit_test_utils::add_ufs_resource(comm, test_resc, vault_name);
        }

        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        rodsEnv env;
        _getRodsEnv(env);

        const auto sandbox = fs::path{env.rodsHome} / "test_rc_data_obj_truncate";
        if (!fs::client::exists(comm, sandbox)) {
            REQUIRE(fs::client::create_collection(comm, sandbox));
        }

        irods::at_scope_exit remove_sandbox{[&sandbox, &test_resc] {
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            REQUIRE(fs::client::remove_all(comm, sandbox, fs::remove_options::no_trash));

            adm::client::remove_resource(comm, test_resc);
        }};

        const auto target_object = sandbox / "target_object";

        static constexpr auto contents = std::string_view{"content!"};

        // Create a new data object.
        {
            irods::experimental::io::client::native_transport tp{conn};
            irods::experimental::io::odstream{tp, target_object} << contents;
        }

        // Show that the replica is in a good state.
        REQUIRE(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));
        REQUIRE(contents.size() == replica::replica_size(comm, target_object, 0));

        SECTION("single replica")
        {
            // Sleep here so that the mtime on the replica we open CAN be different (but we don't expect it to be).
            const auto original_mtime = replica::last_write_time(comm, target_object, 0);
            std::this_thread::sleep_for(2s);

            // Open the object in read-write mode in order to lock the object.
            DataObjInp doi{};
            std::strncpy(doi.objPath, target_object.c_str(), MAX_NAME_LEN);
            doi.openFlags = O_RDWR;

            // Open the replica and ensure that the status is updated appropriately.
            const auto fd = rcDataObjOpen(&comm, &doi);
            REQUIRE(fd > 2);
            REQUIRE(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));

            irods::experimental::client_connection conn2;
            RcComm& comm2 = static_cast<RcComm&>(conn2);

            DataObjInp truncate_doi{};
            std::strncpy(truncate_doi.objPath, target_object.c_str(), MAX_NAME_LEN);

            SECTION("same size")
            {
                truncate_doi.dataSize = contents.size();
            }

            SECTION("larger size")
            {
                truncate_doi.dataSize = contents.size() + 1;
            }

            SECTION("smaller size")
            {
                truncate_doi.dataSize = contents.size() - 1;
            }

            // Attempt to truncate the object using the size specified for each section, and fail.
            CHECK(LOCKED_DATA_OBJECT_ACCESS == rcDataObjTruncate(&comm2, &truncate_doi));

            // Close the open data object so that it is back at rest.
            OpenedDataObjInp close_inp{};
            close_inp.l1descInx = fd;
            REQUIRE(0 == rcDataObjClose(&comm, &close_inp));

            // Ensure that the object (single replica) was not updated.
            CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));
            CHECK(contents.size() == replica::replica_size(comm, target_object, 0));
            CHECK(original_mtime == replica::last_write_time(comm, target_object, 0));
        }
    }
    catch (const irods::exception& e) {
        fmt::print(stderr, "irods::exception occurred: [{}]", e.what());
    }
    catch (const std::exception& e) {
        fmt::print(stderr, "std::exception occurred: [{}]", e.what());
    }
} // truncate_locked_data_object__issue_7104
