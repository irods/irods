#include <catch2/catch.hpp>

#include "irods/json_events.hpp"

namespace je = irods::experimental::json_events;

using json = nlohmann::json;

TEST_CASE("Test json_events", "[json_events]")
{
    SECTION("KW_ALL_KEYS and specific hooks work")
    {
        bool catchall_fired = false;
        bool specific_hook_fired = false;
        bool incorrect_hook_not_fired = true;
        int hooks_so_far = 0;
        // There are 4 semantics that matter here
        // The hook associated with the specific key should fire
        je::add_hook(std::vector<std::string>{"/test_thing"}, [&](const std::string& _path, const json& _op) {
            specific_hook_fired = true;
            // Specifically it should fire second
            REQUIRE(++hooks_so_far == 2);
        });
        // The hook that is not associated with another specific key should not fire
        je::add_hook(std::vector<std::string>{"/incorrect_thing"},
                     [&](const std::string& _path, const json& _op) { incorrect_hook_not_fired = false; });
        // The catchall hook should fire
        je::add_hook(std::vector<std::string>{je::KW_ALL_KEYS}, [&](const std::string& _path, const json& _op) {
            catchall_fired = true;
            // And it should be the first to fire.
            REQUIRE(++hooks_so_far == 1);
        });
        je::run_hooks({{{"path", "/test_thing"}, {"op", "replace"}, {"value", 3}}});
        REQUIRE(catchall_fired);
        REQUIRE(specific_hook_fired);
        REQUIRE(incorrect_hook_not_fired);
    }
}