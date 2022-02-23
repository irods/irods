#include <catch2/catch.hpp>

#include "irods/with_durability.hpp"

#include <chrono>

TEST_CASE("with_durability")
{
    using namespace irods::experimental;
    using namespace std::chrono_literals;

    SECTION("use default options")
    {
        int counter = 0;
        const auto start = std::chrono::steady_clock::now();

        const auto exec_result = with_durability([&] {
            if (counter < 1) {
                ++counter;
                return execution_result::failure;
            }

            return translator::equals_zero(0);
        });

        const auto elapsed_time = std::chrono::steady_clock::now() - start;

        REQUIRE(counter == 1);
        REQUIRE(elapsed_time >= 1s);
        REQUIRE(exec_result == execution_result::success);
    }

    SECTION("use explicit options")
    {
        int counter = 0;
        const auto start = std::chrono::steady_clock::now();

        // Demonstrates out-of-order durability_options.
        const auto exec_result = with_durability(delay_multiplier{2.f}, retries{2}, [&] {
            if (counter < 2) {
                ++counter;
                return execution_result::failure;
            }

            return translator::equals_zero(0);
        });

        const auto elapsed_time = std::chrono::steady_clock::now() - start;

        REQUIRE(counter == 2);
        REQUIRE(elapsed_time >= 3s);
        REQUIRE(exec_result == execution_result::success);
    }

    SECTION("throw exception when constructing durability_options with invalid arguments")
    {
        const char* expected_msg = "Value must be greater than or equal to zero.";
        REQUIRE_THROWS(durability_options{retries{-1}}, expected_msg);
        REQUIRE_THROWS(durability_options{delay{-1s}}, expected_msg);
        REQUIRE_THROWS(durability_options{delay_multiplier{-1.f}}, expected_msg);
    }

    SECTION("throw exception when updating durability_options with invalid arguments")
    {
        durability_options opts;
        const char* expected_msg = "Value must be greater than or equal to zero.";
        REQUIRE_THROWS(opts.set_retries(-1), expected_msg);
        REQUIRE_THROWS(opts.set_delay(-1s), expected_msg);
        REQUIRE_THROWS(opts.set_delay_multiplier(-1.f), expected_msg);
    }
}

