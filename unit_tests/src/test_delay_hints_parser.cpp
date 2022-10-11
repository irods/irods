#include <catch2/catch.hpp>

#include "irods/rcMisc.h"

#include <fmt/format.h>
#include <fmt/chrono.h>

#include <map>
#include <string>
#include <string_view>
#include <array>

struct input
{
    std::string current_time;
    std::string frequency;
};

struct output
{
    int expected_action_code;
    std::string expected_next_time;
    std::string expected_frequency;
};

auto test_execution_frequency_handler(const input& _input, const output& _output)
{
    std::array<char, TIME_LEN> current_time{};
    std::copy(_input.current_time.cbegin(), _input.current_time.cend(), current_time.begin());

    constexpr int frequency_length{128};
    std::array<char, frequency_length> frequency{};
    std::copy(_input.frequency.cbegin(), _input.frequency.cend(), frequency.begin());

    std::array<char, TIME_LEN> next_time{};

    CHECK(getNextRepeatTime(current_time.data(), frequency.data(), next_time.data()) == _output.expected_action_code);
    CHECK(next_time.data() == _output.expected_next_time);
    CHECK(frequency.data() == _output.expected_frequency);
}

auto make_simple_directive_string(
    int _delay_duration,
    const std::string_view _delay_time_unit,
    const std::string_view _directive) -> std::string
{
    return fmt::format("{}{} {}", _delay_duration, _delay_time_unit, _directive);
}

auto make_time_directive_string(
    int _delay_duration,
    const std::string_view _delay_time_unit,
    const std::string_view _directive,
    const rodsLong_t _time) -> std::string
{
    const auto simple_directive_string{make_simple_directive_string(_delay_duration, _delay_time_unit, _directive)};
    return fmt::format("{} {}", simple_directive_string, _time);
}

auto make_counter_directive_string(
    int _delay_duration,
    const std::string_view _delay_time_unit,
    const std::string_view _directive,
    int _counter) -> std::string
{
    const auto base_directive_string{
        make_time_directive_string(_delay_duration, _delay_time_unit, _directive, _counter)};
    return fmt::format("{} TIMES", base_directive_string);
}

auto make_modified_counter_directive_string(
    int _delay_duration,
    const std::string_view _delay_time_unit,
    const std::string_view _directive,
    int _updated_counter) -> std::string
{
    const auto counter_directive_string{
        make_counter_directive_string(_delay_duration, _delay_time_unit, _directive, _updated_counter)};
    return fmt::format("{}. ORIGINAL TIMES={}", counter_directive_string, _updated_counter + 1);
}

TEST_CASE("#5503: getNextRepeatTime parses frequency correctly")
{
    struct test_case
    {
        input input;
        output output;
    };

    using namespace std::chrono_literals;

    const auto now = std::chrono::system_clock::now();
    const auto one_hour_in_future = (now + 1h).time_since_epoch().count();

    // clang-format off
    const std::map<std::string_view, test_case> map{
        {"empty directive", {
            // Input
           {
                .current_time = "0",
                .frequency    = "10s"
           },
            // Output
           {
                .expected_action_code = 0,
                .expected_next_time   = "10",
                .expected_frequency   = "10s"
           }
        }},
        {"REPEAT FOR EVER", {
            // Input
           {
                .current_time = "0",
                .frequency    = make_simple_directive_string(10, "s", "REPEAT FOR EVER")
           },
            // Output
           {
                .expected_action_code = 0,
                .expected_next_time   = "10",
                .expected_frequency   = make_simple_directive_string(10, "s", "REPEAT FOR EVER")
           }
        }},
        {"REPEAT UNTIL SUCCESS", {
            // Input
           {
                .current_time = "0",
                .frequency    = make_simple_directive_string(10, "s", "REPEAT UNTIL SUCCESS")
           },
            // Output
           {
                .expected_action_code = 1,
                .expected_next_time   = "10",
                .expected_frequency   = make_simple_directive_string(10, "s", "REPEAT UNTIL SUCCESS")
           }
        }},
        {"REPEAT <N> TIMES", {
            // Input
           {
                .current_time = "0",
                .frequency    = make_counter_directive_string(10, "s", "REPEAT", 5)
           },
            // Output
           {
                .expected_action_code = 3,
                .expected_next_time   = "10",
                .expected_frequency   = make_modified_counter_directive_string(10, "s", "REPEAT", 4)
           }
        }},
        {"REPEAT UNTIL <TIME>", {
            // Input
           {
                .current_time = "0",
                .frequency    = make_time_directive_string(10, "s", "REPEAT UNTIL", one_hour_in_future)
           },
            // Output
           {
                .expected_action_code = 0,
                .expected_next_time   = "10",
                .expected_frequency   = make_time_directive_string(10, "s", "REPEAT UNTIL", one_hour_in_future)
           }
        }},
        {"REPEAT UNTIL SUCCESS OR UNTIL <TIME>", {
            // Input
           {
                .current_time = "0",
                .frequency    = make_time_directive_string(10, "s", "REPEAT UNTIL SUCCESS OR UNTIL", one_hour_in_future)
           },
            // Output
           {
                .expected_action_code = 1,
                .expected_next_time   = "10",
                .expected_frequency   = make_time_directive_string(10, "s", "REPEAT UNTIL SUCCESS OR UNTIL", one_hour_in_future)
           }
        }},
        {"REPEAT UNTIL SUCCESS OR <N> TIMES", {
            // Input
           {
                .current_time = "0",
                .frequency    = make_counter_directive_string(10, "s", "REPEAT UNTIL SUCCESS OR", 5)
           },
            // Output
           {
                .expected_action_code = 4,
                .expected_next_time   = "10",
                .expected_frequency   = make_modified_counter_directive_string(10, "s", "REPEAT UNTIL SUCCESS OR", 4)
           }
        }},
        {"DOUBLE FOR EVER", {
            // Input
           {
                .current_time = "0",
                .frequency    = make_simple_directive_string(10, "s", "DOUBLE FOR EVER")
           },
            // Output
           {
                .expected_action_code = 3,
                .expected_next_time   = "10",
                .expected_frequency   = make_simple_directive_string(20, "s", "DOUBLE FOR EVER")
           }
        }},
        {"DOUBLE UNTIL SUCCESS", {
            // Input
           {
                .current_time = "0",
                .frequency    = make_simple_directive_string(10, "s", "DOUBLE UNTIL SUCCESS")
           },
            // Output
           {
                .expected_action_code = 4,
                .expected_next_time   = "10",
                .expected_frequency   = make_simple_directive_string(20, "s", "DOUBLE UNTIL SUCCESS")
           }
        }},
        {"DOUBLE <N> TIMES", {
            // Input
           {
                .current_time = "0",
                .frequency    = make_counter_directive_string(10, "s", "DOUBLE", 5)
           },
            // Output
           {
                .expected_action_code = 3,
                .expected_next_time   = "10",
                .expected_frequency   = make_modified_counter_directive_string(20, "s", "DOUBLE", 4)
           }
        }},
        {"DOUBLE UNTIL <TIME>", {
            // Input
           {
                .current_time = "0",
                .frequency    = make_time_directive_string(10, "s", "DOUBLE UNTIL", one_hour_in_future)
           },
            // Output
           {
                .expected_action_code = 3,
                .expected_next_time   = "10",
                .expected_frequency   = make_time_directive_string(20, "s", "DOUBLE UNTIL", one_hour_in_future)
           }
        }},
        {"DOUBLE UNTIL SUCCESS OR UNTIL <TIME>", {
            // Input
           {
                .current_time = "0",
                .frequency    = make_time_directive_string(10, "s", "DOUBLE UNTIL SUCCESS OR UNTIL", one_hour_in_future)
           },
            // Output
           {
                .expected_action_code = 4,
                .expected_next_time   = "10",
                .expected_frequency   = make_time_directive_string(20, "s", "DOUBLE UNTIL SUCCESS OR UNTIL", one_hour_in_future)
           }
        }},
        {"DOUBLE UNTIL SUCCESS OR <N> TIMES", {
            // Input
           {
                .current_time = "0",
                .frequency    = make_counter_directive_string(10, "s", "DOUBLE UNTIL SUCCESS OR", 20)
           },
            // Output
           {
                .expected_action_code = 4,
                .expected_next_time   = "10",
                .expected_frequency   = make_modified_counter_directive_string(20, "s", "DOUBLE UNTIL SUCCESS OR", 19)
           }
        }},
        {"DOUBLE UNTIL SUCCESS UPTO <TIME>", {
            // Input
           {
                .current_time = "0",
                .frequency    = make_time_directive_string(10, "s", "DOUBLE UNTIL SUCCESS UPTO", one_hour_in_future)
           },
            // Output
           {
                .expected_action_code = 4,
                .expected_next_time   = "10",
                .expected_frequency   = make_time_directive_string(20, "s", "DOUBLE UNTIL SUCCESS UPTO", one_hour_in_future)
           }
        }}
    };
    // clang-format on

    for (auto&& [test_case_name, in_out] : map) {
        DYNAMIC_SECTION("test case: " << test_case_name)
        {
            test_execution_frequency_handler(in_out.input, in_out.output);
        }
    }
}

