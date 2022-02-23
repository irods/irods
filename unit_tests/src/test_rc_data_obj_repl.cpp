#include <catch2/catch.hpp>

#include "irods/client_connection.hpp"
#include "irods/dataObjInpOut.h"
#include "irods/dstream.hpp"
#include "irods/filesystem.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods_error_enum_matcher.hpp"
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

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <signal.h>
#include <string>
#include <string_view>

// clang-format off
namespace adm     = irods::experimental::administration;
namespace fs      = irods::experimental::filesystem;
namespace replica = irods::experimental::replica;
// clang-format on

TEST_CASE("verify_checksum")
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

        const auto sandbox = fs::path{env.rodsHome} / "test_rc_data_obj";
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

        const std::string contents = "content!";

        // Create a new data object.
        {
            irods::experimental::io::client::native_transport tp{conn};
            irods::experimental::io::odstream{tp, target_object} << contents;
        }

        // Show that the replica is in a good state.
        CHECK(replica::replica_status<RcComm>(conn, target_object, 0) == GOOD_REPLICA);
        CHECK(replica::replica_size<RcComm>(conn, target_object, 0) == contents.size());

        const std::string expected_checksum = "sha2:Fc4whTlXZ7Ha7bdO80s0oUwLYdzNDBJoEtCkQAcxDIY=";
        const auto [s, slm] = replica::make_replica_proxy(comm, target_object, 0);
        REQUIRE(s.checksum().empty());

        DataObjInp doi{};
        std::strncpy(doi.objPath, target_object.c_str(), MAX_NAME_LEN);

        auto cond_input = irods::experimental::make_key_value_proxy(doi.condInput);
        cond_input[DEST_RESC_NAME_KW] = test_resc;

        SECTION("source replica no checksum")
        {
            SECTION("VERIFY_CHKSUM_KW = 0, REG_CHKSUM_KW = 0, NO_COMPUTE_KW = 0")
            {
                REQUIRE(0 == rcDataObjRepl(&comm, &doi));

                const auto [s, slm] = replica::make_replica_proxy(comm, target_object, 0);
                const auto [d, dlm] = replica::make_replica_proxy(comm, target_object, 1);

                CHECK(s.checksum().empty());
                CHECK(d.checksum().empty());
            }

            SECTION("VERIFY_CHKSUM_KW = 0, REG_CHKSUM_KW = 0, NO_COMPUTE_KW = 1")
            {
                cond_input[NO_COMPUTE_KW] = "";

                REQUIRE(0 == rcDataObjRepl(&comm, &doi));

                const auto [s, slm] = replica::make_replica_proxy(comm, target_object, 0);
                const auto [d, dlm] = replica::make_replica_proxy(comm, target_object, 1);

                CHECK(s.checksum().empty());
                CHECK(d.checksum().empty());
            }

            SECTION("VERIFY_CHKSUM_KW = 0, REG_CHKSUM_KW = 1, NO_COMPUTE_KW = 0")
            {
                cond_input[REG_CHKSUM_KW] = "";

                REQUIRE(0 == rcDataObjRepl(&comm, &doi));

                const auto [s, slm] = replica::make_replica_proxy(comm, target_object, 0);
                const auto [d, dlm] = replica::make_replica_proxy(comm, target_object, 1);

                CHECK(s.checksum().empty());
                CHECK(d.checksum() == expected_checksum);
            }

            SECTION("VERIFY_CHKSUM_KW = 0, REG_CHKSUM_KW = 1, NO_COMPUTE_KW = 1")
            {
                cond_input[REG_CHKSUM_KW] = "";
                cond_input[NO_COMPUTE_KW] = "";

                REQUIRE(USER_INCOMPATIBLE_PARAMS == rcDataObjRepl(&comm, &doi));

                CHECK(!replica::replica_exists(comm, target_object, 1));

                const auto [s, slm] = replica::make_replica_proxy(comm, target_object, 0);

                CHECK(s.checksum().empty());
            }

            SECTION("VERIFY_CHKSUM_KW = 1, REG_CHKSUM_KW = 0, NO_COMPUTE_KW = 0")
            {
                cond_input[VERIFY_CHKSUM_KW] = "";

                // This should result in an error because the VERIFY_CHKSUM_KW indicates that
                // the calculated checksum should be verified when there is nothing against
                // which to verify (that is, there is no checksum on the source replica).
                REQUIRE(CAT_NO_CHECKSUM_FOR_REPLICA == rcDataObjRepl(&comm, &doi));

                CHECK(!replica::replica_exists(comm, target_object, 1));

                const auto [s, slm] = replica::make_replica_proxy(comm, target_object, 0);

                CHECK(s.checksum().empty());
            }

            SECTION("VERIFY_CHKSUM_KW = 1, REG_CHKSUM_KW = 0, NO_COMPUTE_KW = 1")
            {
                cond_input[VERIFY_CHKSUM_KW] = "";
                cond_input[NO_COMPUTE_KW] = "";

                REQUIRE(USER_INCOMPATIBLE_PARAMS == rcDataObjRepl(&comm, &doi));

                CHECK(!replica::replica_exists(comm, target_object, 1));

                const auto [s, slm] = replica::make_replica_proxy(comm, target_object, 0);

                CHECK(s.checksum().empty());
            }

            SECTION("VERIFY_CHKSUM_KW = 1, REG_CHKSUM_KW = 1, NO_COMPUTE_KW = 0")
            {
                cond_input[VERIFY_CHKSUM_KW] = "";
                cond_input[REG_CHKSUM_KW] = "";

                REQUIRE(USER_INCOMPATIBLE_PARAMS == rcDataObjRepl(&comm, &doi));

                CHECK(!replica::replica_exists(comm, target_object, 1));

                const auto [s, slm] = replica::make_replica_proxy(comm, target_object, 0);

                CHECK(s.checksum().empty());
            }

            SECTION("VERIFY_CHKSUM_KW = 1, REG_CHKSUM_KW = 1, NO_COMPUTE_KW = 1")
            {
                cond_input[VERIFY_CHKSUM_KW] = "";
                cond_input[REG_CHKSUM_KW] = "";
                cond_input[NO_COMPUTE_KW] = "";

                REQUIRE(USER_INCOMPATIBLE_PARAMS == rcDataObjRepl(&comm, &doi));

                CHECK(!replica::replica_exists(comm, target_object, 1));

                const auto [s, slm] = replica::make_replica_proxy(comm, target_object, 0);

                CHECK(s.checksum().empty());
            }
        }

        SECTION("source replica has checksum")
        {
            REQUIRE(expected_checksum == replica::replica_checksum(comm, target_object, 0));

            SECTION("VERIFY_CHKSUM_KW = 0, REG_CHKSUM_KW = 0, NO_COMPUTE_KW = 0")
            {
                REQUIRE(0 == rcDataObjRepl(&comm, &doi));

                const auto [s, slm] = replica::make_replica_proxy(comm, target_object, 0);
                const auto [d, dlm] = replica::make_replica_proxy(comm, target_object, 1);

                CHECK(s.checksum() == expected_checksum);
                CHECK(d.checksum() == expected_checksum);
            }

            SECTION("VERIFY_CHKSUM_KW = 0, REG_CHKSUM_KW = 0, NO_COMPUTE_KW = 1")
            {
                cond_input[NO_COMPUTE_KW] = "";

                REQUIRE(0 == rcDataObjRepl(&comm, &doi));

                const auto [s, slm] = replica::make_replica_proxy(comm, target_object, 0);
                const auto [d, dlm] = replica::make_replica_proxy(comm, target_object, 1);

                CHECK(s.checksum() == expected_checksum);
                CHECK(d.checksum().empty());
            }

            SECTION("VERIFY_CHKSUM_KW = 0, REG_CHKSUM_KW = 1, NO_COMPUTE_KW = 0")
            {
                cond_input[REG_CHKSUM_KW] = "";

                REQUIRE(SYS_NOT_ALLOWED == rcDataObjRepl(&comm, &doi));

                CHECK(!replica::replica_exists(comm, target_object, 1));

                const auto [s, slm] = replica::make_replica_proxy(comm, target_object, 0);

                CHECK(s.checksum() == expected_checksum);
            }

            SECTION("VERIFY_CHKSUM_KW = 0, REG_CHKSUM_KW = 1, NO_COMPUTE_KW = 1")
            {
                cond_input[REG_CHKSUM_KW] = "";
                cond_input[NO_COMPUTE_KW] = "";

                REQUIRE(USER_INCOMPATIBLE_PARAMS == rcDataObjRepl(&comm, &doi));

                CHECK(!replica::replica_exists(comm, target_object, 1));

                const auto [s, slm] = replica::make_replica_proxy(comm, target_object, 0);

                CHECK(s.checksum() == expected_checksum);
            }

            SECTION("VERIFY_CHKSUM_KW = 1, REG_CHKSUM_KW = 0, NO_COMPUTE_KW = 0")
            {
                REQUIRE(0 == rcDataObjRepl(&comm, &doi));

                const auto [s, slm] = replica::make_replica_proxy(comm, target_object, 0);
                const auto [d, dlm] = replica::make_replica_proxy(comm, target_object, 1);

                CHECK(s.checksum() == expected_checksum);
                CHECK(d.checksum() == expected_checksum);
            }

            SECTION("VERIFY_CHKSUM_KW = 1, REG_CHKSUM_KW = 0, NO_COMPUTE_KW = 1")
            {
                cond_input[VERIFY_CHKSUM_KW] = "";
                cond_input[NO_COMPUTE_KW] = "";

                REQUIRE(USER_INCOMPATIBLE_PARAMS == rcDataObjRepl(&comm, &doi));

                CHECK(!replica::replica_exists(comm, target_object, 1));

                const auto [s, slm] = replica::make_replica_proxy(comm, target_object, 0);

                CHECK(s.checksum() == expected_checksum);
            }

            SECTION("VERIFY_CHKSUM_KW = 1, REG_CHKSUM_KW = 1, NO_COMPUTE_KW = 0")
            {
                cond_input[VERIFY_CHKSUM_KW] = "";
                cond_input[REG_CHKSUM_KW] = "";

                REQUIRE(USER_INCOMPATIBLE_PARAMS == rcDataObjRepl(&comm, &doi));

                CHECK(!replica::replica_exists(comm, target_object, 1));

                const auto [s, slm] = replica::make_replica_proxy(comm, target_object, 0);

                CHECK(s.checksum() == expected_checksum);
            }

            SECTION("VERIFY_CHKSUM_KW = 1, REG_CHKSUM_KW = 1, NO_COMPUTE_KW = 1")
            {
                cond_input[VERIFY_CHKSUM_KW] = "";
                cond_input[REG_CHKSUM_KW] = "";
                cond_input[NO_COMPUTE_KW] = "";

                REQUIRE(USER_INCOMPATIBLE_PARAMS == rcDataObjRepl(&comm, &doi));

                CHECK(!replica::replica_exists(comm, target_object, 1));

                const auto [s, slm] = replica::make_replica_proxy(comm, target_object, 0);

                CHECK(s.checksum() == expected_checksum);
            }
        }
    }
    catch (const irods::exception& e) {
        std::cout << e.client_display_what();
    }
    catch (const std::exception& e) {
        std::cout << e.what();
    }
} // rc_data_obj_repl

