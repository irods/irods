#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "client_connection.hpp"
#include "connection_pool.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "dstream.hpp"
#include "transport/default_transport.hpp"

#include "filesystem.hpp"
#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "miscUtil.h"
#include "cpUtil.h"
#include "rcGlobalExtern.h"
#include "irods_virtual_path.hpp"
#include "rodsLog.h"

#include <fmt/format.h>

#include <cstdlib>
#include <iostream>

namespace fs = irods::experimental::filesystem;

static void populate(rodsPath_t *rodsPath, fs::path path);
static char rand_char();

class IntValue : public Catch::MatcherBase<int> {
    int m_value;
public:
    IntValue(int value) : m_value(value) {}
    virtual bool match(int const& i) const override {
        return i == m_value;
    }
    virtual std::string describe() const override {
        std::ostringstream ss;
        ss << "is equal to " << rodsErrorName(m_value, NULL) << " (" << m_value << ")";
        return ss.str();
    }
};

inline IntValue IntEquals(int value) {
    return IntValue(value);
}

/*
 * srcPath - sources
 * destPath - destination, not an array
 * targPath - final new paths
 */

TEST_CASE("resolveRodsTarget")
{
    auto& api_table = irods::get_client_api_table();
    auto& pck_table = irods::get_pack_table();
    init_api_table(api_table, pck_table);

    rodsEnv env;
    REQUIRE(getRodsEnv(&env) >= 0);

    const auto connection_count = 1;
    const auto refresh_time = 600;

    irods::connection_pool conn_pool{connection_count,
                                     env.rodsHost,
                                     env.rodsPort,
                                     env.rodsUserName,
                                     env.rodsZone,
                                     refresh_time};

    const auto conn = conn_pool.get_connection();

    using odstream          = irods::experimental::io::odstream;
    using default_transport = irods::experimental::io::client::default_transport;

    const int initRodsLogLevel = getRodsLogLevel();

    fs::path sandbox;
    {
        std::string sandbox_subcoll("unit_testing_sandbox-XXXXXX");
        size_t sandbox_subcol_len = sandbox_subcoll.length();
        do {
            for(int i = 1; i < 7; ++i) {
                sandbox_subcoll[sandbox_subcol_len - i] = rand_char();
            }

            sandbox = fs::path{env.rodsHome} / sandbox_subcoll;
        } while (fs::client::exists(conn, sandbox));
    }

    REQUIRE(fs::client::create_collection(conn, sandbox));
    REQUIRE(fs::client::exists(conn, sandbox));

    irods::at_scope_exit cleanup{[&conn, &sandbox, &initRodsLogLevel] {
        // restore log level
        rodsLogLevel(initRodsLogLevel);
        REQUIRE(fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash));
    }};

    {
    default_transport tp{conn};
        for(int i = 0; i < 5; ++i) {
            fs::path test_data = sandbox / fmt::format("test_data{}", i);
            odstream{tp, test_data};
            REQUIRE(fs::client::exists(conn, test_data));

            fs::path test_coll1 = sandbox / fmt::format("test_coll{}", i);
            REQUIRE(fs::client::create_collection(conn, test_coll1));
            REQUIRE(fs::client::exists(conn, test_coll1));

            fs::path test_coll2 = sandbox / fmt::format("test_coll{}", i + 5);
            REQUIRE(fs::client::create_collection(conn, test_coll2));
            REQUIRE(fs::client::exists(conn, test_coll2));

            for(int j = i; j < 5; ++j) {
                fs::path test_data2 = test_coll1 / fmt::format("test_data{}_{}", i, j);
                odstream{tp, test_data2};
                REQUIRE(fs::client::exists(conn, test_data2));

                fs::path test_data3 = test_coll2 / fmt::format("test_data{}", j);
                odstream{tp, test_data3};
                REQUIRE(fs::client::exists(conn, test_data3));
            }
        }
    }

    // no logging after initialization
    rodsLogLevel(LOG_SYS_WARNING);

    rodsPath_t rodsPath_dest{};

    SECTION("single_source")
    {
        rodsPath_t rodsPath_src{};
        rodsPath_t rodsPath_targ{};
        rodsPathInp_t rodsPathInp{
            .numSrc = 1,
            .srcPath = &rodsPath_src,
            .destPath = &rodsPath_dest,
            .targPath = &rodsPath_targ,
        };

        SECTION("source_collection")
        {
            fs::path src_coll = sandbox / "test_coll2";
            fs::path src_coll_cont1 = src_coll / "test_data2_3";
            REQUIRE(fs::client::exists(conn, src_coll));
            REQUIRE(fs::client::exists(conn, src_coll_cont1));

            populate(&rodsPath_src, src_coll);

            SECTION("destination_parent_exists")
            {
                fs::path dest_coll_parent = sandbox / "test_coll3";
                fs::path dest_coll = dest_coll_parent / "test_coll2";
                fs::path dest_coll_cont1 = dest_coll / "test_data2_3";

                REQUIRE(fs::client::exists(conn, dest_coll_parent));
                REQUIRE_FALSE(fs::client::exists(conn, dest_coll));
                REQUIRE_FALSE(fs::client::exists(conn, dest_coll_cont1));

                SECTION("destination_parent")
                {
                    populate(&rodsPath_dest, dest_coll_parent);

                    SECTION("PUT_OPR")
                    {
                        REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, PUT_OPR));

                        REQUIRE(dest_coll.string() == rodsPathInp.targPath->outPath);
                        REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                        REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                        REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                        REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                        REQUIRE(rodsPathInp.targPath[0].objType == COLL_OBJ_T);
                        REQUIRE(fs::client::exists(conn, dest_coll));
                        REQUIRE_FALSE(fs::client::exists(conn, dest_coll_cont1));
                    }

                    SECTION("COPY_DEST")
                    {
                        REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, COPY_DEST));

                        REQUIRE(dest_coll.string() == rodsPathInp.targPath->outPath);
                        REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                        REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                        REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                        REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                        REQUIRE(rodsPathInp.targPath[0].objType == COLL_OBJ_T);
                        REQUIRE_FALSE(fs::client::exists(conn, dest_coll));
                    }

                    SECTION("MOVE_OPR")
                    {
                        REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, MOVE_OPR));

                        REQUIRE(dest_coll.string() == rodsPathInp.targPath->outPath);
                        REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                        REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                        REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                        REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                        REQUIRE(rodsPathInp.targPath[0].objType == COLL_OBJ_T);
                        REQUIRE_FALSE(fs::client::exists(conn, dest_coll));
                    }

                    SECTION("RSYNC_OPR")
                    {
                        REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, RSYNC_OPR));

                        REQUIRE(dest_coll_parent.string() == rodsPathInp.targPath->outPath);
                        REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                        REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                        REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                        REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                        REQUIRE(rodsPathInp.targPath[0].objType == COLL_OBJ_T);
                        REQUIRE_FALSE(fs::client::exists(conn, dest_coll));
                    }
                }

                SECTION("destination_full")
                {
                    populate(&rodsPath_dest, dest_coll);

                    SECTION("PUT_OPR")
                    {
                        REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, PUT_OPR));

                        REQUIRE(dest_coll.string() == rodsPathInp.targPath->outPath);
                        REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                        REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                        REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                        REQUIRE(rodsPathInp.targPath[0].objType == COLL_OBJ_T);
                        REQUIRE(fs::client::exists(conn, dest_coll));
                        REQUIRE_FALSE(fs::client::exists(conn, dest_coll_cont1));
                    }

                    SECTION("COPY_DEST")
                    {
                        REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, COPY_DEST));

                        REQUIRE(dest_coll.string() == rodsPathInp.targPath->outPath);
                        REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                        REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                        REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                        REQUIRE(rodsPathInp.targPath[0].objType == COLL_OBJ_T);
                        REQUIRE_FALSE(fs::client::exists(conn, dest_coll));
                    }

                    SECTION("MOVE_OPR")
                    {
                        REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, MOVE_OPR));

                        REQUIRE(dest_coll.string() == rodsPathInp.targPath->outPath);
                        REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                        REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                        REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                        REQUIRE(rodsPathInp.targPath[0].objType == COLL_OBJ_T);
                        REQUIRE_FALSE(fs::client::exists(conn, dest_coll));
                    }

                    SECTION("RSYNC_OPR")
                    {
                        REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, RSYNC_OPR));

                        REQUIRE(dest_coll.string() == rodsPathInp.targPath->outPath);
                        REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                        REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                        REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                        REQUIRE(rodsPathInp.targPath[0].objType == COLL_OBJ_T);
                        REQUIRE(fs::client::exists(conn, dest_coll));
                        REQUIRE_FALSE(fs::client::exists(conn, dest_coll_cont1));
                    }
                }
            }

            SECTION("destination_parent_missing")
            {
                fs::path dest_coll_parent = sandbox / "test_coll_nonexist";
                fs::path dest_coll = dest_coll_parent / "test_coll2";

                REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent));
                REQUIRE_FALSE(fs::client::exists(conn, dest_coll));

                populate(&rodsPath_dest, dest_coll);

                SECTION("PUT_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, PUT_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent));
                }

                SECTION("COPY_DEST")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, COPY_DEST),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent));
                }

                SECTION("MOVE_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, MOVE_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent));
                }

                SECTION("RSYNC_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, RSYNC_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent));
                }
            }

            SECTION("destination_parent_parent_missing")
            {
                fs::path dest_coll_parent_parent = sandbox / "test_coll_nonexist";
                fs::path dest_coll_parent = dest_coll_parent_parent / "test_coll_nonexist2";
                fs::path dest_coll = dest_coll_parent / "test_coll2";

                REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent_parent));
                REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent));
                REQUIRE_FALSE(fs::client::exists(conn, dest_coll));

                populate(&rodsPath_dest, dest_coll);

                SECTION("PUT_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, PUT_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent_parent));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent));
                }

                SECTION("COPY_DEST")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, COPY_DEST),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent_parent));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent));
                }

                SECTION("MOVE_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, MOVE_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent_parent));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent));
                }

                SECTION("RSYNC_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, RSYNC_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent_parent));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent));
                }
            }
        }

        SECTION("source_dataobj")
        {
            fs::path src_data = sandbox / "test_data4";
            REQUIRE(fs::client::exists(conn, src_data));

            populate(&rodsPath_src, src_data);

            SECTION("destination_parent_exists")
            {
                fs::path dest_data_parent = sandbox / "test_coll3";
                fs::path dest_data = dest_data_parent / "test_data4";

                REQUIRE(fs::client::exists(conn, dest_data_parent));
                REQUIRE_FALSE(fs::client::exists(conn, dest_data));

                SECTION("destination_parent")
                {
                    populate(&rodsPath_dest, dest_data_parent);

                    SECTION("PUT_OPR")
                    {
                        REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, PUT_OPR));

                        REQUIRE(dest_data.string() == rodsPathInp.targPath->outPath);
                        REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                        REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                        REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                        REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                        REQUIRE(rodsPathInp.targPath[0].objType == DATA_OBJ_T);
                        REQUIRE_FALSE(fs::client::exists(conn, dest_data));
                    }

                    SECTION("COPY_DEST")
                    {
                        REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, COPY_DEST));

                        REQUIRE(dest_data.string() == rodsPathInp.targPath->outPath);
                        REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                        REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                        REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                        REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                        REQUIRE(rodsPathInp.targPath[0].objType == DATA_OBJ_T);
                        REQUIRE_FALSE(fs::client::exists(conn, dest_data));
                    }

                    SECTION("MOVE_OPR")
                    {
                        REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, MOVE_OPR));

                        REQUIRE(dest_data.string() == rodsPathInp.targPath->outPath);
                        REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                        REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                        REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                        REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                        REQUIRE(rodsPathInp.targPath[0].objType == DATA_OBJ_T);
                        REQUIRE_FALSE(fs::client::exists(conn, dest_data));
                    }

                    SECTION("RSYNC_OPR")
                    {
                        REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, RSYNC_OPR));

                        REQUIRE(dest_data.string() == rodsPathInp.targPath->outPath);
                        REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                        REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                        REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                        REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                        REQUIRE(rodsPathInp.targPath[0].objType == DATA_OBJ_T);
                        REQUIRE_FALSE(fs::client::exists(conn, dest_data));
                    }
                }

                SECTION("destination_full")
                {
                    populate(&rodsPath_dest, dest_data);

                    SECTION("PUT_OPR")
                    {
                        REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, PUT_OPR));

                        REQUIRE(dest_data.string() == rodsPathInp.targPath->outPath);
                        REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                        REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                        REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                        REQUIRE(rodsPathInp.targPath[0].objType == DATA_OBJ_T);
                        CHECK(rodsPathInp.targPath[0].objState == NOT_EXIST_ST);
                        REQUIRE_FALSE(fs::client::exists(conn, dest_data));
                    }

                    SECTION("COPY_DEST")
                    {
                        REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, COPY_DEST));

                        REQUIRE(dest_data.string() == rodsPathInp.targPath->outPath);
                        REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                        REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                        REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                        REQUIRE(rodsPathInp.targPath[0].objType == DATA_OBJ_T);
                        CHECK(rodsPathInp.targPath[0].objState == NOT_EXIST_ST);
                        REQUIRE_FALSE(fs::client::exists(conn, dest_data));
                    }

                    SECTION("MOVE_OPR")
                    {
                        REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, MOVE_OPR));

                        REQUIRE(dest_data.string() == rodsPathInp.targPath->outPath);
                        REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                        REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                        REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                        REQUIRE(rodsPathInp.targPath[0].objType == DATA_OBJ_T);
                        CHECK(rodsPathInp.targPath[0].objState == NOT_EXIST_ST);
                        REQUIRE_FALSE(fs::client::exists(conn, dest_data));
                    }

                    SECTION("RSYNC_OPR")
                    {
                        REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, RSYNC_OPR));

                        REQUIRE(dest_data.string() == rodsPathInp.targPath->outPath);
                        REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                        REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                        REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                        REQUIRE(rodsPathInp.targPath[0].objType == DATA_OBJ_T);
                        CHECK(rodsPathInp.targPath[0].objState == NOT_EXIST_ST);
                        REQUIRE_FALSE(fs::client::exists(conn, dest_data));
                    }
                }
            }

            SECTION("destination_parent_missing")
            {
                fs::path dest_data_parent = sandbox / "test_coll_nonexist";
                fs::path dest_data = dest_data_parent / "test_data4";

                REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent));
                REQUIRE_FALSE(fs::client::exists(conn, dest_data));

                populate(&rodsPath_dest, dest_data);

                SECTION("PUT_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, PUT_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent));
                }

                SECTION("COPY_DEST")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, COPY_DEST),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent));
                }

                SECTION("MOVE_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, MOVE_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent));
                }

                SECTION("RSYNC_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, RSYNC_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent));
                }
            }

            SECTION("destination_parent_parent_missing")
            {
                fs::path dest_data_parent_parent = sandbox / "test_coll_nonexist";
                fs::path dest_data_parent = dest_data_parent_parent / "test_coll_nonexist2";
                fs::path dest_data = dest_data_parent / "test_data4";

                REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent_parent));
                REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent));
                REQUIRE_FALSE(fs::client::exists(conn, dest_data));

                populate(&rodsPath_dest, dest_data);

                SECTION("PUT_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, PUT_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent_parent));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent));
                }

                SECTION("COPY_DEST")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, COPY_DEST),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent_parent));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent));
                }

                SECTION("MOVE_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, MOVE_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent_parent));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent));
                }

                SECTION("RSYNC_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, RSYNC_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent_parent));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent));
                }
            }
        }
    }

    SECTION("multi_source_2")
    {
        rodsPath_t rodsPath_src[2] = { rodsPath_t{}, rodsPath_t{} };
        rodsPath_t rodsPath_targ[2] = { rodsPath_t{}, rodsPath_t{} };
        rodsPathInp_t rodsPathInp{
            .numSrc = 2,
            .srcPath = rodsPath_src,
            .destPath = &rodsPath_dest,
            .targPath = rodsPath_targ,
        };

        SECTION("source_collection")
        {
            fs::path src_coll1 = sandbox / "test_coll1";
            fs::path src_coll2 = sandbox / "test_coll2";
            fs::path src_coll1_cont1 = src_coll1 / "test_data1_1";
            fs::path src_coll2_cont1 = src_coll2 / "test_data2_3";
            REQUIRE(fs::client::exists(conn, src_coll1));
            REQUIRE(fs::client::exists(conn, src_coll2));
            REQUIRE(fs::client::exists(conn, src_coll1_cont1));
            REQUIRE(fs::client::exists(conn, src_coll2_cont1));

            populate(&rodsPath_src[0], src_coll1);
            populate(&rodsPath_src[1], src_coll2);

            SECTION("destination_parent_exists")
            {
                fs::path dest_coll_parent = sandbox / "test_coll3";
                fs::path dest_coll1 = dest_coll_parent / "test_coll1";
                fs::path dest_coll2 = dest_coll_parent / "test_coll2";
                fs::path dest_coll1_cont1 = dest_coll1 / "test_data1_1";
                fs::path dest_coll2_cont1 = dest_coll2 / "test_data2_3";

                REQUIRE(fs::client::exists(conn, dest_coll_parent));
                REQUIRE_FALSE(fs::client::exists(conn, dest_coll1));
                REQUIRE_FALSE(fs::client::exists(conn, dest_coll2));
                REQUIRE_FALSE(fs::client::exists(conn, dest_coll1_cont1));
                REQUIRE_FALSE(fs::client::exists(conn, dest_coll2_cont1));

                populate(&rodsPath_dest, dest_coll_parent);

                SECTION("PUT_OPR")
                {
                    REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, PUT_OPR));

                    REQUIRE(dest_coll1.string() == rodsPathInp.targPath[0].outPath);
                    REQUIRE(dest_coll2.string() == rodsPathInp.targPath[1].outPath);
                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                    REQUIRE(rodsPathInp.targPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.targPath[1].objType == COLL_OBJ_T);
                    REQUIRE(fs::client::exists(conn, dest_coll1));
                    REQUIRE(fs::client::exists(conn, dest_coll2));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll1_cont1));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll2_cont1));
                }

                SECTION("COPY_DEST")
                {
                    REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, COPY_DEST));

                    REQUIRE(dest_coll1.string() == rodsPathInp.targPath[0].outPath);
                    REQUIRE(dest_coll2.string() == rodsPathInp.targPath[1].outPath);
                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                    REQUIRE(rodsPathInp.targPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.targPath[1].objType == COLL_OBJ_T);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll1));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll2));
                }

                SECTION("MOVE_OPR")
                {
                    REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, MOVE_OPR));

                    REQUIRE(dest_coll1.string() == rodsPathInp.targPath[0].outPath);
                    REQUIRE(dest_coll2.string() == rodsPathInp.targPath[1].outPath);
                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                    REQUIRE(rodsPathInp.targPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.targPath[1].objType == COLL_OBJ_T);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll1));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll2));
                }

                SECTION("RSYNC_OPR")
                {
                    REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, RSYNC_OPR));

                    REQUIRE(dest_coll1.string() == rodsPathInp.targPath[0].outPath);
                    REQUIRE(dest_coll2.string() == rodsPathInp.targPath[1].outPath);
                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                    REQUIRE(rodsPathInp.targPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.targPath[1].objType == COLL_OBJ_T);
                    REQUIRE(fs::client::exists(conn, dest_coll1));
                    REQUIRE(fs::client::exists(conn, dest_coll2));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll1_cont1));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll2_cont1));
                }
            }

            SECTION("destination_parent_missing")
            {
                fs::path dest_coll_parent = sandbox / "test_coll_nonexist";
                fs::path dest_coll1 = dest_coll_parent / "test_coll1";
                fs::path dest_coll2 = dest_coll_parent / "test_coll2";

                REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent));
                REQUIRE_FALSE(fs::client::exists(conn, dest_coll1));
                REQUIRE_FALSE(fs::client::exists(conn, dest_coll2));

                populate(&rodsPath_dest, dest_coll_parent);

                SECTION("PUT_OPR")
                {
                    REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, PUT_OPR));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                    REQUIRE(fs::client::exists(conn, dest_coll_parent));
                    REQUIRE(fs::client::exists(conn, dest_coll1));
                    REQUIRE(fs::client::exists(conn, dest_coll2));
                }

                SECTION("COPY_DEST")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, COPY_DEST),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent));
                }

                SECTION("MOVE_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, MOVE_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent));
                }

                SECTION("RSYNC_OPR")
                {
                    REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, RSYNC_OPR));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                    REQUIRE(fs::client::exists(conn, dest_coll_parent));
                    REQUIRE(fs::client::exists(conn, dest_coll1));
                    REQUIRE(fs::client::exists(conn, dest_coll2));
                }
            }

            SECTION("destination_parent_parent_missing")
            {
                fs::path dest_coll_parent_parent = sandbox / "test_coll_nonexist";
                fs::path dest_coll_parent = dest_coll_parent_parent / "test_coll_nonexist2";
                fs::path dest_coll1 = dest_coll_parent / "test_coll1";
                fs::path dest_coll2 = dest_coll_parent / "test_coll2";

                REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent_parent));
                REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent));
                REQUIRE_FALSE(fs::client::exists(conn, dest_coll1));
                REQUIRE_FALSE(fs::client::exists(conn, dest_coll2));

                populate(&rodsPath_dest, dest_coll_parent);

                SECTION("PUT_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, PUT_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    CHECK_FALSE(fs::client::exists(conn, dest_coll_parent_parent));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent));
                }

                SECTION("COPY_DEST")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, COPY_DEST),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    CHECK_FALSE(fs::client::exists(conn, dest_coll_parent_parent));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent));
                }

                SECTION("MOVE_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, MOVE_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    CHECK_FALSE(fs::client::exists(conn, dest_coll_parent_parent));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent));
                }

                SECTION("RSYNC_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, RSYNC_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    CHECK_FALSE(fs::client::exists(conn, dest_coll_parent_parent));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll_parent));
                }
            }
        }

        SECTION("source_dataobj")
        {
            fs::path src_data1 = sandbox / "test_data3";
            fs::path src_data2 = sandbox / "test_data4";
            REQUIRE(fs::client::exists(conn, src_data1));
            REQUIRE(fs::client::exists(conn, src_data2));

            populate(&rodsPath_src[0], src_data1);
            populate(&rodsPath_src[1], src_data2);

            SECTION("destination_parent_exists")
            {
                fs::path dest_data_parent = sandbox / "test_coll3";
                fs::path dest_data1 = dest_data_parent / "test_data3";
                fs::path dest_data2 = dest_data_parent / "test_data4";

                REQUIRE(fs::client::exists(conn, dest_data_parent));
                REQUIRE_FALSE(fs::client::exists(conn, dest_data1));
                REQUIRE_FALSE(fs::client::exists(conn, dest_data2));

                populate(&rodsPath_dest, dest_data_parent);

                SECTION("PUT_OPR")
                {
                    REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, PUT_OPR));

                    REQUIRE(dest_data1.string() == rodsPathInp.targPath[0].outPath);
                    REQUIRE(dest_data2.string() == rodsPathInp.targPath[1].outPath);
                    REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                    REQUIRE(rodsPathInp.targPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.targPath[1].objType == DATA_OBJ_T);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data1));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data2));
                }

                SECTION("COPY_DEST")
                {
                    REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, COPY_DEST));

                    REQUIRE(dest_data1.string() == rodsPathInp.targPath[0].outPath);
                    REQUIRE(dest_data2.string() == rodsPathInp.targPath[1].outPath);
                    REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                    REQUIRE(rodsPathInp.targPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.targPath[1].objType == DATA_OBJ_T);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data1));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data2));
                }

                SECTION("MOVE_OPR")
                {
                    REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, MOVE_OPR));

                    REQUIRE(dest_data1.string() == rodsPathInp.targPath[0].outPath);
                    REQUIRE(dest_data2.string() == rodsPathInp.targPath[1].outPath);
                    REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                    REQUIRE(rodsPathInp.targPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.targPath[1].objType == DATA_OBJ_T);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data1));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data2));
                }

                SECTION("RSYNC_OPR")
                {
                    REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, RSYNC_OPR));

                    REQUIRE(dest_data1.string() == rodsPathInp.targPath[0].outPath);
                    REQUIRE(dest_data2.string() == rodsPathInp.targPath[1].outPath);
                    REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                    REQUIRE(rodsPathInp.targPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.targPath[1].objType == DATA_OBJ_T);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data1));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data2));
                }
            }

            SECTION("destination_parent_missing")
            {
                fs::path dest_data_parent = sandbox / "test_coll_nonexist";
                fs::path dest_data1 = dest_data_parent / "test_data3";
                fs::path dest_data2 = dest_data_parent / "test_data4";

                REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent));
                REQUIRE_FALSE(fs::client::exists(conn, dest_data1));
                REQUIRE_FALSE(fs::client::exists(conn, dest_data2));

                populate(&rodsPath_dest, dest_data_parent);

                SECTION("PUT_OPR")
                {
                    REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, PUT_OPR));

                    REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                    REQUIRE(fs::client::exists(conn, dest_data_parent));
                    CHECK_FALSE(fs::client::exists(conn, dest_data1));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data2));
                }

                SECTION("COPY_DEST")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, COPY_DEST),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent));
                }

                SECTION("MOVE_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, MOVE_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent));
                }

                SECTION("RSYNC_OPR")
                {
                    REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, RSYNC_OPR));

                    REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                    REQUIRE(fs::client::exists(conn, dest_data_parent));
                    CHECK_FALSE(fs::client::exists(conn, dest_data1));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data2));
                }
            }

            SECTION("destination_parent_parent_missing")
            {
                fs::path dest_data_parent_parent = sandbox / "test_coll_nonexist";
                fs::path dest_data_parent = dest_data_parent_parent / "test_coll_nonexist2";
                fs::path dest_data1 = dest_data_parent / "test_data3";
                fs::path dest_data2 = dest_data_parent / "test_data4";

                REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent_parent));
                REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent));
                REQUIRE_FALSE(fs::client::exists(conn, dest_data1));
                REQUIRE_FALSE(fs::client::exists(conn, dest_data2));

                populate(&rodsPath_dest, dest_data_parent);

                SECTION("PUT_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, PUT_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    CHECK_FALSE(fs::client::exists(conn, dest_data_parent_parent));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent));
                }

                SECTION("COPY_DEST")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, COPY_DEST),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    CHECK_FALSE(fs::client::exists(conn, dest_data_parent_parent));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent));
                }

                SECTION("MOVE_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, MOVE_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    CHECK_FALSE(fs::client::exists(conn, dest_data_parent_parent));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent));
                }

                SECTION("RSYNC_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, RSYNC_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    CHECK_FALSE(fs::client::exists(conn, dest_data_parent_parent));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data_parent));
                }
            }
        }

        SECTION("source_mixed")
        {
            fs::path src_coll = sandbox / "test_coll2";
            fs::path src_data = sandbox / "test_data4";
            fs::path src_coll_cont1 = src_coll / "test_data2_3";
            REQUIRE(fs::client::exists(conn, src_coll));
            REQUIRE(fs::client::exists(conn, src_data));
            REQUIRE(fs::client::exists(conn, src_coll_cont1));

            populate(&rodsPath_src[0], src_coll);
            populate(&rodsPath_src[1], src_data);

            SECTION("destination_parent_exists")
            {
                fs::path dest_both_parent = sandbox / "test_coll3";
                fs::path dest_coll = dest_both_parent / "test_coll2";
                fs::path dest_data = dest_both_parent / "test_data4";
                fs::path dest_coll_cont1 = dest_coll / "test_data2_3";

                REQUIRE(fs::client::exists(conn, dest_both_parent));
                REQUIRE_FALSE(fs::client::exists(conn, dest_coll));
                REQUIRE_FALSE(fs::client::exists(conn, dest_data));
                REQUIRE_FALSE(fs::client::exists(conn, dest_coll_cont1));

                populate(&rodsPath_dest, dest_both_parent);

                SECTION("PUT_OPR")
                {
                    REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, PUT_OPR));

                    REQUIRE(dest_coll.string() == rodsPathInp.targPath[0].outPath);
                    REQUIRE(dest_data.string() == rodsPathInp.targPath[1].outPath);
                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                    REQUIRE(rodsPathInp.targPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.targPath[1].objType == DATA_OBJ_T);
                    CHECK(fs::client::exists(conn, dest_coll));
                    CHECK_FALSE(fs::client::exists(conn, dest_data));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll_cont1));
                }

                SECTION("COPY_DEST")
                {
                    REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, COPY_DEST));

                    REQUIRE(dest_coll.string() == rodsPathInp.targPath[0].outPath);
                    REQUIRE(dest_data.string() == rodsPathInp.targPath[1].outPath);
                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                    REQUIRE(rodsPathInp.targPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.targPath[1].objType == DATA_OBJ_T);
                    CHECK_FALSE(fs::client::exists(conn, dest_coll));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data));
                }

                SECTION("MOVE_OPR")
                {
                    REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, MOVE_OPR));

                    REQUIRE(dest_coll.string() == rodsPathInp.targPath[0].outPath);
                    REQUIRE(dest_data.string() == rodsPathInp.targPath[1].outPath);
                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                    REQUIRE(rodsPathInp.targPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.targPath[1].objType == DATA_OBJ_T);
                    CHECK_FALSE(fs::client::exists(conn, dest_coll));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data));
                }

                SECTION("RSYNC_OPR")
                {
                    REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, RSYNC_OPR));

                    REQUIRE(dest_coll.string() == rodsPathInp.targPath[0].outPath);
                    REQUIRE(dest_data.string() == rodsPathInp.targPath[1].outPath);
                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                    REQUIRE(rodsPathInp.targPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.targPath[1].objType == DATA_OBJ_T);
                    CHECK(fs::client::exists(conn, dest_coll));
                    CHECK_FALSE(fs::client::exists(conn, dest_data));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_coll_cont1));
                }
            }

            SECTION("destination_parent_missing")
            {
                fs::path dest_both_parent = sandbox / "test_coll_nonexist";
                fs::path dest_coll = dest_both_parent / "test_coll2";
                fs::path dest_data = dest_both_parent / "test_data4";

                REQUIRE_FALSE(fs::client::exists(conn, dest_both_parent));
                REQUIRE_FALSE(fs::client::exists(conn, dest_coll));
                REQUIRE_FALSE(fs::client::exists(conn, dest_data));

                populate(&rodsPath_dest, dest_both_parent);

                SECTION("PUT_OPR")
                {
                    REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, PUT_OPR));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                    REQUIRE(fs::client::exists(conn, dest_both_parent));
                    CHECK(fs::client::exists(conn, dest_coll));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data));
                }

                SECTION("COPY_DEST")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, COPY_DEST),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_both_parent));
                }

                SECTION("MOVE_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, MOVE_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    REQUIRE_FALSE(fs::client::exists(conn, dest_both_parent));
                }

                SECTION("RSYNC_OPR")
                {
                    REQUIRE_FALSE(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, RSYNC_OPR));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == EXIST_ST);
                    REQUIRE(fs::client::exists(conn, dest_both_parent));
                    CHECK(fs::client::exists(conn, dest_coll));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_data));
                }
            }

            SECTION("destination_parent_parent_missing")
            {
                fs::path dest_both_parent_parent = sandbox / "test_coll_nonexist";
                fs::path dest_both_parent = dest_both_parent_parent / "test_coll_nonexist2";
                fs::path dest_coll = dest_both_parent / "test_coll2";
                fs::path dest_data = dest_both_parent / "test_data4";

                REQUIRE_FALSE(fs::client::exists(conn, dest_both_parent_parent));
                REQUIRE_FALSE(fs::client::exists(conn, dest_both_parent));
                REQUIRE_FALSE(fs::client::exists(conn, dest_coll));
                REQUIRE_FALSE(fs::client::exists(conn, dest_data));

                populate(&rodsPath_dest, dest_both_parent);

                SECTION("PUT_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, PUT_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    CHECK_FALSE(fs::client::exists(conn, dest_both_parent_parent));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_both_parent));
                }

                SECTION("COPY_DEST")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, COPY_DEST),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    CHECK_FALSE(fs::client::exists(conn, dest_both_parent_parent));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_both_parent));
                }

                SECTION("MOVE_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, MOVE_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    CHECK_FALSE(fs::client::exists(conn, dest_both_parent_parent));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_both_parent));
                }

                SECTION("RSYNC_OPR")
                {
                    CHECK_THAT(resolveRodsTarget(static_cast<rcComm_t*>(conn), &rodsPathInp, RSYNC_OPR),
                                 IntEquals(CAT_UNKNOWN_COLLECTION) || IntEquals(USER_FILE_DOES_NOT_EXIST));

                    REQUIRE(rodsPathInp.srcPath[0].objType == COLL_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[0].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.srcPath[1].objType == DATA_OBJ_T);
                    REQUIRE(rodsPathInp.srcPath[1].objState == EXIST_ST);
                    REQUIRE(rodsPathInp.destPath->objType == UNKNOWN_OBJ_T);
                    REQUIRE(rodsPathInp.destPath->objState == NOT_EXIST_ST);
                    CHECK_FALSE(fs::client::exists(conn, dest_both_parent_parent));
                    REQUIRE_FALSE(fs::client::exists(conn, dest_both_parent));
                }
            }
        }
    }
}

inline static void populate(rodsPath_t *rodsPath, fs::path path) {
    const char *szPath = path.c_str();
    rstrcpy(rodsPath->inPath, szPath, MAX_NAME_LEN);
    rstrcpy(rodsPath->outPath, szPath, MAX_NAME_LEN);
}

static std::string chars("abcdefghijklmnopqrstuvwxyz1234567890");

#if CATCH_VERSION_MAJOR > 2 || (CATCH_VERSION_MAJOR == 2 && CATCH_VERSION_MINOR >= 10)
static Catch::Generators::RandomIntegerGenerator<int> index_dist(0, chars.size() - 1);
inline static char rand_char() {
    index_dist.next();
    return chars[index_dist.get()];
}
#else
#include <random>
static std::uniform_int_distribution<> index_dist(0, chars.size() - 1);
inline static char rand_char() {
    return chars[index_dist(Catch::rng())];
}
#endif
