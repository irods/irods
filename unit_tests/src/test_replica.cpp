#include <catch2/catch.hpp>

#include "irods/client_connection.hpp"
#include "irods/data_object_proxy.hpp"
#include "irods/dstream.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/replica.hpp"
#include "irods/replica_proxy.hpp"
#include "irods/resource_administration.hpp"
#include "irods/rodsClient.h"
#include "irods/transport/default_transport.hpp"
#include "unit_test_utils.hpp"

#include <chrono>
#include <iostream>
#include <string_view>
#include <thread>

namespace fs = irods::experimental::filesystem;
namespace io = irods::experimental::io;
namespace replica = irods::experimental::replica;
namespace data_object = irods::experimental::data_object;

TEST_CASE("replica", "[replica]")
{
    using namespace std::chrono_literals;

    load_client_api_plugins();

    irods::experimental::client_connection conn;
    RcComm& comm = static_cast<RcComm&>(conn);

    rodsEnv env;
    _getRodsEnv(env);

    const auto sandbox = fs::path{env.rodsHome} / "test_replica";

    if (!fs::client::exists(comm, sandbox)) {
        REQUIRE(fs::client::create_collection(comm, sandbox));
    }

    irods::at_scope_exit remove_sandbox{[&comm, &sandbox] {
        REQUIRE(fs::client::remove_all(comm, sandbox, fs::remove_options::no_trash));
    }};

    const auto target_object = sandbox / "target_object";

    std::string_view expected_checksum = "sha2:z4DNiu1ILV0VJ9fccvzv+E5jJlkoSER9LcCw6H38mpA=";
    std::string_view object_content = "testing";

    std::string_view updated_object_content = "something else";
    std::string_view updated_checksum = "sha2:9B8/piX/Eg3cp+9Fa/ZjcezqI8Ep9OTDI2cQHttRbPg=";

    {
        io::client::default_transport tp{comm};
        io::odstream{tp, target_object} << object_content;
    }

    std::this_thread::sleep_for(2s);

    const std::string_view resc_name = "unit_test_ufs";

    REQUIRE_NOTHROW(unit_test_utils::add_ufs_resource(conn, resc_name, "unit_test_vault"));
    REQUIRE(unit_test_utils::replicate_data_object(conn, target_object.c_str(), resc_name));

    irods::at_scope_exit clean_up{[&conn, &target_object, &resc_name] {
        namespace ix = irods::experimental;
        fs::client::remove(conn, target_object, fs::remove_options::no_trash);
        ix::administration::client::remove_resource(conn, resc_name);
    }};

    // the replica number of the second replica.
    constexpr auto second_replica = 1;

    SECTION("perform library operations using replica number")
    {
        // existence
        REQUIRE(replica::replica_exists(comm, target_object, second_replica));

        // status
        REQUIRE(GOOD_REPLICA == replica::replica_status(comm, target_object, second_replica));

        // size
        REQUIRE(!replica::is_replica_empty(comm, target_object, second_replica));
        REQUIRE(object_content.length() == replica::replica_size(comm, target_object, second_replica));
        REQUIRE(static_cast<rodsLong_t>(object_content.length()) == replica::get_replica_size_from_storage(comm, target_object, second_replica));

        // checksum a specific replica
        REQUIRE(replica::replica_checksum(comm, target_object, second_replica) == expected_checksum);

        // test replica::verification_calculation
        // overwrite second replica with new contents
        {
            std::ios_base::openmode mode = std::ios_base::out | std::ios_base::trunc;
            io::client::default_transport tp{comm};
            io::odstream{tp, target_object, io::replica_number{second_replica}, mode} << updated_object_content;
        }

        // show that the replica was updated
        REQUIRE(updated_object_content.length() == replica::replica_size(comm, target_object, second_replica));
        REQUIRE(static_cast<rodsLong_t>(updated_object_content.length()) == replica::get_replica_size_from_storage(comm, target_object, second_replica));

        // show that the checksum has been erased
        {
            const auto [rp, lm] = replica::make_replica_proxy<RcComm>(conn, target_object, second_replica);
            REQUIRE(rp.checksum().empty());
        }

        // now update the catalog using the force flag
        REQUIRE(replica::replica_checksum(comm, target_object, second_replica, replica::verification_calculation::always) == updated_checksum);

        // show that the catalog updated
        REQUIRE(replica::replica_checksum(comm, target_object, second_replica) == updated_checksum);

        // show that the other replica did not receive a checksum in the catalog
        {
            const auto [rp, lm] = replica::make_replica_proxy(comm, target_object, 0);
            REQUIRE(rp.checksum() == std::string{});
        }

        // show that the mtime of each replica is different.
        const auto old_mtime = replica::last_write_time(comm, target_object, 0);
        REQUIRE(old_mtime != replica::last_write_time(comm, target_object, second_replica));

        // update the mtime for the second replica
        replica::last_write_time(comm, target_object, second_replica, old_mtime);
        REQUIRE(old_mtime == replica::last_write_time(comm, target_object, second_replica));

        // get resource name from replica number
        REQUIRE(resc_name == replica::to_leaf_resource(comm, target_object, second_replica));
    }

    SECTION("perform library operations using resource name")
    {
        // existence
        REQUIRE(replica::replica_exists(comm, target_object, resc_name));

        // existence
        REQUIRE(GOOD_REPLICA == replica::replica_status(comm, target_object, resc_name));

        // size
        REQUIRE(!replica::is_replica_empty(comm, target_object, resc_name));
        REQUIRE(object_content.length() == replica::replica_size(comm, target_object, resc_name));
        REQUIRE(static_cast<rodsLong_t>(object_content.length()) == replica::get_replica_size_from_storage(comm, target_object, resc_name));

        // checksum a specific replica
        REQUIRE(replica::replica_checksum(comm, target_object, resc_name) == expected_checksum);

        // test replica::verification_calculation
        // overwrite second replica with new contents
        {
            std::ios_base::openmode mode = std::ios_base::out | std::ios_base::trunc;
            io::client::default_transport tp{comm};
            io::odstream{tp, target_object, io::leaf_resource_name{resc_name.data()}, mode} << updated_object_content;
        }

        // show that the replica was updated
        REQUIRE(updated_object_content.length() == replica::replica_size(comm, target_object, resc_name));
        REQUIRE(static_cast<rodsLong_t>(updated_object_content.length()) == replica::get_replica_size_from_storage(comm, target_object, resc_name));

        // show that the checksum has been erased
        {
            const auto [rp, lm] = replica::make_replica_proxy<RcComm>(conn, target_object, resc_name);
            REQUIRE(rp.checksum().empty());
        }

        // now update the catalog using the force flag
        REQUIRE(replica::replica_checksum(comm, target_object, resc_name, replica::verification_calculation::always) == updated_checksum);

        // show that the catalog updated
        REQUIRE(replica::replica_checksum(comm, target_object, resc_name) == updated_checksum);

        // show that the other replica did not receive a checksum in the catalog
        {
            const auto [rp, lm] = replica::make_replica_proxy(comm, target_object, 0);
            REQUIRE(rp.checksum() == std::string{});
        }

        // show that the mtime of each replica is different.
        const auto old_mtime = replica::last_write_time(comm, target_object, 0);
        REQUIRE(old_mtime != replica::last_write_time(comm, target_object, resc_name));

        // update the mtime for the second replica
        replica::last_write_time(comm, target_object, resc_name, old_mtime);
        REQUIRE(old_mtime == replica::last_write_time(comm, target_object, resc_name));

        // get replica number from resource name
        REQUIRE(second_replica == replica::to_replica_number(comm, target_object, resc_name));
    }

    SECTION("size in storage")
    {
        // get info from the catalog once
        auto result = replica::get_data_object_info(comm, target_object, resc_name).front();

        std::string_view physical_path = result[replica::genquery_column_index::DATA_PATH];

        // size in storage
        REQUIRE(static_cast<rodsLong_t>(object_content.length()) == replica::get_replica_size_from_storage(comm, target_object, resc_name, physical_path));

        // overwrite second replica with new contents
        {
            std::ios_base::openmode mode = std::ios_base::out | std::ios_base::trunc;
            io::client::default_transport tp{comm};
            io::odstream{tp, target_object, io::leaf_resource_name{resc_name.data()}, mode} << updated_object_content;
        }

        // show that the replica was updated
        REQUIRE(static_cast<rodsLong_t>(updated_object_content.length()) == replica::get_replica_size_from_storage(comm, target_object, resc_name, physical_path));
    }

    SECTION("data objects")
    {
        rodsLong_t data_id;

        {
            auto [obj, obj_lm] = data_object::make_data_object_proxy(comm, target_object);
            data_id = obj.data_id();
            REQUIRE(obj.logical_path() == target_object.c_str());
        }

        {
            auto [obj, obj_lm] = data_object::make_data_object_proxy(comm, data_id);
            CHECK(obj.logical_path() == target_object.c_str());
            CHECK(data_id == obj.data_id());
        }
    }

    SECTION("invalid inputs")
    {
        const char* very_long_string = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

        constexpr int invalid_replica_number = -1;

        CHECK_THROWS(replica::replica_exists(comm, std::string{}, second_replica));
        CHECK_THROWS(replica::is_replica_empty(comm, std::string{}, second_replica));
        CHECK_THROWS(replica::replica_checksum(comm, std::string{}, second_replica));
        CHECK_THROWS(replica::last_write_time(comm, std::string{}, second_replica));
        CHECK_THROWS(replica::replica_status(comm, std::string{}, second_replica));
        CHECK_THROWS(replica::get_replica_size_from_storage(comm, std::string{}, second_replica));
        CHECK_THROWS(replica::get_data_object_info(comm, std::string{}, second_replica));

        CHECK_THROWS(replica::replica_exists(comm, very_long_string, second_replica));
        CHECK_THROWS(replica::is_replica_empty(comm, very_long_string, second_replica));
        CHECK_THROWS(replica::replica_checksum(comm, very_long_string, second_replica));
        CHECK_THROWS(replica::last_write_time(comm, very_long_string, second_replica));
        CHECK_THROWS(replica::replica_status(comm, very_long_string, second_replica));
        CHECK_THROWS(replica::get_replica_size_from_storage(comm, very_long_string, second_replica));
        CHECK_THROWS(replica::get_data_object_info(comm, very_long_string, second_replica));

        CHECK_THROWS(replica::replica_exists(comm, sandbox, second_replica));
        CHECK_THROWS(replica::is_replica_empty(comm, sandbox, second_replica));
        CHECK_THROWS(replica::replica_checksum(comm, sandbox, second_replica));
        CHECK_THROWS(replica::last_write_time(comm, sandbox, second_replica));
        CHECK_THROWS(replica::replica_status(comm, sandbox, second_replica));
        CHECK_THROWS(replica::get_replica_size_from_storage(comm, sandbox, second_replica));
        CHECK_THROWS(replica::get_data_object_info(comm, sandbox, second_replica));

        CHECK_THROWS(replica::replica_exists(comm, target_object, invalid_replica_number));
        CHECK_THROWS(replica::is_replica_empty(comm, target_object, invalid_replica_number));
        CHECK_THROWS(replica::replica_checksum(comm, target_object, invalid_replica_number));
        CHECK_THROWS(replica::last_write_time(comm, target_object, invalid_replica_number));
        CHECK_THROWS(replica::replica_status(comm, target_object, invalid_replica_number));
        CHECK_THROWS(replica::get_replica_size_from_storage(comm, target_object, invalid_replica_number));
        CHECK_THROWS(replica::get_data_object_info(comm, target_object, invalid_replica_number));

        CHECK_THROWS(replica::replica_exists(comm, std::string{}, resc_name));
        CHECK_THROWS(replica::is_replica_empty(comm, std::string{}, resc_name));
        CHECK_THROWS(replica::replica_checksum(comm, std::string{}, resc_name));
        CHECK_THROWS(replica::last_write_time(comm, std::string{}, resc_name));
        CHECK_THROWS(replica::replica_status(comm, std::string{}, resc_name));
        CHECK_THROWS(replica::get_replica_size_from_storage(comm, std::string{}, resc_name));
        CHECK_THROWS(replica::get_data_object_info(comm, std::string{}, resc_name));

        CHECK_THROWS(replica::replica_exists(comm, very_long_string, resc_name));
        CHECK_THROWS(replica::is_replica_empty(comm, very_long_string, resc_name));
        CHECK_THROWS(replica::replica_checksum(comm, very_long_string, resc_name));
        CHECK_THROWS(replica::last_write_time(comm, very_long_string, resc_name));
        CHECK_THROWS(replica::replica_status(comm, very_long_string, resc_name));
        CHECK_THROWS(replica::get_replica_size_from_storage(comm, very_long_string, resc_name));
        CHECK_THROWS(replica::get_data_object_info(comm, very_long_string, resc_name));

        CHECK_THROWS(replica::replica_exists(comm, sandbox, resc_name));
        CHECK_THROWS(replica::is_replica_empty(comm, sandbox, resc_name));
        CHECK_THROWS(replica::replica_checksum(comm, sandbox, resc_name));
        CHECK_THROWS(replica::last_write_time(comm, sandbox, resc_name));
        CHECK_THROWS(replica::replica_status(comm, sandbox, resc_name));
        CHECK_THROWS(replica::get_replica_size_from_storage(comm, sandbox, resc_name));
        CHECK_THROWS(replica::get_data_object_info(comm, sandbox, resc_name));

        CHECK_THROWS(replica::replica_exists(comm, target_object, std::string{}));
        CHECK_THROWS(replica::is_replica_empty(comm, target_object, std::string{}));
        CHECK_THROWS(replica::replica_checksum(comm, target_object, std::string{}));
        CHECK_THROWS(replica::last_write_time(comm, target_object, std::string{}));
        CHECK_THROWS(replica::replica_status(comm, target_object, std::string{}));
        CHECK_THROWS(replica::get_replica_size_from_storage(comm, target_object, std::string{}));
        CHECK_THROWS(replica::get_data_object_info(comm, target_object, std::string{}));

        CHECK_THROWS(replica::get_replica_size_from_storage(comm, std::string{}, resc_name));
        CHECK_THROWS(replica::get_replica_size_from_storage(comm, very_long_string, resc_name));
        CHECK_THROWS(replica::get_replica_size_from_storage(comm, sandbox, resc_name));
        CHECK_THROWS(replica::get_replica_size_from_storage(comm, target_object, std::string{}));

        CHECK(std::nullopt == replica::to_leaf_resource(comm, target_object, 42));
        CHECK(std::nullopt == replica::to_replica_number(comm, target_object, "nope"));
    }
}

