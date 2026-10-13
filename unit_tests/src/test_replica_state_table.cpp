#include <catch2/catch_all.hpp>

#include "irods/data_object_proxy.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/logical_locking.hpp"
#include "irods/replica.hpp"
#include "irods/replica_state_table.hpp"
#include "irods/rodsType.h"

#include <sys/types.h>
#include <unistd.h>

#include <charconv>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include <system_error>

namespace
{
    namespace data_object = irods::experimental::data_object;
    namespace rst = irods::replica_state_table;
    using replica_number_type = irods::experimental::replica::replica_number_type;

    constexpr replica_number_type REPLICA_COUNT = 3;
    constexpr rodsLong_t SIZE_1    = 4000;
    constexpr rodsLong_t SIZE_2    = 9999;
    constexpr rodsLong_t DATA_ID_1 = 10101;
    constexpr rodsLong_t DATA_ID_2 = 20202;
    const std::string LOGICAL_PATH_1  = "/tempZone/home/rods/foo";
    const std::string LOGICAL_PATH_2  = "/tempZone/home/rods/goo";
    const std::string UPDATED_COMMENT = "updated";

    auto generate_data_object(
        const std::uint64_t _data_id,
        std::string_view _logical_path,
        const std::uint64_t _size) -> std::array<DataObjInfo*, REPLICA_COUNT>
    {
        std::array<DataObjInfo*, REPLICA_COUNT> replicas;
        DataObjInfo* head{};
        DataObjInfo* prev{};
        for (replica_number_type i = 0; i < REPLICA_COUNT; ++i) {
            auto [proxy, lm] = irods::experimental::replica::make_replica_proxy();
            proxy.logical_path(_logical_path);
            proxy.size(_size);
            proxy.replica_number(i);
            proxy.data_id(_data_id);
            proxy.replica_status(GOOD_REPLICA);
            proxy.resource_id(i);

            DataObjInfo* curr = lm.release();
            if (!head) {
                head = curr;
            }
            else {
                prev->next = curr;
            }
            prev = curr;
            replicas[i] = curr;

            const auto& r = replicas[i];
            REQUIRE(_logical_path                     == r->objPath);
            REQUIRE(static_cast<rodsLong_t>(_data_id) == r->dataId);
            REQUIRE(i                                 == r->replNum);
            REQUIRE(static_cast<rodsLong_t>(_size)    == r->dataSize);
            REQUIRE(i                                 == r->rescId);
        }

        return replicas;
    } // generate_data_object
}

TEST_CASE("replica state table", "[basic]")
{
    rst::init();

    auto replicas = generate_data_object(DATA_ID_1, LOGICAL_PATH_1, SIZE_1);
    auto* head = replicas[0];

    // ensure that the linked list is valid and will be freed upon exit
    REQUIRE(head);
    const auto replica_list_lm = irods::experimental::lifetime_manager{*head};
    const auto obj = irods::experimental::data_object::make_data_object_proxy(*head);

    // create before entry for the data object
    REQUIRE(0 == rst::insert(obj));
    REQUIRE(rst::contains(DATA_ID_1));
    REQUIRE(REPLICA_COUNT == rst::at(DATA_ID_1).size());

    SECTION("access by logical path (data object)")
    {
        const auto data_object_json = rst::at(DATA_ID_1);

        REQUIRE(REPLICA_COUNT == data_object_json.size());

        // ensure that information matches
        for (int i = 0; i < REPLICA_COUNT; ++i) {
            const auto original_replica = irods::experimental::replica::make_replica_proxy(*replicas.at(i));

            const auto& replica_json = data_object_json.at(i).at("before");
            const auto [before, lm] = irods::experimental::replica::make_replica_proxy(LOGICAL_PATH_1, replica_json);

            CHECK(before.data_id()        == original_replica.data_id());
            CHECK(before.logical_path()   == original_replica.logical_path());
            CHECK(before.replica_number() == original_replica.replica_number());
            CHECK(before.size()           == original_replica.size());
        }
    }

    SECTION("access by replica number")
    {
        constexpr replica_number_type replica_number = REPLICA_COUNT - 1;
        const auto original_replica = irods::experimental::replica::make_replica_proxy(*replicas.at(replica_number));
        const auto replica_json = rst::at(DATA_ID_1, replica_number, rst::state_type::before);
        const auto [before, lm] = irods::experimental::replica::make_replica_proxy(LOGICAL_PATH_1, replica_json);

        CHECK(before.data_id()        == original_replica.data_id());
        CHECK(before.logical_path()   == original_replica.logical_path());
        CHECK(before.replica_number() == original_replica.replica_number());
        CHECK(before.size()           == original_replica.size());
    }

    SECTION("modify entry by whole replica")
    {
        constexpr replica_number_type target_replica_number = 1;

        auto [r, r_lm] = irods::experimental::replica::duplicate_replica(*replicas.at(target_replica_number));

        r.comments(UPDATED_COMMENT);
        r.size(SIZE_2);
        r.replica_status(STALE_REPLICA);

        REQUIRE_NOTHROW(rst::update(DATA_ID_1, r));

        for (int i = 0; i < REPLICA_COUNT; ++i) {
            const auto original_replica = irods::experimental::replica::make_replica_proxy(*replicas.at(i));
            const auto replica_json = rst::at(DATA_ID_1, original_replica.replica_number(), rst::state_type::after);
            const auto [replica, lm] = irods::experimental::replica::make_replica_proxy(LOGICAL_PATH_1, replica_json);

            if (target_replica_number == i) {
                // ensure that the after states updated
                CHECK(replica.data_id()        == DATA_ID_1);
                CHECK(replica.logical_path()   == LOGICAL_PATH_1);
                CHECK(replica.replica_number() == target_replica_number);
                CHECK(replica.size()           == SIZE_2);
                CHECK(replica.replica_status() == STALE_REPLICA);
                CHECK(replica.resource_id()    == original_replica.resource_id());
                CHECK(replica.comments()       == UPDATED_COMMENT);

                // ensure that "before" states remained the same
                const auto replica_json = rst::at(DATA_ID_1, original_replica.replica_number(), rst::state_type::before);
                const auto [before, before_lm] = irods::experimental::replica::make_replica_proxy(LOGICAL_PATH_1, replica_json);
                CHECK(before.data_id()        == original_replica.data_id());
                CHECK(before.logical_path()   == original_replica.logical_path());
                CHECK(before.replica_number() == original_replica.replica_number());
                CHECK(before.size()           == original_replica.size());
                CHECK(before.replica_status() == original_replica.replica_status());
                CHECK(before.resource_id()    == original_replica.resource_id());
                CHECK(before.comments()       == original_replica.comments());
            }
            else {
                // ensure that the other replicas did not update
                CHECK(replica.data_id()        == original_replica.data_id());
                CHECK(replica.logical_path()   == original_replica.logical_path());
                CHECK(replica.replica_number() == original_replica.replica_number());
                CHECK(replica.size()           == original_replica.size());
                CHECK(replica.replica_status() == original_replica.replica_status());
                CHECK(replica.resource_id()    == original_replica.resource_id());
                CHECK(replica.comments()       == original_replica.comments());
            }
        }

        SECTION("property accessors")
        {
            replica_number_type data_repl_num; // NOLINT(cppcoreguidelines-init-variables)
            const std::string data_repl_num_str = rst::get_property(DATA_ID_1, target_replica_number, "data_repl_num", rst::state_type::after);
            const auto* data_repl_num_b = data_repl_num_str.c_str();
            const auto* data_repl_num_e = data_repl_num_b + data_repl_num_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            const std::from_chars_result data_repl_num_fcres = std::from_chars(data_repl_num_b, data_repl_num_e, data_repl_num);
            CHECK(std::errc{} == data_repl_num_fcres.ec);
            CHECK(target_replica_number == data_repl_num);
            
            rodsLong_t data_id; // NOLINT(cppcoreguidelines-init-variables)
            const std::string data_id_str = rst::get_property(DATA_ID_1, target_replica_number, "data_id", rst::state_type::after);
            const auto* data_id_b = data_id_str.c_str();
            const auto* data_id_e = data_id_b + data_id_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            const std::from_chars_result data_id_fcres = std::from_chars(data_id_b, data_id_e, data_id);
            CHECK(std::errc{} == data_id_fcres.ec);
            CHECK(DATA_ID_1 == data_id);
            
            rodsLong_t data_size; // NOLINT(cppcoreguidelines-init-variables)
            const std::string data_size_str = rst::get_property(DATA_ID_1, target_replica_number, "data_size", rst::state_type::after);
            const auto* data_size_b = data_size_str.c_str();
            const auto* data_size_e = data_size_b + data_size_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            const std::from_chars_result data_size_fcres = std::from_chars(data_size_b, data_size_e, data_size);
            CHECK(std::errc{} == data_size_fcres.ec);
            CHECK(SIZE_2 == data_size);
            
            int replica_status; // NOLINT(cppcoreguidelines-init-variables)
            const std::string replica_status_str = rst::get_property(DATA_ID_1, target_replica_number, "data_is_dirty", rst::state_type::after);
            const auto* replica_status_b = replica_status_str.c_str();
            const auto* replica_status_e = replica_status_b + replica_status_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            const std::from_chars_result replica_status_fcres = std::from_chars(replica_status_b, replica_status_e, replica_status);
            CHECK(std::errc{} == replica_status_fcres.ec);
            CHECK(STALE_REPLICA == replica_status);
        }

    }

    SECTION("modify entry by replica number")
    {
        constexpr replica_number_type target_replica_number = 1;

        const auto updates = nlohmann::json{nlohmann::json{
            {"r_comment", UPDATED_COMMENT},
            {"data_size", std::to_string(SIZE_2)},
            {"data_is_dirty", std::to_string(STALE_REPLICA)}
        }};

        REQUIRE_NOTHROW(rst::update(DATA_ID_1, target_replica_number, updates));

        for (replica_number_type i = 0; i < REPLICA_COUNT; ++i) {
            const auto original_replica = irods::experimental::replica::make_replica_proxy(*replicas.at(i));
            const auto replica_json = rst::at(DATA_ID_1, original_replica.replica_number(), rst::state_type::after);
            const auto [replica, lm] = irods::experimental::replica::make_replica_proxy(LOGICAL_PATH_1, replica_json);

            if (target_replica_number == i) {
                // ensure that the after states updated
                CHECK(replica.data_id()        == DATA_ID_1);
                CHECK(replica.logical_path()   == LOGICAL_PATH_1);
                CHECK(replica.replica_number() == target_replica_number);
                CHECK(replica.size()           == SIZE_2);
                CHECK(replica.replica_status() == STALE_REPLICA);
                CHECK(replica.comments()       == UPDATED_COMMENT);

                // ensure that "before" states remained the same
                const auto replica_json = rst::at(DATA_ID_1, original_replica.replica_number(), rst::state_type::before);
                const auto [before, before_lm] = irods::experimental::replica::make_replica_proxy(LOGICAL_PATH_1, replica_json);
                CHECK(before.data_id()        == original_replica.data_id());
                CHECK(before.logical_path()   == original_replica.logical_path());
                CHECK(before.replica_number() == original_replica.replica_number());
                CHECK(before.size()           == original_replica.size());
                CHECK(before.replica_status() == original_replica.replica_status());
                CHECK(before.comments()       == original_replica.comments());
            }
            else {
                // ensure that the other replicas did not update
                CHECK(replica.data_id()        == original_replica.data_id());
                CHECK(replica.logical_path()   == original_replica.logical_path());
                CHECK(replica.replica_number() == original_replica.replica_number());
                CHECK(replica.size()           == original_replica.size());
                CHECK(replica.replica_status() == original_replica.replica_status());
                CHECK(replica.comments()       == original_replica.comments());
            }
        }

        SECTION("property accessors")
        {
            rodsLong_t data_id; // NOLINT(cppcoreguidelines-init-variables)
            const std::string data_id_str = rst::get_property(DATA_ID_1, target_replica_number, "data_id", rst::state_type::after);
            const auto* data_id_b = data_id_str.c_str();
            const auto* data_id_e = data_id_b + data_id_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            const std::from_chars_result data_id_fcres = std::from_chars(data_id_b, data_id_e, data_id);
            CHECK(std::errc{} != data_id_fcres.ec);
            CHECK(DATA_ID_1 == data_id);

            rodsLong_t data_size; // NOLINT(cppcoreguidelines-init-variables)
            const std::string data_size_str = rst::get_property(DATA_ID_1, target_replica_number, "data_size", rst::state_type::after);
            const auto* data_size_b = data_size_str.c_str();
            const auto* data_size_e = data_size_b + data_size_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            const std::from_chars_result data_size_fcres = std::from_chars(data_size_b, data_size_e, data_size);
            CHECK(std::errc{} != data_size_fcres.ec);
            CHECK(SIZE_2 == data_size);

            int replica_status; // NOLINT(cppcoreguidelines-init-variables)
            const std::string replica_status_str = rst::get_property(DATA_ID_1, target_replica_number, "data_is_dirty", rst::state_type::after);
            const auto* replica_status_b = replica_status_str.c_str();
            const auto* replica_status_e = replica_status_b + replica_status_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            const std::from_chars_result replica_status_fcres = std::from_chars(replica_status_b, replica_status_e, replica_status);
            CHECK(std::errc{} != replica_status_fcres.ec);
            CHECK(STALE_REPLICA == replica_status);
        }
    }

    SECTION("track multiple data objects")
    {
        auto replicas_2 = generate_data_object(DATA_ID_2, LOGICAL_PATH_2, SIZE_2);
        auto* head_2 = replicas_2[0];

        // ensure that the linked list is valid and will be freed upon exit
        REQUIRE(head_2);
        const auto replica_list_2_lm = irods::experimental::lifetime_manager{*head_2};

        const auto obj_2 = irods::experimental::data_object::make_data_object_proxy(*head_2);

        REQUIRE(0 == rst::insert(obj_2));
        REQUIRE(rst::contains(DATA_ID_2));

        for (replica_number_type i = 0; i < REPLICA_COUNT; ++i) {
            const auto original_replica = irods::experimental::replica::make_replica_proxy(*replicas_2.at(i));

            const auto replica_json = rst::at(DATA_ID_2, original_replica.replica_number(), rst::state_type::before);
            const auto [before, lm] = irods::experimental::replica::make_replica_proxy(LOGICAL_PATH_2, replica_json);

            // ensure that the replicas match
            CHECK(before.data_id()        == original_replica.data_id());
            CHECK(before.logical_path()   == original_replica.logical_path());
            CHECK(before.replica_number() == original_replica.replica_number());
            CHECK(before.size()           == original_replica.size());
        }
    }

    SECTION("insert replica for existing entry")
    {
        constexpr replica_number_type REPL_NUM = 7;

        auto [proxy, lm] = irods::experimental::replica::make_replica_proxy();
        proxy.data_id(DATA_ID_1);
        proxy.logical_path(LOGICAL_PATH_1);
        proxy.replica_number(REPL_NUM);

        CHECK_FALSE(rst::contains(proxy.data_id(), proxy.replica_number()));

        REQUIRE(0 == rst::insert(proxy));

        CHECK(rst::contains(proxy.data_id(), proxy.replica_number()));
        CHECK(REPLICA_COUNT + 1 == rst::at(proxy.data_id()).size());

        replica_number_type data_repl_num; // NOLINT(cppcoreguidelines-init-variables)
        const std::string data_repl_num_str = rst::get_property(proxy.data_id(), REPL_NUM, "data_repl_num");
        const auto* data_repl_num_b = data_repl_num_str.c_str();
        const auto* data_repl_num_e = data_repl_num_b + data_repl_num_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const std::from_chars_result data_repl_num_fcres = std::from_chars(data_repl_num_b, data_repl_num_e, data_repl_num);
        CHECK(std::errc{} != data_repl_num_fcres.ec);
        CHECK(REPL_NUM == data_repl_num);

        SECTION("erase entry for existing entry")
        {
            REQUIRE_NOTHROW(rst::erase(DATA_ID_1, proxy.replica_number()));

            REQUIRE(rst::contains(proxy.data_id()));
            CHECK(REPLICA_COUNT == rst::at(proxy.data_id()).size());
            CHECK(!rst::contains(proxy.data_id(), proxy.replica_number()));
        }
    }

    CHECK_NOTHROW(rst::erase(DATA_ID_1));
    CHECK_FALSE(rst::contains(DATA_ID_1));
    rst::deinit();
}

TEST_CASE("invalid_keys", "[invalid]")
{
    constexpr rst::key_type BOGUS_KEY = 42;
    CHECK_FALSE(rst::contains(BOGUS_KEY));
    CHECK_THROWS(rst::at(BOGUS_KEY));
    CHECK_THROWS(rst::get_property(BOGUS_KEY, 0, "whatever", rst::state_type::both));
    rst::deinit();
}

TEST_CASE("logical_locking", "[basic]")
{
    namespace ill = irods::logical_locking;
    using json = nlohmann::json;

    rst::init();

    auto replicas = generate_data_object(DATA_ID_1, LOGICAL_PATH_1, SIZE_1);
    auto* head = replicas[0];

    // ensure that the linked list is valid and will be freed upon exit
    REQUIRE(head);
    const auto replica_list_lm = irods::experimental::lifetime_manager{*head};
    const auto obj = irods::experimental::data_object::make_data_object_proxy(*head);

    // create before entry for the data object
    REQUIRE(0 == rst::insert(obj));
    REQUIRE(rst::contains(DATA_ID_1));
    REQUIRE(REPLICA_COUNT == rst::at(DATA_ID_1).size());

    for (replica_number_type i = 0; i < REPLICA_COUNT; ++i) {
        int replica_status; // NOLINT(cppcoreguidelines-init-variables)
        const std::string replica_status_str = rst::get_property(DATA_ID_1, i, "data_is_dirty", rst::state_type::after);
        const auto* replica_status_b = replica_status_str.c_str();
        const auto* replica_status_e = replica_status_b + replica_status_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const std::from_chars_result replica_status_fcres = std::from_chars(replica_status_b, replica_status_e, replica_status);
        CHECK(std::errc{} != replica_status_fcres.ec);
        REQUIRE(GOOD_REPLICA == replica_status);
    }

    SECTION("write lock")
    {
        constexpr replica_number_type locked_replica_number = 1;

        ill::lock(DATA_ID_1, locked_replica_number, ill::lock_type::write);

        for (replica_number_type i = 0; i < REPLICA_COUNT; ++i) {
            int replica_status; // NOLINT(cppcoreguidelines-init-variables)
            const std::string replica_status_str = rst::get_property(DATA_ID_1, i, "data_is_dirty", rst::state_type::after);
            const auto* replica_status_b = replica_status_str.c_str();
            const auto* replica_status_e = replica_status_b + replica_status_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            const std::from_chars_result replica_status_fcres = std::from_chars(replica_status_b, replica_status_e, replica_status);
            CHECK(std::errc{} != replica_status_fcres.ec);
            if (locked_replica_number == i) {
                CHECK(INTERMEDIATE_REPLICA == replica_status);
            }
            else {
                CHECK(WRITE_LOCKED == replica_status);
            }

            // the data status column should contain an original_status key with the original replica status as its value
            const auto data_status = json::parse(rst::get_property(DATA_ID_1, i, "data_status", rst::state_type::after));
            int original_status; // NOLINT(cppcoreguidelines-init-variables)
            const std::string original_status_str = data_status.at("original_status").get<std::string>();
            const auto* original_status_b = original_status_str.c_str();
            const auto* original_status_e = original_status_b + original_status_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            const std::from_chars_result original_status_fcres = std::from_chars(original_status_b, original_status_e, original_status);
            CHECK(std::errc{} != original_status_fcres.ec);
            REQUIRE(GOOD_REPLICA == original_status);
        }

        SECTION("restore status")
        {
            ill::unlock(DATA_ID_1, locked_replica_number, GOOD_REPLICA, ill::restore_status);

            for (replica_number_type i = 0; i < REPLICA_COUNT; ++i) {
                int replica_status; // NOLINT(cppcoreguidelines-init-variables)
                const std::string replica_status_str = rst::get_property(DATA_ID_1, i, "data_is_dirty", rst::state_type::after);
                const auto* replica_status_b = replica_status_str.c_str();
                const auto* replica_status_e = replica_status_b + replica_status_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                const std::from_chars_result replica_status_fcres = std::from_chars(replica_status_b, replica_status_e, replica_status);
                CHECK(std::errc{} != replica_status_fcres.ec);
                REQUIRE(GOOD_REPLICA == replica_status);

                // the data_status column should be empty again
                REQUIRE(rst::get_property(DATA_ID_1, i, "data_status", rst::state_type::after).empty());
            }
        }

        SECTION("unlock as stale")
        {
            ill::unlock(DATA_ID_1, locked_replica_number, GOOD_REPLICA, STALE_REPLICA);

            for (replica_number_type i = 0; i < REPLICA_COUNT; ++i) {
                int replica_status; // NOLINT(cppcoreguidelines-init-variables)
                const std::string replica_status_str = rst::get_property(DATA_ID_1, i, "data_is_dirty", rst::state_type::after);
                const auto* replica_status_b = replica_status_str.c_str();
                const auto* replica_status_e = replica_status_b + replica_status_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                const std::from_chars_result replica_status_fcres = std::from_chars(replica_status_b, replica_status_e, replica_status);
                CHECK(std::errc{} != replica_status_fcres.ec);
                if (locked_replica_number == i) {
                    // unlock() does not change the replica status of the target replica
                    CHECK(GOOD_REPLICA == replica_status);
                }
                else {
                    // all replicas other than the target replica should be the status passed to unlock() call
                    CHECK(STALE_REPLICA == replica_status);
                }

                // the data_status column should be empty again
                REQUIRE(rst::get_property(DATA_ID_1, i, "data_status", rst::state_type::after).empty());
            }
        }
    }

    SECTION("read lock")
    {
        constexpr replica_number_type locked_replica_number = 1;

        REQUIRE(0 == ill::lock(DATA_ID_1, locked_replica_number, ill::lock_type::read));

        for (replica_number_type i = 0; i < REPLICA_COUNT; ++i) {
            // All replicas should be read-locked, including the target replica
            int replica_status; // NOLINT(cppcoreguidelines-init-variables)
            const std::string replica_status_str = rst::get_property(DATA_ID_1, i, "data_is_dirty", rst::state_type::after);
            const auto* replica_status_b = replica_status_str.c_str();
            const auto* replica_status_e = replica_status_b + replica_status_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            const std::from_chars_result replica_status_fcres = std::from_chars(replica_status_b, replica_status_e, replica_status);
            CHECK(std::errc{} != replica_status_fcres.ec);
            CHECK(READ_LOCKED == replica_status);

            // the data status column should contain an original_status key with the original replica status as its value
            const auto data_status = json::parse(rst::get_property(DATA_ID_1, i, "data_status", rst::state_type::after));
            int original_status; // NOLINT(cppcoreguidelines-init-variables)
            const std::string original_status_str = data_status.at("original_status").get<std::string>();
            const auto* original_status_b = original_status_str.c_str();
            const auto* original_status_e = original_status_b + original_status_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            const std::from_chars_result original_status_fcres = std::from_chars(original_status_b, original_status_e, original_status);
            CHECK(std::errc{} != original_status_fcres.ec);
            CHECK(GOOD_REPLICA == original_status);

            // TODO: need to check for how many readers, who they are, etc.
        }

        REQUIRE(0 == ill::unlock(DATA_ID_1, locked_replica_number, GOOD_REPLICA, ill::restore_status));

        for (replica_number_type i = 0; i < REPLICA_COUNT; ++i) {
            int replica_status; // NOLINT(cppcoreguidelines-init-variables)
            const std::string replica_status_str = rst::get_property(DATA_ID_1, i, "data_is_dirty", rst::state_type::after);
            const auto* replica_status_b = replica_status_str.c_str();
            const auto* replica_status_e = replica_status_b + replica_status_str.size(); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            const std::from_chars_result replica_status_fcres = std::from_chars(replica_status_b, replica_status_e, replica_status);
            CHECK(std::errc{} != replica_status_fcres.ec);
            CHECK(GOOD_REPLICA == replica_status);

            // the data_status column should be empty again
            REQUIRE(rst::get_property(DATA_ID_1, i, "data_status", rst::state_type::after).empty());
        }
    }

    CHECK_NOTHROW(rst::erase(DATA_ID_1));
    CHECK_FALSE(rst::contains(DATA_ID_1));
    rst::deinit();
}

