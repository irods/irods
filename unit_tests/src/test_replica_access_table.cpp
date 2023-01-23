#include <catch2/catch.hpp>

#include "irods/replica_access_table.hpp"
#include "irods/irods_at_scope_exit.hpp"

#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <vector>

namespace rat = irods::experimental::replica_access_table;

struct access_info
{
    std::string token;
    int data_id;
    int replica_number;
    pid_t pid;
};

auto insert_new_entry(access_info& info) -> void;
auto append_to_entry(access_info& info, const std::string& token) -> void;

TEST_CASE("replica_access_table")
{
    std::vector<access_info> infos{
        {"", 12000, 3, 4000},
        {"", 12000, 3, 5000},
        {"", 12000, 2, 6000}
    };
    
    rat::init("irods_replica_access_table_test", 100'000);
    irods::at_scope_exit cleanup{[] { rat::deinit(); }};

    // Insert a new entry into the table.
    REQUIRE_NOTHROW(insert_new_entry(infos[0]));

    // Attempting to insert an entry that already exists generates an exception.
    REQUIRE_THROWS(insert_new_entry(infos[0]), "replica_access_table: Entry already exists");

    // Because the next entry is referencing the same replica, it must use the token returned
    // by the first call to insert.
    REQUIRE_NOTHROW(append_to_entry(infos[1], infos[0].token));

    // Insert a completely new entry because a different replica is being referenced.
    REQUIRE_NOTHROW(insert_new_entry(infos[2]));

    // Verify the token relationships.
    // Entries referencing the same data id and replica number must share the same token.
    REQUIRE(infos[0].token == infos[1].token);
    REQUIRE(infos[0].token != infos[2].token);

    // Erase a PID which does not exist in the replica access table. This should be a no-op.
    CHECK_NOTHROW(rat::erase_pid(7000));

    SECTION("erase and restore entry with single PID")
    {
        auto& info = infos[2];

        auto entry = rat::erase_pid(info.token, info.pid);
        REQUIRE(entry);

        auto [token, data_id, replica_number, pid] = *entry;
        REQUIRE_FALSE(rat::contains(data_id, replica_number));
        REQUIRE_FALSE(rat::contains(info.token, data_id, replica_number));

        rat::restore(*entry);
        REQUIRE(rat::contains(data_id, replica_number));
        REQUIRE(rat::contains(info.token, data_id, replica_number));
    }

    SECTION("erase and restore entry with multiple PIDs")
    {
        auto entry_1 = rat::erase_pid(infos[0].token, infos[0].pid);
        REQUIRE(entry_1);

        // Because there are two PIDs associated with the same (token, data_id, replica_number)
        // tuple, the entry still exists in the table. However, only one PID remains in the PID list.
        auto [token, data_id, replica_number, pid] = *entry_1;
        REQUIRE(rat::contains(data_id, replica_number));
        REQUIRE(rat::contains(infos[0].token, data_id, replica_number));

        // Erasing the last PID will cause the entry to be permanently deleted from the table.
        auto entry_2 = rat::erase_pid(infos[1].token, infos[1].pid);
        REQUIRE(entry_2);
        REQUIRE_FALSE(rat::contains(data_id, replica_number));
        REQUIRE_FALSE(rat::contains(infos[0].token, data_id, replica_number));

        // Show that the restore member function restores the previously removed entry.
        // Restoring the entry does not generate a new token.
        rat::restore(*entry_1);
        rat::restore(*entry_2);
        REQUIRE(rat::contains(data_id, replica_number));
        REQUIRE(rat::contains(infos[0].token, data_id, replica_number));
    }

    for (auto&& info : infos) {
        REQUIRE(rat::contains(info.data_id, info.replica_number));
        REQUIRE(rat::contains(info.token, info.data_id, info.replica_number));
    }

    for (auto&& info : infos) {
        rat::erase_pid(info.token, info.pid);
    }

    for (auto&& info : infos) {
        REQUIRE_FALSE(rat::contains(info.data_id, info.replica_number));
        REQUIRE_FALSE(rat::contains(info.token, info.data_id, info.replica_number));
    }
}

auto insert_new_entry(access_info& info) -> void
{
    info.token = rat::create_new_entry(info.data_id, info.replica_number, info.pid);
}

auto append_to_entry(access_info& info, const std::string& token) -> void
{
    info.token = token;
    rat::append_pid(info.token, info.data_id, info.replica_number, info.pid);
}

