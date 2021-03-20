#include "catch.hpp"

#include "client_connection.hpp"
#include "dataObjChksum.h"
#include "dataObjInpOut.h"
#include "dataObjPut.h"
#include "dstream.hpp"
#include "filesystem.hpp"
#include "get_file_descriptor_info.h"
#include "irods_at_scope_exit.hpp"
#include "irods_error_enum_matcher.hpp"
#include "irods_exception.hpp"
#include "objInfo.h"
#include "rcMisc.h"
#include "replica.hpp"
#include "replica_proxy.hpp"
#include "rodsClient.h"
#include "rodsErrorTable.h"
#include "transport/default_transport.hpp"

#include "boost/filesystem.hpp"
#include "fmt/format.h"

#include <cstring>
#include <iostream>
#include <thread>
#include <string>
#include <string_view>
#include <fstream>

using namespace std::chrono_literals;

// clang-format off
namespace fs      = irods::experimental::filesystem;
namespace replica = irods::experimental::replica;
// clang-format on

const std::string DEFAULT_RESOURCE_HIERARCHY = "demoResc";

auto create_empty_replica(const fs::path& _path) -> void;

TEST_CASE("open,read,write,close")
{
    try {
        load_client_api_plugins();

        irods::experimental::client_connection setup_conn;
        RcComm& setup_comm = static_cast<RcComm&>(setup_conn);

        rodsEnv env;
        _getRodsEnv(env);

        const auto sandbox = fs::path{env.rodsHome} / "test_rc_data_obj";
        if (!fs::client::exists(setup_comm, sandbox)) {
            REQUIRE(fs::client::create_collection(setup_comm, sandbox));
        }

        irods::at_scope_exit remove_sandbox{[&sandbox] {
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            REQUIRE(fs::client::remove_all(comm, sandbox, fs::remove_options::no_trash));
        }};

        const auto target_object = sandbox / "target_object";

        std::string_view path_str = target_object.c_str();

        std::string contents = "content!";

        SECTION("create,close")
        {
            create_empty_replica(target_object);
        }

        SECTION("create,no close")
        {
            {
                irods::experimental::client_connection conn;
                RcComm& comm = static_cast<RcComm&>(conn);

                dataObjInp_t open_inp{};
                std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", path_str.data());
                open_inp.openFlags = O_CREAT | O_TRUNC | O_WRONLY;
                const auto fd = rcDataObjOpen(&comm, &open_inp);
                REQUIRE(fd > 2);
                CHECK(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));

                // disconnect without close
            }

            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            // ensure all system metadata were updated properly
            const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
            CHECK(replica_info.mtime() == replica_info.ctime());
            CHECK(0 == static_cast<unsigned long>(replica_info.size()));
            CHECK(STALE_REPLICA == replica_info.replica_status());
        }

        SECTION("open for read,close")
        {
            create_empty_replica(target_object);

            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            dataObjInp_t open_inp{};
            std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", path_str.data());
            open_inp.openFlags = O_RDONLY;
            const auto fd = rcDataObjOpen(&comm, &open_inp);
            REQUIRE(fd > 2);
            CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));

            std::this_thread::sleep_for(2s);

            openedDataObjInp_t close_inp{};
            close_inp.l1descInx = fd;
            REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

            // ensure all system metadata were restored properly
            const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
            CHECK(replica_info.mtime() == replica_info.ctime());
            CHECK(0 == static_cast<unsigned long>(replica_info.size()));
            CHECK(GOOD_REPLICA == replica_info.replica_status());
        }

        SECTION("open for write,close")
        {
            create_empty_replica(target_object);

            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            dataObjInp_t open_inp{};
            std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", path_str.data());
            open_inp.openFlags = O_WRONLY;
            const auto fd = rcDataObjOpen(&comm, &open_inp);
            REQUIRE(fd > 2);
            CHECK(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));

            std::this_thread::sleep_for(2s);

            openedDataObjInp_t close_inp{};
            close_inp.l1descInx = fd;
            REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

            // ensure all system metadata were updated properly
            const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
            CHECK(replica_info.mtime() != replica_info.ctime());
            CHECK(0 == static_cast<unsigned long>(replica_info.size()));
            CHECK(GOOD_REPLICA == replica_info.replica_status());
        }

        SECTION("open for read,no close")
        {
            create_empty_replica(target_object);

            {
                irods::experimental::client_connection conn;
                RcComm& comm = static_cast<RcComm&>(conn);

                dataObjInp_t open_inp{};
                std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", path_str.data());
                open_inp.openFlags = O_RDONLY;
                const auto fd = rcDataObjOpen(&comm, &open_inp);
                REQUIRE(fd > 2);
                CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));

                // disconnect without close
            }

            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            // ensure all system metadata were updated properly
            const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
            CHECK(replica_info.mtime() == replica_info.ctime());
            CHECK(0 == static_cast<unsigned long>(replica_info.size()));
            CHECK(GOOD_REPLICA == replica_info.replica_status());
        }

        SECTION("open for write,no close")
        {
            create_empty_replica(target_object);

            {
                irods::experimental::client_connection conn;
                RcComm& comm = static_cast<RcComm&>(conn);

                dataObjInp_t open_inp{};
                std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", path_str.data());
                open_inp.openFlags = O_WRONLY;
                const auto fd = rcDataObjOpen(&comm, &open_inp);
                REQUIRE(fd > 2);
                CHECK(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));

                std::this_thread::sleep_for(2s);

                // disconnect without close
            }

            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            REQUIRE(STALE_REPLICA == replica::replica_status(comm, target_object, 0));

            dataObjInp_t open_inp{};
            std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", path_str.data());
            open_inp.openFlags = O_WRONLY;
            const auto fd = rcDataObjOpen(&comm, &open_inp);
            REQUIRE(fd > 2);
            CHECK(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));

            openedDataObjInp_t close_inp{};
            close_inp.l1descInx = fd;
            REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

            // ensure all system metadata were updated properly
            const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
            CHECK(replica_info.mtime() != replica_info.ctime());
            CHECK(0 == static_cast<unsigned long>(replica_info.size()));
            CHECK(STALE_REPLICA == replica_info.replica_status());
        }

        SECTION("create,no close,open for write,close")
        {
            // create,no close
            {
                irods::experimental::client_connection conn;
                RcComm& comm = static_cast<RcComm&>(conn);

                dataObjInp_t open_inp{};
                std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", path_str.data());
                open_inp.openFlags = O_CREAT | O_TRUNC | O_WRONLY;
                const auto fd = rcDataObjOpen(&comm, &open_inp);
                REQUIRE(fd > 2);
                CHECK(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));

                // disconnect without close
            }

            {
                irods::experimental::client_connection conn;
                RcComm& comm = static_cast<RcComm&>(conn);

                // ensure all system metadata were updated properly
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
                CHECK(replica_info.mtime() == replica_info.ctime());
                CHECK(0 == static_cast<unsigned long>(replica_info.size()));
                CHECK(STALE_REPLICA == replica_info.replica_status());
            }

            // open for write, close
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            dataObjInp_t open_inp{};
            std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", path_str.data());
            open_inp.openFlags = O_WRONLY;
            const auto fd = rcDataObjOpen(&comm, &open_inp);
            REQUIRE(fd > 2);
            CHECK(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));

            std::this_thread::sleep_for(2s);

            openedDataObjInp_t close_inp{};
            close_inp.l1descInx = fd;
            REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

            // ensure all system metadata were updated properly
            const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
            CHECK(replica_info.mtime() != replica_info.ctime());
            CHECK(0 == static_cast<unsigned long>(replica_info.size()));
            CHECK(STALE_REPLICA == replica_info.replica_status());
        }

        SECTION("create,write,close")
        {
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            dataObjInp_t open_inp{};
            std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", path_str.data());
            open_inp.openFlags = O_CREAT | O_TRUNC | O_WRONLY;
            const auto fd = rcDataObjOpen(&comm, &open_inp);
            REQUIRE(fd > 2);
            CHECK(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));

            bytesBuf_t write_bbuf{};
            write_bbuf.buf = static_cast<void*>(contents.data());
            write_bbuf.len = contents.size() + 1;

            openedDataObjInp_t write_inp{};
            write_inp.l1descInx = fd;
            write_inp.len = write_bbuf.len;
            const auto bytes_written = rcDataObjWrite(&comm, &write_inp, &write_bbuf);
            CHECK(write_bbuf.len == bytes_written);

            openedDataObjInp_t close_inp{};
            close_inp.l1descInx = fd;
            close_inp.bytesWritten = bytes_written;
            REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

            // ensure all system metadata were updated properly
            const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
            CHECK(replica_info.mtime() == replica_info.ctime());
            CHECK(contents.size() + 1 == static_cast<unsigned long>(replica_info.size()));
            CHECK(GOOD_REPLICA == replica_info.replica_status());
        }

        SECTION("create,write,no close")
        {
            {
                irods::experimental::client_connection conn;
                RcComm& comm = static_cast<RcComm&>(conn);

                dataObjInp_t open_inp{};
                std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", path_str.data());
                open_inp.openFlags = O_CREAT | O_TRUNC | O_WRONLY;
                const auto fd = rcDataObjOpen(&comm, &open_inp);
                REQUIRE(fd > 2);
                CHECK(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));

                bytesBuf_t write_bbuf{};
                write_bbuf.buf = static_cast<void*>(contents.data());
                write_bbuf.len = contents.size() + 1;

                openedDataObjInp_t write_inp{};
                write_inp.l1descInx = fd;
                write_inp.len = write_bbuf.len;
                const auto bytes_written = rcDataObjWrite(&comm, &write_inp, &write_bbuf);
                CHECK(write_bbuf.len == bytes_written);

                // disconnect without close
            }

            // ensure all system metadata were updated properly
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
            CHECK(replica_info.mtime() == replica_info.ctime());
            CHECK(contents.size() + 1 == static_cast<unsigned long>(replica_info.size()));
            CHECK(STALE_REPLICA == replica_info.replica_status());
        }

        SECTION("open for read x2,close x2")
        {
            create_empty_replica(target_object);

            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            dataObjInp_t open_inp_1{};
            std::snprintf(open_inp_1.objPath, sizeof(open_inp_1.objPath), "%s", path_str.data());
            open_inp_1.openFlags = O_RDONLY;
            const auto fd_1 = rcDataObjOpen(&comm, &open_inp_1);
            REQUIRE(fd_1 > 2);
            CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));

            dataObjInp_t open_inp_2{};
            std::snprintf(open_inp_2.objPath, sizeof(open_inp_2.objPath), "%s", path_str.data());
            open_inp_2.openFlags = O_RDONLY;
            const auto fd_2 = rcDataObjOpen(&comm, &open_inp_2);
            REQUIRE(fd_2 > 2);
            CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));

            REQUIRE(fd_2 != fd_1);

            {
                openedDataObjInp_t close_inp{};
                close_inp.l1descInx = fd_1;
                REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

                // ensure all system metadata were restored properly
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
                CHECK(replica_info.mtime() == replica_info.ctime());
                CHECK(0 == static_cast<unsigned long>(replica_info.size()));
                CHECK(GOOD_REPLICA == replica_info.replica_status());
            }

            {
                openedDataObjInp_t close_inp{};
                close_inp.l1descInx = fd_2;
                REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

                // ensure all system metadata were restored properly
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
                CHECK(replica_info.mtime() == replica_info.ctime());
                CHECK(0 == static_cast<unsigned long>(replica_info.size()));
                CHECK(GOOD_REPLICA == replica_info.replica_status());
            }
        }

        SECTION("open for write,open for read,close for write,close for read")
        {
            create_empty_replica(target_object);

            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            // open for write
            dataObjInp_t open_inp_1{};
            std::snprintf(open_inp_1.objPath, sizeof(open_inp_1.objPath), "%s", path_str.data());
            open_inp_1.openFlags = O_WRONLY;
            const auto fd_1 = rcDataObjOpen(&comm, &open_inp_1);
            REQUIRE(fd_1 > 2);
            REQUIRE(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));

            // open for read without pre-determeined resource hierarchy (i.e. request vote)
            {
                dataObjInp_t open_inp_2{};
                std::snprintf(open_inp_2.objPath, sizeof(open_inp_2.objPath), "%s", path_str.data());
                open_inp_2.openFlags = O_RDONLY;
                REQUIRE(HIERARCHY_ERROR == rcDataObjOpen(&comm, &open_inp_2));
                REQUIRE(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));
            }

            // open for read with pre-determined resource hierarchy
            {
                dataObjInp_t open_inp_2{};
                auto cond_input_2 = irods::experimental::make_key_value_proxy(open_inp_2.condInput);
                cond_input_2[RESC_HIER_STR_KW] = DEFAULT_RESOURCE_HIERARCHY;
                std::snprintf(open_inp_2.objPath, sizeof(open_inp_2.objPath), "%s", path_str.data());
                open_inp_2.openFlags = O_RDONLY;
                REQUIRE(USER_INTERMEDIATE_REPLICA_ACCESS == rcDataObjOpen(&comm, &open_inp_2));
                REQUIRE(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));
            }

            // get replica token to properly open the replica
            const nlohmann::json input{{"fd", fd_1}};
            char* json_output{};
            REQUIRE(rc_get_file_descriptor_info(&comm, input.dump().c_str(), &json_output) == 0);
            const auto out = nlohmann::json::parse(json_output);
            const std::string token = out.at("replica_token");

            // use replica access token and pre-determined resource hierarchy
            dataObjInp_t open_inp_2{};
            auto cond_input_2 = irods::experimental::make_key_value_proxy(open_inp_2.condInput);
            cond_input_2[RESC_HIER_STR_KW] = DEFAULT_RESOURCE_HIERARCHY;
            cond_input_2[REPLICA_TOKEN_KW] = token;
            std::snprintf(open_inp_2.objPath, sizeof(open_inp_2.objPath), "%s", path_str.data());
            open_inp_2.openFlags = O_RDONLY;
            const auto fd_2 = rcDataObjOpen(&comm, &open_inp_2);
            REQUIRE(fd_2 > 2);
            REQUIRE(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));

            REQUIRE(fd_2 != fd_1);

            std::this_thread::sleep_for(2s);

            // close the file descriptor for write
            {
                openedDataObjInp_t close_inp{};
                close_inp.l1descInx = fd_1;
                REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

                // ensure all system metadata were restored properly
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
                CHECK(replica_info.mtime() != replica_info.ctime());
                CHECK(0 == static_cast<unsigned long>(replica_info.size()));
                CHECK(GOOD_REPLICA == replica_info.replica_status());
            }

            // close the file descriptor for read
            {
                openedDataObjInp_t close_inp{};
                close_inp.l1descInx = fd_2;
                REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

                // ensure all system metadata were restored properly
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
                CHECK(0 == static_cast<unsigned long>(replica_info.size()));
                CHECK(GOOD_REPLICA == replica_info.replica_status());
            }
        }

        SECTION("open for write,open for read,close for read,close for write")
        {
            create_empty_replica(target_object);

            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            // open for write
            dataObjInp_t open_inp_1{};
            std::snprintf(open_inp_1.objPath, sizeof(open_inp_1.objPath), "%s", path_str.data());
            open_inp_1.openFlags = O_WRONLY;
            const auto fd_1 = rcDataObjOpen(&comm, &open_inp_1);
            REQUIRE(fd_1 > 2);
            REQUIRE(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));

            // open for read without pre-determeined resource hierarchy (i.e. request vote)
            {
                dataObjInp_t open_inp_2{};
                std::snprintf(open_inp_2.objPath, sizeof(open_inp_2.objPath), "%s", path_str.data());
                open_inp_2.openFlags = O_RDONLY;
                REQUIRE(HIERARCHY_ERROR == rcDataObjOpen(&comm, &open_inp_2));
                REQUIRE(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));
            }

            // open for read with pre-determined resource hierarchy
            {
                dataObjInp_t open_inp_2{};
                auto cond_input_2 = irods::experimental::make_key_value_proxy(open_inp_2.condInput);
                cond_input_2[RESC_HIER_STR_KW] = DEFAULT_RESOURCE_HIERARCHY;
                std::snprintf(open_inp_2.objPath, sizeof(open_inp_2.objPath), "%s", path_str.data());
                open_inp_2.openFlags = O_RDONLY;
                REQUIRE(USER_INTERMEDIATE_REPLICA_ACCESS == rcDataObjOpen(&comm, &open_inp_2));
                REQUIRE(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));
            }

            // get replica token to properly open the replica
            const nlohmann::json input{{"fd", fd_1}};
            char* json_output{};
            REQUIRE(rc_get_file_descriptor_info(&comm, input.dump().c_str(), &json_output) == 0);
            const auto out = nlohmann::json::parse(json_output);
            const std::string token = out.at("replica_token");

            // use replica access token and pre-determined resource hierarchy
            dataObjInp_t open_inp_2{};
            auto cond_input_2 = irods::experimental::make_key_value_proxy(open_inp_2.condInput);
            cond_input_2[RESC_HIER_STR_KW] = DEFAULT_RESOURCE_HIERARCHY;
            cond_input_2[REPLICA_TOKEN_KW] = token;
            std::snprintf(open_inp_2.objPath, sizeof(open_inp_2.objPath), "%s", path_str.data());
            open_inp_2.openFlags = O_RDONLY;
            const auto fd_2 = rcDataObjOpen(&comm, &open_inp_2);
            REQUIRE(fd_2 > 2);
            REQUIRE(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));

            REQUIRE(fd_2 != fd_1);

            std::this_thread::sleep_for(2s);

            // close the file descriptor for read
            {
                openedDataObjInp_t close_inp{};
                close_inp.l1descInx = fd_2;
                REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

                // ensure all system metadata were restored properly
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
                CHECK(0 == static_cast<unsigned long>(replica_info.size()));
                REQUIRE(INTERMEDIATE_REPLICA == replica_info.replica_status());
            }

            // close the file descriptor for write
            {
                openedDataObjInp_t close_inp{};
                close_inp.l1descInx = fd_1;
                REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

                // ensure all system metadata were restored properly
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
                CHECK(replica_info.mtime() != replica_info.ctime());
                CHECK(0 == static_cast<unsigned long>(replica_info.size()));
                REQUIRE(GOOD_REPLICA == replica_info.replica_status());
            }
        }

        SECTION("open for read,open for write,close x2")
        {
            create_empty_replica(target_object);

            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            dataObjInp_t open_inp_1{};
            std::snprintf(open_inp_1.objPath, sizeof(open_inp_1.objPath), "%s", path_str.data());
            open_inp_1.openFlags = O_RDONLY;
            const auto fd_1 = rcDataObjOpen(&comm, &open_inp_1);
            REQUIRE(fd_1 > 2);
            REQUIRE(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));

            dataObjInp_t open_inp_2{};
            std::snprintf(open_inp_2.objPath, sizeof(open_inp_2.objPath), "%s", path_str.data());
            open_inp_2.openFlags = O_WRONLY;
            const auto fd_2 = rcDataObjOpen(&comm, &open_inp_2);
            REQUIRE(fd_2 > 2);
            REQUIRE(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));

            REQUIRE(fd_2 != fd_1);

            std::this_thread::sleep_for(2s);

            {
                openedDataObjInp_t close_inp{};
                close_inp.l1descInx = fd_1;
                REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

                // ensure all system metadata were restored properly
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
                CHECK(replica_info.mtime() == replica_info.ctime());
                CHECK(0 == static_cast<unsigned long>(replica_info.size()));
                CHECK(INTERMEDIATE_REPLICA == replica_info.replica_status());
            }

            {
                openedDataObjInp_t close_inp{};
                close_inp.l1descInx = fd_2;
                REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

                // ensure all system metadata were restored properly
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
                CHECK(replica_info.mtime() != replica_info.ctime());
                CHECK(0 == static_cast<unsigned long>(replica_info.size()));
                CHECK(GOOD_REPLICA == replica_info.replica_status());
            }
        }
    }
    catch (const irods::exception& e) {
        std::cout << e.client_display_what();
    }
    catch (const std::exception& e) {
        std::cout << e.what();
    }
} // open,read,write,close

TEST_CASE("rxDataObjChksum")
{
    namespace ix = irods::experimental;
    namespace io = irods::experimental::io;
    namespace fs = irods::experimental::filesystem;

    load_client_api_plugins();

    ix::client_connection conn;
    REQUIRE(conn);

    rodsEnv env;
    _getRodsEnv(env);

    const auto sandbox = fs::path{env.rodsHome} / "unit_testing_sandbox";

    if (!fs::client::exists(conn, sandbox)) {
        REQUIRE(fs::client::create_collection(conn, sandbox));
    }

    irods::at_scope_exit remove_sandbox{[&conn, &sandbox] {
        REQUIRE(fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash));
    }};

    SECTION("incompatible parameters")
    {
        const auto data_object = sandbox / "data_object.txt";

        {
            io::client::native_transport tp{conn};
            io::odstream{tp, data_object} << "some important data.";
        }

        DataObjInp input{};
        std::strcpy(input.objPath, data_object.c_str());
        ix::key_value_proxy kvp{input.condInput};
        irods::at_scope_exit free_memory{[&kvp] { kvp.clear(); }};

        char* ignore_checksum{};

        SECTION("replica number flag and resource name flag")
        {
            kvp[REPL_NUM_KW] = "0";
            kvp[RESC_NAME_KW] = env.rodsDefResource;
            REQUIRE(rcDataObjChksum(static_cast<RcComm*>(conn), &input, &ignore_checksum) == USER_INCOMPATIBLE_PARAMS);
        }

        SECTION("all replicas flag and replica number flag")
        {
            kvp[CHKSUM_ALL_KW] = "";
            kvp[REPL_NUM_KW] = "0";
            REQUIRE(rcDataObjChksum(static_cast<RcComm*>(conn), &input, &ignore_checksum) == USER_INCOMPATIBLE_PARAMS);
        }

        SECTION("all replicas flag and resource name flag")
        {
            kvp[CHKSUM_ALL_KW] = "";
            kvp[RESC_NAME_KW] = env.rodsDefResource;
            REQUIRE(rcDataObjChksum(static_cast<RcComm*>(conn), &input, &ignore_checksum) == USER_INCOMPATIBLE_PARAMS);
        }

        SECTION("verification flag and force flag")
        {
            kvp[VERIFY_CHKSUM_KW] = "";
            kvp[FORCE_CHKSUM_KW] = "";
            REQUIRE(rcDataObjChksum(static_cast<RcComm*>(conn), &input, &ignore_checksum) == USER_INCOMPATIBLE_PARAMS);
        }

        SECTION("verification flag, all flag, and replica number flag")
        {
            kvp[VERIFY_CHKSUM_KW] = "";
            kvp[CHKSUM_ALL_KW] = "";
            kvp[REPL_NUM_KW] = "0";
            REQUIRE(rcDataObjChksum(static_cast<RcComm*>(conn), &input, &ignore_checksum) == USER_INCOMPATIBLE_PARAMS);
        }

        SECTION("verification flag, all flag, and resource name flag")
        {
            kvp[VERIFY_CHKSUM_KW] = "";
            kvp[CHKSUM_ALL_KW] = "";
            kvp[RESC_NAME_KW] = env.rodsDefResource;
            REQUIRE(rcDataObjChksum(static_cast<RcComm*>(conn), &input, &ignore_checksum) == USER_INCOMPATIBLE_PARAMS);
        }
    } // incompatible parameters
} // rxDataObjChksum

TEST_CASE("rxDataObjPut")
{
    rodsEnv env;
    _getRodsEnv(env);

    irods::experimental::client_connection conn;
    const auto sandbox = fs::path{env.rodsHome} / "unit_testing_sandbox";

    if (!fs::client::exists(conn, sandbox)) {
        REQUIRE(fs::client::create_collection(conn, sandbox));
    }

    irods::at_scope_exit remove_sandbox{[&conn, &sandbox] {
        REQUIRE(fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash));
    }};

    SECTION("#5400: VERIFY_CHKSUM_KW is always honored")
    {
        DataObjInp put_input{};
        put_input.openFlags = O_WRONLY;
        put_input.createMode = 0600;
        addKeyVal(&put_input.condInput, DEF_RESC_NAME_KW, env.rodsDefResource);

        const auto logical_path = sandbox / "eels.txt";
        std::strcpy(put_input.objPath, logical_path.c_str());

        // Set a bad checksum so that the "put" operation fails.
        const char file_contents[] = "My hovercraft is full of eels";
        addKeyVal(&put_input.condInput, VERIFY_CHKSUM_KW, file_contents);

        // Put a file into iRODS and trigger an error.
        const auto local_file = boost::filesystem::temp_directory_path() / "eels.txt";
        { std::ofstream{local_file.c_str()} << file_contents; }
        REQUIRE(rcDataObjPut(static_cast<RcComm*>(conn), &put_input, const_cast<char*>(local_file.c_str())) == USER_CHKSUM_MISMATCH);

        CHECK(replica::replica_status<RcComm>(conn, logical_path, env.rodsDefResource) == STALE_REPLICA);
        CHECK(replica::replica_size<RcComm>(conn, logical_path, env.rodsDefResource) == 0);

        // Show that the computed checksum matches that of an empty string because the data size in the
        // catalog is zero.
        const std::string_view empty_string_checksum = "sha2:47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=";
        CHECK(replica::replica_checksum<RcComm>(conn, logical_path, env.rodsDefResource) == empty_string_checksum);

        // Show that overwriting the replica still results in a checksum error and that all information
        // about the replica remains unchanged.
        addKeyVal(&put_input.condInput, FORCE_FLAG_KW, "");
        REQUIRE(rcDataObjPut(static_cast<RcComm*>(conn), &put_input, const_cast<char*>(local_file.c_str())) == USER_CHKSUM_MISMATCH);

        CHECK(replica::replica_status<RcComm>(conn, logical_path, env.rodsDefResource) == STALE_REPLICA);
        CHECK(replica::replica_size<RcComm>(conn, logical_path, env.rodsDefResource) == 0);
        CHECK(replica::replica_checksum<RcComm>(conn, logical_path, env.rodsDefResource) == empty_string_checksum);

        // Overwrite the replica and include a good checksum.
        // Show that the information about the replica is correct.
        const std::string_view good_checksum = "sha2:XL7wWccBvgYxXqmzjM4SYIGoTiocxaycQ9uzweaphcQ=";
        addKeyVal(&put_input.condInput, VERIFY_CHKSUM_KW, good_checksum.data());
        REQUIRE(rcDataObjPut(static_cast<RcComm*>(conn), &put_input, const_cast<char*>(local_file.c_str())) == 0);

        CHECK(replica::replica_status<RcComm>(conn, logical_path, env.rodsDefResource) == GOOD_REPLICA);
        CHECK(replica::replica_size<RcComm>(conn, logical_path, env.rodsDefResource) == std::strlen(file_contents));
        CHECK(replica::replica_checksum<RcComm>(conn, logical_path, env.rodsDefResource) == good_checksum);

        // Overwrite the replica again, but trigger a checksum mismatch error.
        addKeyVal(&put_input.condInput, VERIFY_CHKSUM_KW, file_contents);
        REQUIRE(rcDataObjPut(static_cast<RcComm*>(conn), &put_input, const_cast<char*>(local_file.c_str())) == USER_CHKSUM_MISMATCH);

        CHECK(replica::replica_status<RcComm>(conn, logical_path, env.rodsDefResource) == STALE_REPLICA);
        CHECK(replica::replica_size<RcComm>(conn, logical_path, env.rodsDefResource) == 0);

        // Show that the checksum in the catalog remains unchanged due to the checksum mismatch error.
        CHECK(replica::replica_checksum<RcComm>(conn, logical_path, env.rodsDefResource) == good_checksum);

        // Show that recalculating the checksum results in the empty string checksum because the checksum
        // API honors the data size in the catalog and not the physical object's size in storage.
        constexpr auto recalculate = replica::verification_calculation::always;
        CHECK(replica::replica_checksum<RcComm>(conn, logical_path, env.rodsDefResource, recalculate) == empty_string_checksum);
    }
} // rxDataObjPut

auto create_empty_replica(const fs::path& _path) -> void
{
    std::string_view path_str = _path.c_str();

    irods::experimental::client_connection conn;
    RcComm& comm = static_cast<RcComm&>(conn);

    dataObjInp_t open_inp{};
    std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", path_str.data());
    open_inp.openFlags = O_CREAT | O_TRUNC | O_WRONLY;
    const auto fd = rcDataObjOpen(&comm, &open_inp);
    REQUIRE(fd > 2);
    CHECK(INTERMEDIATE_REPLICA == replica::replica_status(comm, _path, 0));

    openedDataObjInp_t close_inp{};
    close_inp.l1descInx = fd;
    REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

    // ensure all system metadata were updated properly
    const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, _path, 0);
    CHECK(replica_info.mtime() == replica_info.ctime());
    CHECK(0 == static_cast<unsigned long>(replica_info.size()));
    REQUIRE(GOOD_REPLICA == replica::replica_status(comm, _path, 0));
} // create_empty_replica

