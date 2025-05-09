#include <catch2/catch_all.hpp>

#include "irods_error_enum_matcher.hpp"
#include "unit_test_utils.hpp"

#include "irods/client_connection.hpp"
#include "irods/dataObjChksum.h"
#include "irods/dataObjInpOut.h"
#include "irods/dataObjLseek.h"
#include "irods/dataObjPut.h"
#include "irods/dstream.hpp"
#include "irods/fileLseek.h"
#include "irods/filesystem.hpp"
#include "irods/get_file_descriptor_info.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_exception.hpp"
#include "irods/key_value_proxy.hpp"
#include "irods/modDataObjMeta.h"
#include "irods/objInfo.h"
#include "irods/rcMisc.h"
#include "irods/register_physical_path.h"
#include "irods/replica.hpp"
#include "irods/replica_proxy.hpp"
#include "irods/resource_administration.hpp"
#include "irods/rodsClient.h"
#include "irods/rodsDef.h"
#include "irods/rodsErrorTable.h"
#include "irods/ticketAdmin.h"
#include "irods/ticket_administration.hpp"
#include "irods/touch.h"
#include "irods/transport/default_transport.hpp"

#include <boost/filesystem.hpp>
#include <fmt/format.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <csignal>
#include <string>
#include <string_view>
#include <thread>

using namespace std::chrono_literals;

// clang-format off
namespace adm     = irods::experimental::administration;
namespace fs      = irods::experimental::filesystem;
namespace replica = irods::experimental::replica;
// clang-format on

const std::string DEFAULT_RESOURCE_HIERARCHY = "demoResc";

namespace
{
    auto wait_for_agent_to_close(const pid_t _pid, const int _max_checks = 60) -> int
    {
        for (int i = 0; i < _max_checks; ++i) {
            if (0 != kill(_pid, 0)) {
                return 0;
            }

            std::this_thread::sleep_for(1s);
        }

        return -1;
    } // wait_for_agent_to_close
} // anonymous namespace

TEST_CASE("rc_data_obj_open")
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

        const std::string test_resc = "test_resc";
        const std::string vault_name = "test_resc_vault";

        // Create a new resource
        {
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);
            unit_test_utils::add_ufs_resource(comm, test_resc, vault_name);
        }

        irods::at_scope_exit remove_sandbox{[&sandbox, &test_resc] {
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            REQUIRE(fs::client::remove_all(comm, sandbox, fs::remove_options::no_trash));

            adm::client::remove_resource(comm, test_resc);
        }};

        const auto target_object = sandbox / "target_object";

        std::string_view path_str = target_object.c_str();

        std::string contents = "content!";

        SECTION("#7154: open with dataSize of -1 on resource with minimum_free_space_for_create_in_bytes configured")
        {
            constexpr char* free_space = "1000000000";
            constexpr char* context = "minimum_free_space_for_create_in_bytes=1000";

            // Modify the test resource free space and minimum_free_space_for_create_in_bytes in the context string.
            // This must be done on a separate connection because the resource manager is not updated with any changes
            // which occur to resources after it is initialized.
            {
                irods::experimental::client_connection conn;
                RcComm& comm = static_cast<RcComm&>(conn);

                REQUIRE_NOTHROW(adm::client::modify_resource(comm, test_resc, adm::free_space_property{free_space}));
                REQUIRE_NOTHROW(adm::client::modify_resource(comm, test_resc, adm::context_string_property{context}));
            }

            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            // Create data object with a specified dataSize of -1.
            dataObjInp_t open_inp{};
            std::strncpy(open_inp.objPath, path_str.data(), sizeof(open_inp.objPath));
            open_inp.openFlags = O_CREAT | O_TRUNC | O_WRONLY; // NOLINT(hicpp-signed-bitwise)
            open_inp.dataSize = -1;
            addKeyVal(&open_inp.condInput, DEST_RESC_NAME_KW, test_resc.data());
            const auto fd = rcDataObjOpen(&comm, &open_inp);
            REQUIRE(fd > 2);

            // Write something to the data object so we have something to check for success.
            bytesBuf_t write_bbuf{};
            write_bbuf.buf = const_cast<char*>(free_space); // NOLINT(cppcoreguidelines-pro-type-const-cast)
            // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
            write_bbuf.len = std::strlen(free_space) + 1;

            openedDataObjInp_t write_inp{};
            write_inp.l1descInx = fd;
            write_inp.len = write_bbuf.len;
            const auto bytes_written = rcDataObjWrite(&comm, &write_inp, &write_bbuf);
            CHECK(write_bbuf.len == bytes_written);

            // Close data object and ensure that everything completes successfully.
            openedDataObjInp_t close_inp{};
            close_inp.l1descInx = fd;
            REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

            // Show that the data object was created and has the right size.
            const auto [rp, lm] = replica::make_replica_proxy<RcComm>(conn, target_object, 0);
            CHECK(rp.replica_status() == GOOD_REPLICA);
            CHECK(rp.size() == write_bbuf.len);
        }

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

        SECTION("create_with_no_close")
        {
            pid_t unclosed_fd_pid{};

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

                unclosed_fd_pid = unit_test_utils::get_agent_pid(comm);
                REQUIRE(unclosed_fd_pid > 0);

                // disconnect without close
            }

            REQUIRE(0 == wait_for_agent_to_close(unclosed_fd_pid));

            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            // ensure all system metadata were updated properly
            const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
            CHECK(replica_info.mtime() != replica_info.ctime());
            CHECK(0 == static_cast<unsigned long>(replica_info.size()));
            CHECK(STALE_REPLICA == replica_info.replica_status());
        }

        SECTION("open_for_read_and_close")
        {
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            unit_test_utils::create_empty_replica(comm, target_object);

            std::string original_mtime_for_replica_0;
            {
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
                original_mtime_for_replica_0 = replica_info.mtime();
            }

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
            CHECK(replica_info.mtime() == original_mtime_for_replica_0);
            CHECK(0 == static_cast<unsigned long>(replica_info.size()));
            CHECK(GOOD_REPLICA == replica_info.replica_status());
        }

        SECTION("open_for_write_and_close")
        {
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            unit_test_utils::create_empty_replica(comm, target_object);

            std::string original_mtime_for_replica_0;
            {
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
                original_mtime_for_replica_0 = replica_info.mtime();
            }

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
            CHECK(replica_info.mtime() == original_mtime_for_replica_0);
            CHECK(0 == static_cast<unsigned long>(replica_info.size()));
            CHECK(GOOD_REPLICA == replica_info.replica_status());
        }

        SECTION("open_for_read_no_close")
        {
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            unit_test_utils::create_empty_replica(comm, target_object);

            std::string original_mtime_for_replica_0;
            {
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
                original_mtime_for_replica_0 = replica_info.mtime();
            }

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
            CHECK(replica_info.mtime() == original_mtime_for_replica_0);
            CHECK(0 == static_cast<unsigned long>(replica_info.size()));
            CHECK(GOOD_REPLICA == replica_info.replica_status());
        }

        SECTION("open_for_write_no_close")
        {
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            unit_test_utils::create_empty_replica(comm, target_object);

            std::string original_mtime_for_replica_0;
            {
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
                original_mtime_for_replica_0 = replica_info.mtime();
            }

            std::this_thread::sleep_for(2s);

            pid_t unclosed_fd_pid{};

            {
                irods::experimental::client_connection conn2;
                RcComm& comm2 = static_cast<RcComm&>(conn2);

                dataObjInp_t open_inp{};
                std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", path_str.data());
                open_inp.openFlags = O_WRONLY;
                const auto fd = rcDataObjOpen(&comm2, &open_inp);
                REQUIRE(fd > 2);
                CHECK(INTERMEDIATE_REPLICA == replica::replica_status(comm2, target_object, 0));

                unclosed_fd_pid = unit_test_utils::get_agent_pid(comm2);
                REQUIRE(unclosed_fd_pid > 0);

                // disconnect without close
            }

            REQUIRE(0 == wait_for_agent_to_close(unclosed_fd_pid));

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
            CHECK(replica_info.mtime() != original_mtime_for_replica_0);
            CHECK(0 == static_cast<unsigned long>(replica_info.size()));
            CHECK(STALE_REPLICA == replica_info.replica_status());
        }

        SECTION("create_no_close_and_open_for_write_close")
        {
            pid_t unclosed_fd_pid{};

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

                unclosed_fd_pid = unit_test_utils::get_agent_pid(comm);
                REQUIRE(unclosed_fd_pid > 0);

                // sleep here to ensure that the mtime is updated on auto-close
                std::this_thread::sleep_for(2s);

                // disconnect without close
            }

            REQUIRE(0 == wait_for_agent_to_close(unclosed_fd_pid));

            {
                irods::experimental::client_connection conn;
                RcComm& comm = static_cast<RcComm&>(conn);

                // ensure all system metadata were updated properly
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
                CHECK(replica_info.mtime() != replica_info.ctime());
                CHECK(0 == static_cast<unsigned long>(replica_info.size()));
                CHECK(STALE_REPLICA == replica_info.replica_status());
            }

            // sleep here to test that the mtime is NOT updated on close
            std::this_thread::sleep_for(2s);

            // open for write, close
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            std::string original_mtime_for_replica_0;
            {
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
                original_mtime_for_replica_0 = replica_info.mtime();
            }

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
            CHECK(replica_info.mtime() == original_mtime_for_replica_0);
            CHECK(0 == static_cast<unsigned long>(replica_info.size()));
            CHECK(STALE_REPLICA == replica_info.replica_status());
        }

        SECTION("create_write_close")
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
            CHECK(replica_info.mtime() != replica_info.ctime());
            CHECK(contents.size() + 1 == static_cast<unsigned long>(replica_info.size()));
            CHECK(GOOD_REPLICA == replica_info.replica_status());
        }

        SECTION("create_write_and_no_close")
        {
            pid_t unclosed_fd_pid{};

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

                // Sleep here to make sure that the mtime is updated
                std::this_thread::sleep_for(2s);

                unclosed_fd_pid = unit_test_utils::get_agent_pid(comm);
                REQUIRE(unclosed_fd_pid > 0);

                // disconnect without close
            }

            REQUIRE(0 == wait_for_agent_to_close(unclosed_fd_pid));

            // ensure all system metadata were updated properly
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
            CHECK(replica_info.mtime() != replica_info.ctime());
            CHECK(contents.size() + 1 == static_cast<unsigned long>(replica_info.size()));
            CHECK(STALE_REPLICA == replica_info.replica_status());
        }

        SECTION("create_empty_replica_on_non_default_resource_and_write_in_separate_open__issue_5548")
        {
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            // Create the initial replica on non-default resource as an empty replica.
            unit_test_utils::create_empty_replica(comm, target_object, test_resc);
            REQUIRE(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));
            REQUIRE(!replica::replica_exists(comm, target_object, 1));

            std::string original_mtime_for_replica_0;
            {
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
                original_mtime_for_replica_0 = replica_info.mtime();
            }

            // Sleep to ensure that mtime updates when we write to it.
            std::this_thread::sleep_for(2s);

            // Open the replica again with O_CREAT and use DEST_RESC_NAME_KW to
            // influence the vote. We expect a new replica to be created on the default
            // resource.
            dataObjInp_t open_inp{};
            std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", path_str.data());
            open_inp.openFlags = O_CREAT | O_WRONLY;
            addKeyVal(&open_inp.condInput, DEST_RESC_NAME_KW, test_resc.data());
            const auto fd = rcDataObjOpen(&comm, &open_inp);
            REQUIRE(fd > 2);
            CHECK(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));
            CHECK(!replica::replica_exists(comm, target_object, 1));

            // Write data to the replica so that it is different from the initial replica.
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

            // Ensure that no new replicas appeared and that the existing replica is good.
            CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));
            CHECK(!replica::replica_exists(comm, target_object, 1));

            // Ensure all system metadata were updated properly
            const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
            CHECK(replica_info.mtime() != original_mtime_for_replica_0);
            CHECK(contents.size() + 1 == static_cast<unsigned long>(replica_info.size()));
        }

        SECTION("create_empty_replica_on_non_default_resource_and_create_with_no_keyword__issue_5548")
        {
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            // Create the initial replica on non-default resource as an empty replica.
            unit_test_utils::create_empty_replica(comm, target_object, test_resc);
            REQUIRE(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));
            REQUIRE(!replica::replica_exists(comm, target_object, 1));

            std::string original_mtime_for_replica_0;
            {
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
                original_mtime_for_replica_0 = replica_info.mtime();
            }

            // Sleep to ensure that mtime does not update when we create
            // the new replica.
            std::this_thread::sleep_for(2s);

            // Open the replica again with O_CREAT and without specifying a replica.
            // We expect a new replica to be created on the default resource.
            dataObjInp_t open_inp{};
            std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", path_str.data());
            open_inp.openFlags = O_CREAT | O_WRONLY;
            const auto fd = rcDataObjOpen(&comm, &open_inp);
            REQUIRE(fd > 2);
            CHECK(WRITE_LOCKED == replica::replica_status(comm, target_object, 0));
            CHECK(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 1));

            // Write data to the new replica so that it is different from the initial,
            // empty replica.
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

            // Ensure all system metadata were updated properly for the original replica.
            {
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
                CHECK(replica_info.mtime() == original_mtime_for_replica_0);
                CHECK(0 == static_cast<unsigned long>(replica_info.size()));
                REQUIRE(STALE_REPLICA == replica::replica_status(comm, target_object, 0));
            }

            // Ensure all system metadata were updated properly for the new replica.
            {
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 1);
                CHECK(replica_info.mtime() != original_mtime_for_replica_0);
                CHECK(contents.size() + 1 == static_cast<unsigned long>(replica_info.size()));
                REQUIRE(GOOD_REPLICA == replica::replica_status(comm, target_object, 1));
            }
        }

        SECTION("open_same_replica_on_non_default_resource_twice_with_other_replica_on_default_resource__issue_5848")
        {
            // Create a new connection so that newly created resources are known.
            irods::experimental::client_connection conn;
            RcComm& comm = static_cast<RcComm&>(conn);

            // Create two replicas - one on the newly created resource and one on the default resource.
            unit_test_utils::create_empty_replica(comm, target_object, test_resc);
            unit_test_utils::replicate_data_object(comm, path_str, DEFAULT_RESOURCE_HIERARCHY);
            REQUIRE(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));
            REQUIRE(GOOD_REPLICA == replica::replica_status(comm, target_object, 1));

            std::string original_mtime_for_replica_0;
            {
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
                original_mtime_for_replica_0 = replica_info.mtime();
            }

            std::string original_mtime_for_replica_1;
            {
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 1);
                original_mtime_for_replica_1 = replica_info.mtime();
            }

            // Sleep here so that the mtime on the replica we open will be updated.
            std::this_thread::sleep_for(2s);

            // Open the replica on the non-default resource with O_CREAT flag specified because this
            // is how some clients are using open. Should target an existing replica for overwrite.
            dataObjInp_t open_inp{};
            std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", path_str.data());
            open_inp.openFlags = O_CREAT | O_WRONLY;
            addKeyVal(&open_inp.condInput, DEST_RESC_NAME_KW, test_resc.data());

            // Open the replica and ensure that the status is updated appropriately.
            const auto fd = rcDataObjOpen(&comm, &open_inp);
            REQUIRE(fd > 2);
            CHECK(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));
            CHECK(WRITE_LOCKED == replica::replica_status(comm, target_object, 1));

            // Get the file descriptor info from the opened replica and extract the replica token.
            // We are not dealing in hierarchies here so that piece is not required here.
            char* out = nullptr;
            const auto json_input = nlohmann::json{{"fd", fd}}.dump();
            const auto l1_desc_info = rc_get_file_descriptor_info(&comm, json_input.data(), &out);
            REQUIRE(l1_desc_info == 0);
            const auto token = nlohmann::json::parse(out).at("replica_token").get<std::string>();
            std::free(out);

            // Open the same replica on a different connection using the replica token and replica
            // resource hierarchy. Ensure that the open is successful. This would fail if the wrong
            // replica is targeted because no agent can open a replica which is write-locked, as the
            // sibling replica on the default resource should be in this case.
            {
                irods::experimental::client_connection conn2;
                RcComm& comm2 = static_cast<RcComm&>(conn2);

                dataObjInp_t open_inp2{};
                std::snprintf(open_inp2.objPath, sizeof(open_inp2.objPath), "%s", path_str.data());
                open_inp2.openFlags = O_CREAT | O_WRONLY;
                addKeyVal(&open_inp2.condInput, RESC_HIER_STR_KW, test_resc.data());
                addKeyVal(&open_inp2.condInput, REPLICA_TOKEN_KW, token.data());

                const auto fd2 = rcDataObjOpen(&comm2, &open_inp2);
                REQUIRE(fd2 > 2);
                CHECK(INTERMEDIATE_REPLICA == replica::replica_status(comm2, target_object, 0));
                CHECK(WRITE_LOCKED == replica::replica_status(comm2, target_object, 1));

                REQUIRE(unit_test_utils::close_replica(comm2, fd2) >= 0);
            }

            openedDataObjInp_t close_inp{};
            close_inp.l1descInx = fd;
            REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

            // Ensure all system metadata were updated properly. That is, the system metadata
            // should be unchanged except for the replica statuses which should be restored
            // to what they were before opening.
            {
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 0);
                CHECK(replica_info.mtime() == original_mtime_for_replica_0);
                CHECK(0 == static_cast<unsigned long>(replica_info.size()));
                CHECK(GOOD_REPLICA == replica_info.replica_status());
            }

            {
                const auto [replica_info, replica_lm] = replica::make_replica_proxy(comm, target_object, 1);
                CHECK(replica_info.mtime() == original_mtime_for_replica_1);
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
} // rc_data_obj_open

TEST_CASE("read-only streams")
{
    namespace ix = irods::experimental;
    namespace io = irods::experimental::io;

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

    const auto data_object = sandbox / "data_object.txt";

    SECTION("#5352: read/seek operations honor the data size on read-only streams")
    {
        const std::string_view contents = "0123456789";

        {
            io::client::native_transport tp{conn};
            io::odstream{tp, data_object} << contents;
        }

        auto* conn_ptr = static_cast<RcComm*>(conn);

        // Change the data size of the replica stored in the catalog.
        DataObjInfo obj_info{};
        std::strcpy(obj_info.objPath, data_object.c_str());

        const auto [kvp, lm] = ix::make_key_value_proxy({{DATA_SIZE_KW, "7"}});

        ModDataObjMetaInp update_inp{};
        update_inp.dataObjInfo = &obj_info;
        update_inp.regParam = kvp.get();
        REQUIRE(rcModDataObjMeta(conn_ptr, &update_inp) == 0);

        // Open the replica in read-only mode.
        dataObjInp_t open_inp{};
        std::strcpy(open_inp.objPath, data_object.c_str());
        open_inp.openFlags = O_RDONLY;
        const auto fd = rcDataObjOpen(conn_ptr, &open_inp);
        REQUIRE(fd > 2);

        SECTION("rxDataObjSeek will not seek past the data size")
        {
            // Show that the offset is clamped to the size in the catalog because
            // the lseek operation honors the data size in the catalog for read-only
            // streams.
            openedDataObjInp_t seek_inp{};
            seek_inp.l1descInx = fd;
            seek_inp.whence = SEEK_CUR;
            seek_inp.offset = 1000;

            fileLseekOut_t* seek_out{};

            CHECK(rcDataObjLseek(conn_ptr, &seek_inp, &seek_out) == 0);
            REQUIRE(seek_out->offset == 7);

            // Show that reading from the replica returns zero.
            openedDataObjInp_t read_inp{};
            read_inp.l1descInx = fd;
            read_inp.len = 100;

            bytesBuf_t read_bbuf{};
            read_bbuf.len = read_inp.len;
            read_bbuf.buf = std::malloc(read_bbuf.len);
            irods::at_scope_exit free_bbuf{[&read_bbuf] { std::free(read_bbuf.buf); }};

            REQUIRE(rcDataObjRead(conn_ptr, &read_inp, &read_bbuf) == 0);

            // Close the replica.
            openedDataObjInp_t close_inp{};
            close_inp.l1descInx = fd;
            REQUIRE(rcDataObjClose(conn_ptr, &close_inp) >= 0);
        }

        SECTION("rxDataObjRead will not read past the data size")
        {
            // Move the file read position to the middle of the known data size.
            openedDataObjInp_t seek_inp{};
            seek_inp.l1descInx = fd;
            seek_inp.whence = SEEK_CUR;
            seek_inp.offset = 4;

            fileLseekOut_t* seek_out{};

            CHECK(rcDataObjLseek(conn_ptr, &seek_inp, &seek_out) == 0);
            REQUIRE(seek_out->offset == 4);

            // Show that the read operation will not read past the recorded data size.
            openedDataObjInp_t read_inp{};
            read_inp.l1descInx = fd;
            read_inp.len = 100;

            bytesBuf_t read_bbuf{};
            read_bbuf.len = read_inp.len;
            read_bbuf.buf = std::malloc(read_bbuf.len);

            irods::at_scope_exit free_bbuf{[&read_bbuf] { std::free(read_bbuf.buf); }};

            CHECK(rcDataObjRead(conn_ptr, &read_inp, &read_bbuf) == 3);

            const auto* data = static_cast<const char*>(read_bbuf.buf);
            REQUIRE(std::equal(data, data + 3, std::next(std::begin(contents), seek_inp.offset)));

            // Close the replica.
            openedDataObjInp_t close_inp{};
            close_inp.l1descInx = fd;
            REQUIRE(rcDataObjClose(conn_ptr, &close_inp) >= 0);
        }
    }
}

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
        CHECK(static_cast<rodsLong_t>(replica::replica_size<RcComm>(conn, logical_path, env.rodsDefResource)) == put_input.dataSize);

        std::string_view expected_checksum = "sha2:oC4OD1/qnmXHaUV298S0PCsoAvvWPmwh7oIHHEhsi1U=";
        CHECK(replica::replica_checksum<RcComm>(conn, logical_path, env.rodsDefResource) == expected_checksum);

        // Overwrite the local file's contents so that we can prove that VERIFY_CHKSUM_KW is honored.
        REQUIRE(unit_test_utils::create_local_file(local_file.c_str(), put_input.dataSize, 'B'));

        // Show that the VERIFY_CHKSUM_KW is honored when overwriting the replica via the FORCE_FLAG_KW flag.
        // The catalog will still reflect the correct information even though there was an error.
        addKeyVal(&put_input.condInput, FORCE_FLAG_KW, "");
        REQUIRE(rcDataObjPut(static_cast<RcComm*>(conn), &put_input, const_cast<char*>(local_file.c_str())) == USER_CHKSUM_MISMATCH);

        CHECK(replica::replica_status<RcComm>(conn, logical_path, env.rodsDefResource) == STALE_REPLICA);
        CHECK(static_cast<rodsLong_t>(replica::replica_size<RcComm>(conn, logical_path, env.rodsDefResource)) == put_input.dataSize);

        expected_checksum = "sha2:Nje5rtNS8hXbdo639d9ux4gLptcm3sichw1PssbSnOg=";
        CHECK(replica::replica_checksum<RcComm>(conn, logical_path, env.rodsDefResource) == expected_checksum);

        // Restore the local file's contents for verification.
        REQUIRE(unit_test_utils::create_local_file(local_file.c_str(), put_input.dataSize, 'A'));

        // Overwrite the replica again and include the correct checksum for verification.
        expected_checksum = "sha2:oC4OD1/qnmXHaUV298S0PCsoAvvWPmwh7oIHHEhsi1U=";
        addKeyVal(&put_input.condInput, VERIFY_CHKSUM_KW, expected_checksum.data());
        REQUIRE(rcDataObjPut(static_cast<RcComm*>(conn), &put_input, const_cast<char*>(local_file.c_str())) == 0);

        CHECK(replica::replica_status<RcComm>(conn, logical_path, env.rodsDefResource) == GOOD_REPLICA);
        CHECK(static_cast<rodsLong_t>(replica::replica_size<RcComm>(conn, logical_path, env.rodsDefResource)) == put_input.dataSize);
        CHECK(replica::replica_checksum<RcComm>(conn, logical_path, env.rodsDefResource) == expected_checksum);

        // Show that recalculating the checksum results in the same checksum because the checksum
        // API honors the data size in the catalog and not the physical object's size in storage.
        constexpr auto recalculate = replica::verification_calculation::always;
        CHECK(replica::replica_checksum<RcComm>(conn, logical_path, env.rodsDefResource, recalculate) == expected_checksum);
    }
} // rxDataObjPut

TEST_CASE("#6600: server no longer changes the openmode from O_WRONLY to O_RDWR")
{
    namespace ix = irods::experimental;
    namespace io = irods::experimental::io;

    load_client_api_plugins();

    ix::client_connection conn;
    REQUIRE(conn);

    rodsEnv env;
    _getRodsEnv(env);

    const auto sandbox = fs::path{env.rodsHome} / "unit_testing_sandbox";

    if (!fs::client::exists(conn, sandbox)) {
        REQUIRE(fs::client::create_collection(conn, sandbox));
    }

    irods::at_scope_exit remove_sandbox{
        [&conn, &sandbox] { REQUIRE(fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash)); }};

    const auto data_object = sandbox / "data_object.txt";
    const std::string_view contents = "0123456789";

    // Create a new data object.
    {
        io::client::native_transport tp{conn};
        io::odstream{tp, data_object} << contents;
        REQUIRE(fs::client::data_object_size(conn, data_object) == contents.size());
    }

    auto* conn_ptr = static_cast<RcComm*>(conn);

    // Open the replica in write-only mode.
    dataObjInp_t open_inp{};
    std::strcpy(open_inp.objPath, data_object.c_str());
    open_inp.openFlags = O_WRONLY;
    const auto fd = rcDataObjOpen(conn_ptr, &open_inp);
    REQUIRE(fd > 2);

    // Show that the read operation will fail because the server honored the openmode.
    openedDataObjInp_t read_inp{};
    read_inp.l1descInx = fd;
    read_inp.len = contents.size();

    bytesBuf_t read_bbuf{};
    read_bbuf.len = read_inp.len;
    read_bbuf.buf = std::malloc(read_bbuf.len);
    irods::at_scope_exit free_bbuf{[&read_bbuf] { std::free(read_bbuf.buf); }};

    const auto ec = rcDataObjRead(conn_ptr, &read_inp, &read_bbuf);

    // Compare only the iRODS error code (i.e. ignore errno value).
    CHECK(ec - (ec % UNIX_FILE_READ_ERR) == UNIX_FILE_READ_ERR);

    // Close the replica.
    openedDataObjInp_t close_inp{};
    close_inp.l1descInx = fd;
    REQUIRE(rcDataObjClose(conn_ptr, &close_inp) >= 0);
}

TEST_CASE("#7250: updates to ticket stats do not hang")
{
    load_client_api_plugins();

    irods::experimental::client_connection conn1;
    REQUIRE(conn1);

    rodsEnv env;
    _getRodsEnv(env);

    const auto sandbox = fs::path{env.rodsHome} / "unit_testing_sandbox";

    if (!fs::client::exists(conn1, sandbox)) {
        REQUIRE(fs::client::create_collection(conn1, sandbox));
    }

    irods::at_scope_exit remove_sandbox{
        [&conn1, &sandbox] { REQUIRE(fs::client::remove_all(conn1, sandbox, fs::remove_options::no_trash)); }};

    const auto data_object = sandbox / "issue_7250.txt";

    // Create a new data object.
    {
        irods::experimental::io::client::native_transport tp{conn1};
        irods::experimental::io::odstream{tp, data_object} << "the data";
        REQUIRE(fs::client::data_object_size(conn1, data_object) > 0);
    }

    namespace ticket = irods::experimental::administration::ticket;

    const auto enable_ticket = [](auto& _conn, const auto* _ticket_string) {
        TicketAdminInput input{};
        input.arg1 = const_cast<char*>("session"); // NOLINT(cppcoreguidelines-pro-type-const-cast)
        input.arg2 = const_cast<char*>(_ticket_string); // NOLINT(cppcoreguidelines-pro-type-const-cast)
        input.arg3 = const_cast<char*>(""); // NOLINT(cppcoreguidelines-pro-type-const-cast)
        REQUIRE(rcTicketAdmin(static_cast<RcComm*>(_conn), &input) == 0);
    };

    SECTION("for read tickets and rcGetHostForGet")
    {
        std::string ticket_string;

        SECTION("create ticket for collection")
        {
            REQUIRE_NOTHROW(ticket_string =
                                ticket::client::create_ticket(conn1, ticket::ticket_type::read, sandbox.c_str()));
        }

        SECTION("create ticket for data object")
        {
            REQUIRE_NOTHROW(ticket_string =
                                ticket::client::create_ticket(conn1, ticket::ticket_type::read, data_object.c_str()));
        }

        irods::at_scope_exit delete_ticket{
            [&conn1, &ticket_string] { ticket::client::delete_ticket(conn1, ticket_string); }};

        REQUIRE_FALSE(ticket_string.empty());

        // Enable the ticket on the connection.
        enable_ticket(conn1, ticket_string.c_str());
        irods::at_scope_exit disable_ticket{[&conn1, &enable_ticket] { enable_ticket(conn1, ""); }};

        // Call rcGetHostForGet to determine to which server to connect.
        // Keep in mind there is only one server. This step is necessary for reproducing the error.
        dataObjInp_t open_inp{};
        std::strcpy(open_inp.objPath, data_object.c_str()); // NOLINT(clang-analyzer-security.insecureAPI.strcpy)
        open_inp.openFlags = O_RDONLY;
        char* new_host{};
        REQUIRE(rcGetHostForGet(static_cast<RcComm*>(conn1), &open_inp, &new_host) == 0);
        REQUIRE(new_host != nullptr);
        REQUIRE(std::strlen(new_host) > 0);

        // Connect to the server using a different connection and enable the ticket on it.
        irods::experimental::client_connection conn2{new_host, env.rodsPort, {env.rodsUserName, env.rodsZone}};
        std::free(new_host); // NOLINT(cppcoreguidelines-no-malloc, cppcoreguidelines-owning-memory)
        enable_ticket(conn2, ticket_string.c_str());

        // Open the replica for reading using the new connection.
        auto* conn_ptr2 = static_cast<RcComm*>(conn2);
        const auto fd = rcDataObjOpen(conn_ptr2, &open_inp);
        REQUIRE(fd > 2);

        // Close the replica.
        openedDataObjInp_t close_inp{};
        close_inp.l1descInx = fd;
        REQUIRE(rcDataObjClose(conn_ptr2, &close_inp) >= 0);
    }

    SECTION("for write tickets and rcGetHostForPut")
    {
        std::string ticket_string;

        SECTION("create ticket for collection")
        {
            REQUIRE_NOTHROW(ticket_string =
                                ticket::client::create_ticket(conn1, ticket::ticket_type::write, sandbox.c_str()));
        }

        SECTION("create ticket for data object")
        {
            REQUIRE_NOTHROW(ticket_string =
                                ticket::client::create_ticket(conn1, ticket::ticket_type::write, data_object.c_str()));
        }

        irods::at_scope_exit delete_ticket{
            [&conn1, &ticket_string] { ticket::client::delete_ticket(conn1, ticket_string); }};

        REQUIRE_FALSE(ticket_string.empty());

        // Set an upper limit on the total number of bytes that can be written to the replica.
        // This is required so the server knows to track/update the byte count.
        // Without this, the assertion involving the GenQuery check would always fail.
        REQUIRE_NOTHROW(
            ticket::client::set_ticket_constraint(conn1, ticket_string, ticket::n_write_bytes_constraint{100}));

        // Enable the ticket on the connection.
        enable_ticket(conn1, ticket_string.c_str());
        irods::at_scope_exit disable_ticket{[&conn1, &enable_ticket] { enable_ticket(conn1, ""); }};

        // Call rcGetHostForPut to determine to which server to connect.
        // Keep in mind there is only one server. This step is necessary for reproducing the error.
        dataObjInp_t open_inp{};
        std::strcpy(open_inp.objPath, data_object.c_str()); // NOLINT(clang-analyzer-security.insecureAPI.strcpy)
        open_inp.openFlags = O_WRONLY;
        char* new_host{};
        REQUIRE(rcGetHostForPut(static_cast<RcComm*>(conn1), &open_inp, &new_host) == 0);
        REQUIRE(new_host != nullptr);
        REQUIRE(std::strlen(new_host) > 0);

        // Connect to the server using a different connection and enable the ticket on it.
        irods::experimental::client_connection conn2{new_host, env.rodsPort, {env.rodsUserName, env.rodsZone}};
        std::free(new_host); // NOLINT(cppcoreguidelines-no-malloc, cppcoreguidelines-owning-memory)
        enable_ticket(conn2, ticket_string.c_str());

        // Open the replica for writing using the new connection.
        auto* conn_ptr2 = static_cast<RcComm*>(conn2);
        const auto fd = rcDataObjOpen(conn_ptr2, &open_inp);
        REQUIRE(fd > 2);

        // Write new data to the replica.
        char new_data[] = "the new data"; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
        bytesBuf_t write_bbuf{};
        write_bbuf.buf = new_data;
        write_bbuf.len = static_cast<int>(std::strlen(new_data));

        openedDataObjInp_t write_inp{};
        write_inp.l1descInx = fd;
        write_inp.len = write_bbuf.len;
        const auto bytes_written = rcDataObjWrite(conn_ptr2, &write_inp, &write_bbuf);
        CHECK(write_bbuf.len == bytes_written);

        // Close the replica.
        openedDataObjInp_t close_inp{};
        close_inp.l1descInx = fd;
        REQUIRE(rcDataObjClose(conn_ptr2, &close_inp) >= 0);

        // Show that the ticket stats have been updated correctly.
        const auto gql = fmt::format(
            "select TICKET_WRITE_FILE_COUNT, TICKET_WRITE_BYTE_COUNT where TICKET_STRING = '{}'", ticket_string);

        for (auto&& row : irods::query{conn_ptr2, gql}) {
            CHECK(row[0] == "2");
            CHECK(row[1] == std::to_string(write_bbuf.len));
        }
    }
}

TEST_CASE("#7338")
{
    load_client_api_plugins();

    irods::experimental::client_connection conn{irods::experimental::defer_authentication};

    SECTION("rc_register_physical_path")
    {
        DataObjInp input{};
        char* json_error_string{};

        CHECK(rc_register_physical_path(static_cast<RcComm*>(conn), &input, &json_error_string) == SYS_NO_API_PRIV);
        CHECK(json_error_string == nullptr);
    }

    SECTION("rc_touch")
    {
        CHECK(rc_touch(static_cast<RcComm*>(conn), "") == SYS_NO_API_PRIV);
    }
}
