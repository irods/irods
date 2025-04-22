#include <catch2/catch_all.hpp>

#include "irods/access_time_queue.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_exception.hpp"

#include <string_view>

namespace atq = irods::access_time_queue;

TEST_CASE("access_time_queue")
{
    constexpr auto queue_size = 3;
    atq::init("irods_access_time_queue_test", queue_size);
    irods::at_scope_exit deinit_queue{[] { atq::deinit(); }};

    SECTION("enqueue one update, dequeue one update")
    {
        constexpr auto data_id = 100;
        constexpr auto replica_number = 3;
        constexpr auto atime = std::string_view{"00000000042"};

        atq::access_time_data in{};
        in.data_id = data_id;
        in.replica_number = replica_number;
        atime.copy(in.last_accessed, sizeof(atq::access_time_data::last_accessed) - 1);
        REQUIRE(atq::try_enqueue(in));

        atq::access_time_data out{};
        REQUIRE(atq::try_dequeue(out));

        REQUIRE(data_id == out.data_id);
        REQUIRE(replica_number == out.replica_number);
        REQUIRE(atime == out.last_accessed);
    }

    SECTION("queue returns number of entries")
    {
        REQUIRE(atq::number_of_queued_updates() == 0);

        REQUIRE(atq::try_enqueue({}));
        REQUIRE(atq::number_of_queued_updates() == 1);

        REQUIRE(atq::try_enqueue({}));
        REQUIRE(atq::number_of_queued_updates() == 2);

        REQUIRE(atq::try_enqueue({}));
        REQUIRE(atq::number_of_queued_updates() == 3);

        atq::access_time_data data{};
        REQUIRE(atq::try_dequeue(data));
        REQUIRE(atq::number_of_queued_updates() == 2);

        REQUIRE(atq::try_dequeue(data));
        REQUIRE(atq::number_of_queued_updates() == 1);

        REQUIRE(atq::try_dequeue(data));
        REQUIRE(atq::number_of_queued_updates() == 0);
    }

    SECTION("remove entries from an empty queue")
    {
        atq::access_time_data data{};
        REQUIRE_FALSE(atq::try_dequeue(data));
        REQUIRE_FALSE(atq::try_dequeue(data));
        REQUIRE_FALSE(atq::try_dequeue(data));
        REQUIRE(atq::number_of_queued_updates() == 0);
    }

    SECTION("add entries to a full queue")
    {
        // Fill the queue.
        REQUIRE(atq::try_enqueue({}));
        REQUIRE(atq::try_enqueue({}));
        REQUIRE(atq::try_enqueue({}));
        REQUIRE(atq::number_of_queued_updates() == 3);

        // Try to add more items to the queue even though it's full.
        REQUIRE_FALSE(atq::try_enqueue({}));
        REQUIRE_FALSE(atq::try_enqueue({}));
        REQUIRE_FALSE(atq::try_enqueue({}));
        REQUIRE(atq::number_of_queued_updates() == 3);

        // Remove the entries.
        atq::access_time_data data{};
        REQUIRE(atq::try_dequeue(data));
        REQUIRE(atq::try_dequeue(data));
        REQUIRE(atq::try_dequeue(data));
        REQUIRE(atq::number_of_queued_updates() == 0);
    }
}

TEST_CASE("invalid initialization")
{
    SECTION("empty queue name prefix")
    {
        REQUIRE_THROWS_AS([] { atq::init("", 1); }(), irods::exception);
    }

    SECTION("queue name prefix starts with character other than alphabetic or underscore")
    {
        auto matcher = Catch::Matchers::ContainsSubstring(
            "init: Access time queue name prefix violates name requirement: [_a-zA-Z][_a-zA-Z0-9]*");

        CHECK_THROWS_WITH([] { atq::init("1_number", 1); }(), matcher);
        CHECK_THROWS_WITH([] { atq::init("-_hyphen", 1); }(), matcher);
        CHECK_THROWS_WITH([] { atq::init("<_less_than_sign", 1); }(), matcher);
        CHECK_THROWS_WITH([] { atq::init(" _space", 1); }(), matcher);
        CHECK_THROWS_WITH([] { atq::init("£_utf-8", 1); }(), matcher);
    }

    SECTION("queue size greater than 500000")
    {
        REQUIRE_THROWS_AS([] { atq::init("good_name", 500001); }(), irods::exception);
    }
}
