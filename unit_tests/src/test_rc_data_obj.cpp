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
#include "unit_test_utils.hpp"

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

        SECTION("#5496: partial overwrite erases checksum")
        {
            irods::experimental::client_connection conn;
            REQUIRE(conn);

            const std::string_view contents = "My hovercraft is full of eels";

            // Create a new data object.
            {
                irods::experimental::io::client::native_transport tp{conn};
                irods::experimental::io::odstream{tp, target_object} << contents;
            }

            // Show that the replica is in a good state.
            CHECK(replica::replica_status<RcComm>(conn, target_object, 0) == GOOD_REPLICA);
            CHECK(replica::replica_size<RcComm>(conn, target_object, 0) == contents.size());

            // Checksum the replica.
            std::string_view expected_checksum = "sha2:XL7wWccBvgYxXqmzjM4SYIGoTiocxaycQ9uzweaphcQ=";
            CHECK(replica::replica_checksum<RcComm>(conn, target_object, 0) == expected_checksum);

            // Open the replica again and overwrite part of it.
            dataObjInp_t open_inp{};
            std::strcpy(open_inp.objPath, target_object.c_str());
            open_inp.openFlags = O_CREAT | O_WRONLY;
            auto* conn_ptr = static_cast<RcComm*>(conn);
            const auto fd = rcDataObjOpen(conn_ptr, &open_inp);
            REQUIRE(fd > 2);

            const std::string_view new_data = "ABC";
            bytesBuf_t write_bbuf{};
            write_bbuf.buf = const_cast<char*>(new_data.data());
            write_bbuf.len = new_data.size() + 1;

            openedDataObjInp_t write_inp{};
            write_inp.l1descInx = fd;
            write_inp.len = write_bbuf.len;
            const auto bytes_written = rcDataObjWrite(conn_ptr, &write_inp, &write_bbuf);
            CHECK(write_bbuf.len == bytes_written);

            openedDataObjInp_t close_inp{};
            close_inp.l1descInx = fd;
            close_inp.bytesWritten = bytes_written;
            REQUIRE(rcDataObjClose(conn_ptr, &close_inp) >= 0);

            // Show that the replica is still in a good state and its checksum has been erased.
            const auto [rp, lm] = replica::make_replica_proxy<RcComm>(conn, target_object, 0);
            CHECK(rp.replica_status() == GOOD_REPLICA);
            CHECK(rp.size() == contents.size());
            REQUIRE(rp.checksum().empty());
        }

        SECTION("#5496: agent tear down and checksums")
        {
            irods::experimental::client_connection conn;
            REQUIRE(conn);

            const std::string_view contents = "My hovercraft is full of eels";

            // Create a new data object.
            {
                irods::experimental::io::client::native_transport tp{conn};
                irods::experimental::io::odstream{tp, target_object} << contents;
            }

            // Show that the replica is in a good state.
            CHECK(replica::replica_status<RcComm>(conn, target_object, 0) == GOOD_REPLICA);
            CHECK(replica::replica_size<RcComm>(conn, target_object, 0) == contents.size());

            // Checksum the replica.
            const std::string_view expected_checksum = "sha2:XL7wWccBvgYxXqmzjM4SYIGoTiocxaycQ9uzweaphcQ=";
            CHECK(replica::replica_checksum<RcComm>(conn, target_object, 0) == expected_checksum);

            // Spawn a new agent.
            conn.disconnect();
            conn.connect();
            REQUIRE(conn);

            // Overwrite part of the replica so that the agent can decide how to handle
            // the checksum on tear down.
            auto* conn_ptr = static_cast<RcComm*>(conn);
            REQUIRE(conn_ptr);

            dataObjInp_t open_inp{};
            std::strcpy(open_inp.objPath, target_object.c_str());
            open_inp.openFlags = O_CREAT | O_WRONLY;
            const auto fd = rcDataObjOpen(conn_ptr, &open_inp);
            REQUIRE(fd > 2);
            CHECK(INTERMEDIATE_REPLICA == replica::replica_status<RcComm>(conn, target_object, 0));

            SECTION("no bytes written, agent erases checksum")
            {
                // Disconnect without closing the replica so that the agent cleans up behind us.
                conn.disconnect();
                REQUIRE_FALSE(conn);

                // Give the agent a few seconds to clean up and shutdown.
                std::this_thread::sleep_for(3s);

                conn.connect();
                REQUIRE(conn);

                // Show that the replica is still in a good state and its checksum has been erased.
                const auto [rp, lm] = replica::make_replica_proxy<RcComm>(conn, target_object, 0);
                CHECK(rp.replica_status() == STALE_REPLICA);
                CHECK(rp.size() == contents.size());
                REQUIRE(rp.checksum().empty());
            }

            SECTION("bytes are written, agent erases checksum")
            {
                const std::string_view new_data = "ABC";
                bytesBuf_t write_bbuf{};
                write_bbuf.buf = const_cast<char*>(new_data.data());
                write_bbuf.len = new_data.size();

                openedDataObjInp_t write_inp{};
                write_inp.l1descInx = fd;
                write_inp.len = write_bbuf.len;
                const auto bytes_written = rcDataObjWrite(conn_ptr, &write_inp, &write_bbuf);
                CHECK(write_bbuf.len == bytes_written);

                // Disconnect without closing the replica so that the agent cleans up behind us.
                conn.disconnect();
                REQUIRE_FALSE(conn);

                // Give the agent a few seconds to clean up and shutdown.
                std::this_thread::sleep_for(3s);

                conn.connect();
                REQUIRE(conn);

                // Show that the replica is still in a good state and its checksum has been erased.
                const auto [rp, lm] = replica::make_replica_proxy<RcComm>(conn, target_object, 0);
                CHECK(rp.replica_status() == STALE_REPLICA);
                CHECK(rp.size() == contents.size());
                REQUIRE(rp.checksum().empty());
            }
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

                std::this_thread::sleep_for(2s);

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
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            unit_test_utils::create_empty_replica(comm, target_object);

            std::this_thread::sleep_for(2s);

            dataObjInp_t open_inp{};
            std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", path_str.data());
            open_inp.openFlags = O_RDONLY;
            const auto fd = rcDataObjOpen(&comm, &open_inp);
            REQUIRE(fd > 2);
            CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));

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
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            unit_test_utils::create_empty_replica(comm, target_object);

            std::this_thread::sleep_for(2s);

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
            CHECK(replica_info.mtime() == replica_info.ctime());
            CHECK(0 == static_cast<unsigned long>(replica_info.size()));
            CHECK(GOOD_REPLICA == replica_info.replica_status());
        }

        SECTION("open for read,no close")
        {
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            unit_test_utils::create_empty_replica(comm, target_object);

            std::this_thread::sleep_for(2s);

            {
                irods::experimental::client_connection conn2;
                RcComm& comm2 = static_cast<RcComm&>(conn2);

                dataObjInp_t open_inp{};
                std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", path_str.data());
                open_inp.openFlags = O_RDONLY;
                const auto fd = rcDataObjOpen(&comm2, &open_inp);
                REQUIRE(fd > 2);
                CHECK(GOOD_REPLICA == replica::replica_status(comm2, target_object, 0));

                // disconnect without close
            }

            // ensure all system metadata were updated properly
            const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
            CHECK(replica_info.mtime() == replica_info.ctime());
            CHECK(0 == static_cast<unsigned long>(replica_info.size()));
            CHECK(GOOD_REPLICA == replica_info.replica_status());
        }

        SECTION("open for write,no close")
        {
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            unit_test_utils::create_empty_replica(comm, target_object);

            std::this_thread::sleep_for(2s);

            {
                irods::experimental::client_connection conn2;
                RcComm& comm2 = static_cast<RcComm&>(conn2);

                dataObjInp_t open_inp{};
                std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", path_str.data());
                open_inp.openFlags = O_WRONLY;
                const auto fd = rcDataObjOpen(&comm2, &open_inp);
                REQUIRE(fd > 2);
                CHECK(INTERMEDIATE_REPLICA == replica::replica_status(comm2, target_object, 0));

                // disconnect without close
            }

            // Sleep here to ensure that the catalog has had enough time to update
            std::this_thread::sleep_for(10s);

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

            std::this_thread::sleep_for(2s);

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

            openedDataObjInp_t close_inp{};
            close_inp.l1descInx = fd;
            REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

            // ensure all system metadata were updated properly
            const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
            CHECK(replica_info.mtime() == replica_info.ctime());
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

            std::this_thread::sleep_for(2s);

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

                // Sleep here to make sure that the mtime is not updated
                std::this_thread::sleep_for(2s);

                // disconnect without close
            }

            // Sleep here to ensure that the catalog has had enough time to update
            std::this_thread::sleep_for(2s);

            // ensure all system metadata were updated properly
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
            CHECK(replica_info.mtime() == replica_info.ctime());
            CHECK(contents.size() + 1 == static_cast<unsigned long>(replica_info.size()));
            CHECK(STALE_REPLICA == replica_info.replica_status());
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

    const auto logical_path = sandbox / "eels.txt";

    SECTION("#5496: custom put operations erase checksums")
    {
        auto* conn_ptr = static_cast<RcComm*>(conn);
        const std::string_view expected_checksum = "sha2:47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=";

        const auto do_test = [&](const std::string_view _key,
                                 const std::string_view _value,
                                 bool _test_mismatch_checksums)
        {
            // Create an empty data object.
            dataObjInp_t open_inp{};
            std::strcpy(open_inp.objPath, logical_path.c_str());
            open_inp.openFlags = O_CREAT | O_TRUNC | O_WRONLY;
            open_inp.oprType = PUT_OPR;

            addKeyVal(&open_inp.condInput, _key.data(), _value.data());
            auto fd = rcDataObjOpen(conn_ptr, &open_inp);
            REQUIRE(fd > 2);

            openedDataObjInp_t close_inp{};
            close_inp.l1descInx = fd;

            if (_test_mismatch_checksums) {
                REQUIRE(rcDataObjClose(conn_ptr, &close_inp) == USER_CHKSUM_MISMATCH);
            }
            else {
                REQUIRE(rcDataObjClose(conn_ptr, &close_inp) >= 0);
            }

            // Verify the state of the replica.
            const auto [rp, lm] = replica::make_replica_proxy<RcComm>(conn, logical_path, 0);
            CHECK(rp.size() == 0);
            CHECK(rp.checksum() == expected_checksum);

            if (_test_mismatch_checksums) {
                CHECK(rp.replica_status() == STALE_REPLICA);
            }
            else {
                CHECK(rp.replica_status() == GOOD_REPLICA);
            }

            // Overwrite the replica.
            rmKeyVal(&open_inp.condInput, _key.data());
            fd = rcDataObjOpen(conn_ptr, &open_inp);
            REQUIRE(fd > 2);

            const std::string_view new_data = "ABC";
            bytesBuf_t bbuf{};
            bbuf.buf = const_cast<char*>(new_data.data());
            bbuf.len = new_data.size() + 1;

            openedDataObjInp_t write_inp{};
            write_inp.l1descInx = fd;
            write_inp.len = bbuf.len;
            const auto bytes_written = rcDataObjWrite(conn_ptr, &write_inp, &bbuf);
            CHECK(bbuf.len == bytes_written);

            close_inp.l1descInx = fd;
            REQUIRE(rcDataObjClose(conn_ptr, &close_inp) >= 0);

            // Show that the checksum has been erased.
            {
                const auto [rp, lm] = replica::make_replica_proxy<RcComm>(conn, logical_path, 0);
                CHECK(rp.size() == bytes_written);
                REQUIRE(rp.replica_status() == GOOD_REPLICA);
                REQUIRE(rp.checksum().empty());
            }
        };

        SECTION("register new checksum")
        {
            do_test(REG_CHKSUM_KW, "", /* test_mismatch_checksums */ false);
        }

        SECTION("verify with expected checksum")
        {
            do_test(VERIFY_CHKSUM_KW, expected_checksum, /* test_mismatch_checksums */ false);
        }

        SECTION("verify with mismatch checksum")
        {
            do_test(VERIFY_CHKSUM_KW, "---=== USER CHECKSUM MISMATCH ===---", /* test_mismatch_checksums */ true);
        }
    }

    SECTION("#5400: VERIFY_CHKSUM_KW is always honored on single buffer put")
    {
        DataObjInp put_input{};
        put_input.createMode = 0600;
        addKeyVal(&put_input.condInput, DEF_RESC_NAME_KW, env.rodsDefResource);

        std::strcpy(put_input.objPath, logical_path.c_str());

        // Set a bad checksum so that the PUT operation fails.
        std::string_view file_contents = "My hovercraft is full of eels";
        addKeyVal(&put_input.condInput, VERIFY_CHKSUM_KW, file_contents.data());

        // Put a file into iRODS and trigger an error.
        const auto local_file = boost::filesystem::temp_directory_path() / "eels.txt";
        { std::ofstream{local_file.c_str()} << file_contents; }
        put_input.dataSize = file_contents.size();
        REQUIRE(rcDataObjPut(static_cast<RcComm*>(conn), &put_input, const_cast<char*>(local_file.c_str())) == USER_CHKSUM_MISMATCH);

        // Show that even though the PUT resulted in a checksum mismatch error, the catalog captures the
        // correct information for the replica.
        CHECK(replica::replica_status<RcComm>(conn, logical_path, env.rodsDefResource) == STALE_REPLICA);
        CHECK(replica::replica_size<RcComm>(conn, logical_path, env.rodsDefResource) == file_contents.size());

        std::string_view expected_checksum = "sha2:XL7wWccBvgYxXqmzjM4SYIGoTiocxaycQ9uzweaphcQ=";
        CHECK(replica::replica_checksum<RcComm>(conn, logical_path, env.rodsDefResource) == expected_checksum);

        // Overwrite the local file's contents so that we can prove that VERIFY_CHKSUM_KW is honored.
        file_contents = "My hovercraft has been taken over by sharks!";
        { std::ofstream{local_file.c_str()} << file_contents; }
        put_input.dataSize = file_contents.size();

        // Show that the VERIFY_CHKSUM_KW is honored when overwriting the replica via the FORCE_FLAG_KW flag.
        // The catalog will still reflect the correct information even though there was an error.
        addKeyVal(&put_input.condInput, FORCE_FLAG_KW, "");
        REQUIRE(rcDataObjPut(static_cast<RcComm*>(conn), &put_input, const_cast<char*>(local_file.c_str())) == USER_CHKSUM_MISMATCH);

        CHECK(replica::replica_status<RcComm>(conn, logical_path, env.rodsDefResource) == STALE_REPLICA);
        CHECK(replica::replica_size<RcComm>(conn, logical_path, env.rodsDefResource) == file_contents.size());

        expected_checksum = "sha2:U1GXHohbx5JiwKy3uqEI1lPyTfw/SZmKk5KhccdQ6iY=";
        CHECK(replica::replica_checksum<RcComm>(conn, logical_path, env.rodsDefResource) == expected_checksum);

        // Restore the local file's contents for verification.
        file_contents = "My hovercraft is full of eels";
        { std::ofstream{local_file.c_str()} << file_contents; }
        put_input.dataSize = file_contents.size();

        // Overwrite the replica again and include the correct checksum for verification.
        expected_checksum = "sha2:XL7wWccBvgYxXqmzjM4SYIGoTiocxaycQ9uzweaphcQ=";
        addKeyVal(&put_input.condInput, VERIFY_CHKSUM_KW, expected_checksum.data());
        REQUIRE(rcDataObjPut(static_cast<RcComm*>(conn), &put_input, const_cast<char*>(local_file.c_str())) == 0);

        CHECK(replica::replica_status<RcComm>(conn, logical_path, env.rodsDefResource) == GOOD_REPLICA);
        CHECK(replica::replica_size<RcComm>(conn, logical_path, env.rodsDefResource) == file_contents.size());
        CHECK(replica::replica_checksum<RcComm>(conn, logical_path, env.rodsDefResource) == expected_checksum);

        // Show that recalculating the checksum results in the same checksum because the checksum
        // API honors the data size in the catalog and not the physical object's size in storage.
        constexpr auto recalculate = replica::verification_calculation::always;
        CHECK(replica::replica_checksum<RcComm>(conn, logical_path, env.rodsDefResource, recalculate) == expected_checksum);
    }

    SECTION("#5400: VERIFY_CHKSUM_KW is always honored on parallel transfer put")
    {
        DataObjInp put_input{};
        put_input.createMode = 0600;
        put_input.dataSize = 40'000'000;
        addKeyVal(&put_input.condInput, DEF_RESC_NAME_KW, env.rodsDefResource);

        std::strcpy(put_input.objPath, logical_path.c_str());

        // Set a bad checksum so that the PUT operation fails.
        std::string_view file_contents = "---=== USER CHECKSUM MISMATCH ===---";
        addKeyVal(&put_input.condInput, VERIFY_CHKSUM_KW, file_contents.data());

        // Put a file into iRODS and trigger an error.
        const auto local_file = boost::filesystem::temp_directory_path() / "eels.txt";
        REQUIRE(unit_test_utils::create_local_file(local_file.c_str(), put_input.dataSize, 'A'));
        REQUIRE(rcDataObjPut(static_cast<RcComm*>(conn), &put_input, const_cast<char*>(local_file.c_str())) == USER_CHKSUM_MISMATCH);

        // Show that even though the PUT resulted in a checksum mismatch error, the catalog captures the
        // correct information for the replica.
        CHECK(replica::replica_status<RcComm>(conn, logical_path, env.rodsDefResource) == STALE_REPLICA);
        CHECK(replica::replica_size<RcComm>(conn, logical_path, env.rodsDefResource) == put_input.dataSize);

        std::string_view expected_checksum = "sha2:oC4OD1/qnmXHaUV298S0PCsoAvvWPmwh7oIHHEhsi1U=";
        CHECK(replica::replica_checksum<RcComm>(conn, logical_path, env.rodsDefResource) == expected_checksum);

        // Overwrite the local file's contents so that we can prove that VERIFY_CHKSUM_KW is honored.
        REQUIRE(unit_test_utils::create_local_file(local_file.c_str(), put_input.dataSize, 'B'));

        // Show that the VERIFY_CHKSUM_KW is honored when overwriting the replica via the FORCE_FLAG_KW flag.
        // The catalog will still reflect the correct information even though there was an error.
        addKeyVal(&put_input.condInput, FORCE_FLAG_KW, "");
        REQUIRE(rcDataObjPut(static_cast<RcComm*>(conn), &put_input, const_cast<char*>(local_file.c_str())) == USER_CHKSUM_MISMATCH);

        CHECK(replica::replica_status<RcComm>(conn, logical_path, env.rodsDefResource) == STALE_REPLICA);
        CHECK(replica::replica_size<RcComm>(conn, logical_path, env.rodsDefResource) == put_input.dataSize);

        expected_checksum = "sha2:Nje5rtNS8hXbdo639d9ux4gLptcm3sichw1PssbSnOg=";
        CHECK(replica::replica_checksum<RcComm>(conn, logical_path, env.rodsDefResource) == expected_checksum);

        // Restore the local file's contents for verification.
        REQUIRE(unit_test_utils::create_local_file(local_file.c_str(), put_input.dataSize, 'A'));

        // Overwrite the replica again and include the correct checksum for verification.
        expected_checksum = "sha2:oC4OD1/qnmXHaUV298S0PCsoAvvWPmwh7oIHHEhsi1U=";
        addKeyVal(&put_input.condInput, VERIFY_CHKSUM_KW, expected_checksum.data());
        REQUIRE(rcDataObjPut(static_cast<RcComm*>(conn), &put_input, const_cast<char*>(local_file.c_str())) == 0);

        CHECK(replica::replica_status<RcComm>(conn, logical_path, env.rodsDefResource) == GOOD_REPLICA);
        CHECK(replica::replica_size<RcComm>(conn, logical_path, env.rodsDefResource) == put_input.dataSize);
        CHECK(replica::replica_checksum<RcComm>(conn, logical_path, env.rodsDefResource) == expected_checksum);

        // Show that recalculating the checksum results in the same checksum because the checksum
        // API honors the data size in the catalog and not the physical object's size in storage.
        constexpr auto recalculate = replica::verification_calculation::always;
        CHECK(replica::replica_checksum<RcComm>(conn, logical_path, env.rodsDefResource, recalculate) == expected_checksum);
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

