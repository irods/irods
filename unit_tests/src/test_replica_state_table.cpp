#include "catch.hpp"

#include "data_object_proxy.hpp"
#include "irods_at_scope_exit.hpp"
#include "replica_state_table.hpp"

#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <utility>
#include <vector>

namespace
{
    using state_type = irods::replica_state_table::state_type;
    namespace data_object = irods::experimental::data_object;

    constexpr int REPLICA_COUNT       = 3;
    constexpr std::uint64_t SIZE_1    = 4000;
    constexpr std::uint64_t SIZE_2    = 9999;
    constexpr std::uint64_t DATA_ID_1 = 10101;
    constexpr std::uint64_t DATA_ID_2 = 20202;
    const std::string LOGICAL_PATH_1  = "/tempZone/home/rods/foo";

    auto generate_data_object(
        std::string_view _logical_path,
        const std::uint64_t _data_id,
        const std::uint64_t _size) -> std::array<DataObjInfo*, REPLICA_COUNT>
    {
        std::array<DataObjInfo*, REPLICA_COUNT> replicas;
        DataObjInfo* head{};
        DataObjInfo* prev{};
        for (int i = 0; i < REPLICA_COUNT; ++i) {
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
            REQUIRE(_logical_path == r->objPath);
            REQUIRE(_data_id      == r->dataId);
            REQUIRE(i             == r->replNum);
            REQUIRE(_size         == r->dataSize);
            REQUIRE(i             == r->rescId);
        }

        return replicas;
    } // generate_data_object
}

TEST_CASE("replica state table", "[basic]")
{
    //const auto replicas = generate_data_object(LOGICAL_PATH_1, DATA_ID_1, SIZE_1);
    //const auto* head = replicas[0];
    auto replicas = generate_data_object(LOGICAL_PATH_1, DATA_ID_1, SIZE_1);
    auto* head = replicas[0];

    // ensure that the linked list is valid and will be freed upon exit
    REQUIRE(head);
    const auto replica_list_lm = irods::experimental::lifetime_manager{*head};
    const auto obj = irods::experimental::data_object::make_data_object_proxy(*head);

    auto& rst = irods::replica_state_table::instance();

    // create before entry for the data object
    REQUIRE_NOTHROW(rst.insert(obj));
    REQUIRE(rst.contains(LOGICAL_PATH_1));
    REQUIRE(REPLICA_COUNT == rst.at(LOGICAL_PATH_1).size());

    SECTION("access by logical path (data object)")
    {
        const auto data_object_json = rst.at(LOGICAL_PATH_1);

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
        constexpr int replica_number = REPLICA_COUNT - 1;
        const auto original_replica = irods::experimental::replica::make_replica_proxy(*replicas.at(replica_number));
        const auto replica_json = rst.at(LOGICAL_PATH_1, replica_number, state_type::before);
        const auto [before, lm] = irods::experimental::replica::make_replica_proxy(LOGICAL_PATH_1, replica_json);

        CHECK(before.data_id()        == original_replica.data_id());
        CHECK(before.logical_path()   == original_replica.logical_path());
        CHECK(before.replica_number() == original_replica.replica_number());
        CHECK(before.size()           == original_replica.size());
    }

    SECTION("modify entry by whole replica")
    {
        constexpr int target_replica_number = 1;

        auto [r, r_lm] = irods::experimental::replica::duplicate_replica(*replicas.at(target_replica_number));

        r.data_id(DATA_ID_2);
        r.size(SIZE_2);
        r.replica_status(STALE_REPLICA);

        REQUIRE_NOTHROW(rst.update(LOGICAL_PATH_1, r));

        for (int i = 0; i < REPLICA_COUNT; ++i) {
            const auto original_replica = irods::experimental::replica::make_replica_proxy(*replicas.at(i));
            const auto replica_json = rst.at(LOGICAL_PATH_1, original_replica.replica_number(), state_type::after);
            const auto [replica, lm] = irods::experimental::replica::make_replica_proxy(LOGICAL_PATH_1, replica_json);

            if (target_replica_number == i) {
                // ensure that the after states updated
                CHECK(replica.data_id()        == DATA_ID_2);
                CHECK(replica.logical_path()   == LOGICAL_PATH_1);
                CHECK(replica.replica_number() == target_replica_number);
                CHECK(replica.size()           == SIZE_2);
                CHECK(replica.replica_status() == STALE_REPLICA);
                CHECK(replica.resource_id()    == original_replica.resource_id());

                // ensure that "before" states remained the same
                const auto replica_json = rst.at(LOGICAL_PATH_1, original_replica.replica_number(), state_type::before);
                const auto [before, before_lm] = irods::experimental::replica::make_replica_proxy(LOGICAL_PATH_1, replica_json);
                CHECK(before.data_id()        == original_replica.data_id());
                CHECK(before.logical_path()   == original_replica.logical_path());
                CHECK(before.replica_number() == original_replica.replica_number());
                CHECK(before.size()           == original_replica.size());
                CHECK(before.replica_status() == original_replica.replica_status());
                CHECK(before.resource_id()    == original_replica.resource_id());
            }
            else {
                // ensure that the other replicas did not update
                CHECK(replica.data_id()        == original_replica.data_id());
                CHECK(replica.logical_path()   == original_replica.logical_path());
                CHECK(replica.replica_number() == original_replica.replica_number());
                CHECK(replica.size()           == original_replica.size());
                CHECK(replica.replica_status() == original_replica.replica_status());
                CHECK(replica.resource_id()    == original_replica.resource_id());
            }
        }

        SECTION("property accessors")
        {
            const auto& rst = irods::replica_state_table::instance();
            CHECK(target_replica_number == std::stoi(rst.get_property(LOGICAL_PATH_1, target_replica_number, "data_repl_num", state_type::after)));
            CHECK(DATA_ID_2 == std::stoll(rst.get_property(LOGICAL_PATH_1, target_replica_number, "data_id", state_type::after)));
            CHECK(SIZE_2 == std::stoll(rst.get_property(LOGICAL_PATH_1, target_replica_number, "data_size", state_type::after)));
            CHECK(STALE_REPLICA == std::stoi(rst.get_property(LOGICAL_PATH_1, target_replica_number, "data_is_dirty", state_type::after)));
        }
    }

    SECTION("modify entry by replica number")
    {
        constexpr int target_replica_number = 1;

        const nlohmann::json updates{{
            "replicas", nlohmann::json{
                {"data_id", std::to_string(DATA_ID_2)},
                {"data_size", std::to_string(SIZE_2)},
                {"data_is_dirty", std::to_string(STALE_REPLICA)}
            }
        }};

        REQUIRE_NOTHROW(rst.update(LOGICAL_PATH_1, target_replica_number, updates));

        for (int i = 0; i < REPLICA_COUNT; ++i) {
            const auto original_replica = irods::experimental::replica::make_replica_proxy(*replicas.at(i));
            const auto replica_json = rst.at(LOGICAL_PATH_1, original_replica.replica_number(), state_type::after);
            const auto [replica, lm] = irods::experimental::replica::make_replica_proxy(LOGICAL_PATH_1, replica_json);

            if (target_replica_number == i) {
                // ensure that the after states updated
                CHECK(replica.data_id()        == DATA_ID_2);
                CHECK(replica.logical_path()   == LOGICAL_PATH_1);
                CHECK(replica.replica_number() == target_replica_number);
                CHECK(replica.size()           == SIZE_2);
                CHECK(replica.replica_status() == STALE_REPLICA);

                // ensure that "before" states remained the same
                const auto replica_json = rst.at(LOGICAL_PATH_1, original_replica.replica_number(), state_type::before);
                const auto [before, before_lm] = irods::experimental::replica::make_replica_proxy(LOGICAL_PATH_1, replica_json);
                CHECK(before.data_id()        == original_replica.data_id());
                CHECK(before.logical_path()   == original_replica.logical_path());
                CHECK(before.replica_number() == original_replica.replica_number());
                CHECK(before.size()           == original_replica.size());
                CHECK(before.replica_status() == original_replica.replica_status());
            }
            else {
                // ensure that the other replicas did not update
                CHECK(replica.data_id()        == original_replica.data_id());
                CHECK(replica.logical_path()   == original_replica.logical_path());
                CHECK(replica.replica_number() == original_replica.replica_number());
                CHECK(replica.size()           == original_replica.size());
                CHECK(replica.replica_status() == original_replica.replica_status());
            }
        }

        SECTION("property accessors")
        {
            const auto& rst = irods::replica_state_table::instance();
            CHECK(target_replica_number == std::stoi(rst.get_property(LOGICAL_PATH_1, target_replica_number, "data_repl_num", state_type::after)));
            CHECK(DATA_ID_2 == std::stoll(rst.get_property(LOGICAL_PATH_1, target_replica_number, "data_id", state_type::after)));
            CHECK(SIZE_2 == std::stoll(rst.get_property(LOGICAL_PATH_1, target_replica_number, "data_size", state_type::after)));
            CHECK(STALE_REPLICA == std::stoi(rst.get_property(LOGICAL_PATH_1, target_replica_number, "data_is_dirty", state_type::after)));
        }
    }

#if 0
    SECTION("access by leaf resource name")
    {
        // TODO: this requires a running iRODS server and catalog (not a unit test) -- skip
    }

    SECTION("update by leaf resource name")
    {
        // TODO: this requires a running iRODS server and catalog (not a unit test) -- skip
    }
#endif

    SECTION("track multiple data objects")
    {
        const std::string LOGICAL_PATH_2  = "/tempZone/home/rods/goo";

        //const auto replicas_2 = generate_data_object(LOGICAL_PATH_2, DATA_ID_2, SIZE_2);
        //const auto* head_2 = replicas_2[0];
        auto replicas_2 = generate_data_object(LOGICAL_PATH_2, DATA_ID_2, SIZE_2);
        auto* head_2 = replicas_2[0];

        // ensure that the linked list is valid and will be freed upon exit
        REQUIRE(head_2);
        const auto replica_list_2_lm = irods::experimental::lifetime_manager{*head_2};

        const auto obj_2 = irods::experimental::data_object::make_data_object_proxy(*head_2);

        REQUIRE_NOTHROW(rst.insert(obj_2));
        REQUIRE(rst.contains(LOGICAL_PATH_2));

        for (int i = 0; i < REPLICA_COUNT; ++i) {
            const auto original_replica = irods::experimental::replica::make_replica_proxy(*replicas_2.at(i));

            const auto replica_json = rst.at(LOGICAL_PATH_2, original_replica.replica_number(), state_type::before);
            const auto [before, lm] = irods::experimental::replica::make_replica_proxy(LOGICAL_PATH_2, replica_json);

            // ensure that the replicas match
            CHECK(before.data_id()        == original_replica.data_id());
            CHECK(before.logical_path()   == original_replica.logical_path());
            CHECK(before.replica_number() == original_replica.replica_number());
            CHECK(before.size()           == original_replica.size());
        }

        CHECK_NOTHROW(rst.erase(LOGICAL_PATH_2));
        CHECK_FALSE(rst.contains(LOGICAL_PATH_2));
    }

    SECTION("insert replica for existing entry")
    {
        constexpr int REPL_NUM = 7;

        auto [proxy, lm] = irods::experimental::replica::make_replica_proxy();
        proxy.logical_path(LOGICAL_PATH_1);
        proxy.replica_number(REPL_NUM);

        CHECK_FALSE(rst.contains(proxy.logical_path(), proxy.replica_number()));

        REQUIRE_NOTHROW(rst.insert(proxy.logical_path(), proxy));

        CHECK(rst.contains(proxy.logical_path(), proxy.replica_number()));
        CHECK(REPLICA_COUNT + 1 == rst.at(proxy.logical_path()).size());
        CHECK(REPL_NUM == std::stoi(rst.get_property(proxy.logical_path(), REPL_NUM, "data_repl_num")));

        CHECK_NOTHROW(rst.erase(LOGICAL_PATH_1));
        CHECK(rst.contains(LOGICAL_PATH_1));
    }

    SECTION("test ref_count")
    {
        REQUIRE(rst.contains(LOGICAL_PATH_1));
        REQUIRE_NOTHROW(rst.insert(obj));
        CHECK_NOTHROW(rst.erase(LOGICAL_PATH_1));
        CHECK(rst.contains(LOGICAL_PATH_1));
    }

    CHECK_NOTHROW(rst.erase(LOGICAL_PATH_1));
    CHECK_FALSE(rst.contains(LOGICAL_PATH_1));
    rst.deinit();
}

TEST_CASE("invalid_keys", "[basic]")
{
    auto& rst = irods::replica_state_table::instance();
    CHECK_FALSE(rst.contains("nope"));
    CHECK_THROWS(rst.at("nope"));
    CHECK_THROWS(rst.get_property("whatever", 0, "whatever", state_type::both));
    rst.deinit();
}

