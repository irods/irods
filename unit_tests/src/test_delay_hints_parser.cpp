#include <catch2/catch.hpp>

#include "irods/rcMisc.h"

#include <fmt/format.h>
#include <fmt/chrono.h>

#include <cstring>
#include <map>
#include <string>
#include <string_view>

struct input
{
    std::string_view current_time;
    std::string_view frequency;
};

struct output
{
    int expected_action_code;
    std::string_view expected_next_time;
    std::string_view expected_frequency;
};

auto test_execution_frequency_handler(const input& _input, const output& _output)
{
    char current_time[TIME_LEN]{};
    std::strcpy(current_time, _input.current_time.data());

    char frequency[128]{};
    std::strcpy(frequency, _input.frequency.data());

    char next_time[TIME_LEN]{};

    CHECK(getNextRepeatTime(current_time, frequency, next_time) == _output.expected_action_code);
    CHECK(next_time == _output.expected_next_time);
    CHECK(frequency == _output.expected_frequency);
}

template <typename Timepoint>
auto make_repeat_until_time_string(int _nnnn,
                                   const std::string_view _U,
                                   const Timepoint _tp) -> std::string
{
    return fmt::format("{}{} REPEAT UNTIL {}", _nnnn, _U, std::chrono::system_clock::to_time_t(_tp));
}

template <typename Timepoint>
auto make_repeat_until_success_or_until_time_string(int _nnnn,
                                                    const std::string_view _U,
                                                    const Timepoint _tp) -> std::string
{
    return fmt::format("{}{} REPEAT UNTIL SUCCESS OR UNTIL {}", _nnnn, _U, std::chrono::system_clock::to_time_t(_tp));
}

#if 0
//
// Disabled due to issues with "DOUBLE" delay hints.
// See https://github.com/irods/irods/issues/5964 for more details.
//

template <typename Timepoint>
auto make_double_until_time_string(int _nnnn,
                                   const std::string_view _U,
                                   const Timepoint _tp) -> std::string
{
    return fmt::format("{}{} DOUBLE UNTIL {}", _nnnn, _U, std::chrono::system_clock::to_time_t(_tp));
}

template <typename Timepoint>
auto make_double_until_success_or_until_time_string(int _nnnn,
                                                    const std::string_view _U,
                                                    const Timepoint _tp) -> std::string
{
    return fmt::format("{}{} DOUBLE UNTIL SUCCESS OR UNTIL {}", _nnnn, _U, std::chrono::system_clock::to_time_t(_tp));
}
#endif

TEST_CASE("#5503: getNextRepeatTime parses frequency correctly")
{
    struct test_case
    {
        input input;
        output output;
    };

    using namespace std::chrono_literals;

    const auto now = std::chrono::system_clock::now();

    const auto repeat_until_time_string = make_repeat_until_time_string(10, "s", now + 1h);
    const auto repeat_until_success_or_until_time_string = make_repeat_until_success_or_until_time_string(10, "s", now + 1h);

#if 0
    //
    // Disabled due to issues with "DOUBLE" delay hints.
    // See https://github.com/irods/irods/issues/5964 for more details.
    //

    auto double_until_time_string = make_double_until_time_string(10, "s", now + 1h);
    auto double_until_success_or_until_time_string = make_double_until_success_or_until_time_string(10, "s", now + 1h);
#endif

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
                .frequency    = "10s REPEAT FOR EVER"
           },
            // Output
           {
                .expected_action_code = 0,
                .expected_next_time   = "10",
                .expected_frequency   = "10s REPEAT FOR EVER"
           }
        }},
        {"REPEAT UNTIL SUCCESS", {
            // Input
           {
                .current_time = "0",
                .frequency    = "10s REPEAT UNTIL SUCCESS"
           },
            // Output
           {
                .expected_action_code = 1,
                .expected_next_time   = "10",
                .expected_frequency   = "10s REPEAT UNTIL SUCCESS"
           }
        }},
        {"REPEAT <N> TIMES", {
            // Input
           {
                .current_time = "0",
                .frequency    = "10s REPEAT 5 TIMES"
           },
            // Output
           {
                .expected_action_code = 3,
                .expected_next_time   = "10",
                .expected_frequency   = "10s REPEAT 4 TIMES. ORIGINAL TIMES=5"
           }
        }},
        {"REPEAT UNTIL <TIME>", {
            // Input
           {
                .current_time = "0",
                .frequency    = repeat_until_time_string
           },
            // Output
           {
                .expected_action_code = 0,
                .expected_next_time   = "10",
                .expected_frequency   = repeat_until_time_string
           }
        }},
        {"REPEAT UNTIL SUCCESS OR UNTIL <TIME>", {
            // Input
           {
                .current_time = "0",
                .frequency    = repeat_until_success_or_until_time_string
           },
            // Output
           {
                .expected_action_code = 1,
                .expected_next_time   = "10",
                .expected_frequency   = repeat_until_success_or_until_time_string
           }
        }},
        {"REPEAT UNTIL SUCCESS OR <N> TIMES", {
            // Input
           {
                .current_time = "0",
                .frequency    = "10s REPEAT UNTIL SUCCESS OR 5 TIMES"
           },
            // Output
           {
                .expected_action_code = 4,
                .expected_next_time   = "10",
                .expected_frequency   = "10s REPEAT UNTIL SUCCESS OR 4 TIMES. ORIGINAL TIMES=5"
           }
        }},
#if 0
        //
        // Disabled due to issues with "DOUBLE" delay hints.
        // See https://github.com/irods/irods/issues/5964 for more details.
        //

        {"DOUBLE FOR EVER", {
            // Input
           {
                .current_time = "0",
                .frequency    = "10s DOUBLE FOR EVER"
           },
            // Output
           {
                .expected_action_code = 3,
                .expected_next_time   = "10",
                .expected_frequency   = "20s DOUBLE FOR EVER"
           }
        }},
        {"DOUBLE UNTIL SUCCESS", {
            // Input
           {
                .current_time = "0",
                .frequency    = "10s DOUBLE UNTIL SUCCESS"
           },
            // Output
           {
                .expected_action_code = 4,
                .expected_next_time   = "10",
                .expected_frequency   = "20s DOUBLE UNTIL SUCCESS"
           }
        }},
        {"DOUBLE <N> TIMES", {
            // Input
           {
                .current_time = "0",
                .frequency    = "10s DOUBLE 5 TIMES"
           },
            // Output
           {
                .expected_action_code = 3,
                .expected_next_time   = "10",
                .expected_frequency   = "20s DOUBLE 4 TIMES. ORIGINAL TIMES=5"
           }
        }},
        {"DOUBLE UNTIL <TIME>", {
            // Input
           {
                .current_time = "0",
                .frequency    = double_until_time_string
           },
            // Output
           {
                .expected_action_code = 3,
                .expected_next_time   = "10",
                .expected_frequency   = double_until_time_string.replace(0, 2, "20")
           }
        }},
        {"DOUBLE UNTIL SUCCESS OR UNTIL <TIME>", {
            // Input
           {
                .current_time = "0",
                .frequency    = double_until_success_or_until_time_string
           },
            // Output
           {
                .expected_action_code = 4,
                .expected_next_time   = "10",
                .expected_frequency   = double_until_success_or_until_time_string.replace(0, 2, "20")
           }
        }},
        {"DOUBLE UNTIL SUCCESS OR <N> TIMES", {
            // Input
           {
                .current_time = "0",
                .frequency    = double_until_success_or_n_times
           },
            // Output
           {
                .expected_action_code = 0,
                .expected_next_time   = "20",
                .expected_frequency   = double_until_success_or_n_times
           }
        }},
        {"DOUBLE UNTIL SUCCESS OR UPTO <TIME>", {
            // Input
           {
                .current_time = "0",
                .frequency    = double_until_success_or_upto_time
           },
            // Output
           {
                .expected_action_code = 0,
                .expected_next_time   = "20",
                .expected_frequency   = double_until_success_or_upto_time
           }
        }}
#endif
    };

    for (auto&& [test_case_name, in_out] : map) {
        DYNAMIC_SECTION("test case: " << test_case_name)
        {
            test_execution_frequency_handler(in_out.input, in_out.output);
        }
    }
}

