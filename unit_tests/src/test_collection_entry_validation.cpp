#include <catch2/catch_all.hpp>

#include "irods/collection_entry_validation.hpp"

#include <string>

TEST_CASE("#9079: Reject invalid logical path pathname elements")
{
    char collection[] = "/tempZone/home/alice/coll2204";
    char name[] = "canary2204-canary.txt";
    collEnt_t entry{};
    entry.objType = DATA_OBJ_T;
    entry.collName = collection;
    entry.dataName = name;
    REQUIRE(irods::is_valid_collection_entry(entry));

    for (const auto* value :
         {"../../vic2204/newfile", "../../../etc/cron.d/pwn2204", "/tmp/payload", "nested/file", ".", "..", ""})
    {
        CAPTURE(value);
        std::string malicious_name{value};
        entry.dataName = malicious_name.data();
        CHECK_FALSE(irods::is_valid_collection_entry(entry));
    }

    entry.objType = COLL_OBJ_T;
    entry.dataName = nullptr;
    REQUIRE(irods::is_valid_collection_entry(entry));
    for (const auto* value : {"/tempZone/home/alice/..", "/tempZone/home/alice/.", "/tempZone/home/alice/../escape"}) {
        CAPTURE(value);
        std::string malicious_collection{value};
        entry.collName = malicious_collection.data();
        CHECK_FALSE(irods::is_valid_collection_entry(entry));
    }

    CHECK_FALSE(irods::is_valid_child_name(std::string_view{"file\0suffix", 11}));
}
