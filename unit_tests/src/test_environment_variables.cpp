#include <catch2/catch.hpp>

#include "irods/getRodsEnv.h"
#include "irods/irods_environment_properties.hpp"
#include "irods/rodsErrorTable.h"

#include <cstddef>
#include <cstring>
#include <string>

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("environment_variables")
{
    const std::string env_var_key{"ISSUE_6866_ENV_KEY"};
    const std::string env_var_val{"ISSUE_6866_ENV_VAL"};

    irods::set_environment_property(env_var_key, env_var_val);

    // Buffer size constants
    constexpr std::size_t ZERO_BUFFER_SIZE = 0;
    constexpr std::size_t TOO_SMALL_BUFFER_SIZE = 10;
    constexpr std::size_t LARGE_BUFFER_SIZE = 100;

    SECTION("test bound checks for environment variables with large buffer issue 6866")
    {
        // Assert the working case:
        // Given a large enough buffer, the environment variable will be copied into the buffer
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
        char large_buffer[LARGE_BUFFER_SIZE]{};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        REQUIRE(0 == capture_string_property(env_var_key.c_str(), large_buffer, LARGE_BUFFER_SIZE));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        REQUIRE(std::strlen(large_buffer) == env_var_val.size());
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        REQUIRE(0 == std::strncmp(large_buffer, env_var_val.c_str(), env_var_val.size()));
    }

    SECTION("test bounds checks for environment variables with small buffer issue 6866")
    {
        // Assert it fails with small buffer:
        // If the buffer is too small, capture_string_property should
        // return a SYS_GET_ENV_ERR and the buffer should remain empty
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
        char small_buffer[TOO_SMALL_BUFFER_SIZE]{};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        REQUIRE(TOO_SMALL_BUFFER_SIZE < env_var_val.size());
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        REQUIRE(SYS_GETENV_ERR == capture_string_property(env_var_key.c_str(), small_buffer, TOO_SMALL_BUFFER_SIZE));
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
        REQUIRE(small_buffer[0] == '\0');
    }

    SECTION("test bounds checks for environment variables with large buffer and small size issue 6866")
    {
        // Assert it fails with small buffer size:
        // If the given buffer size is too small, capture_string_property should
        // return a SYS_GET_ENV_ERR and the buffer should remain empty
        // even if the buffer can actually hold more
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
        char mismatched_buffer[LARGE_BUFFER_SIZE]{};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        REQUIRE(SYS_GETENV_ERR ==
                capture_string_property(env_var_key.c_str(), mismatched_buffer, TOO_SMALL_BUFFER_SIZE));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        REQUIRE(std::strlen(mismatched_buffer) == 0);
    }

    SECTION("test bounds checks for environment variables with zero buffer issue 6866")
    {
        // Assert it fails with zero buffer size:
        // If the given buffer size is zero, capture_string_property should
        // return a SYS_GET_ENV_ERR and the buffer should not be touched
        //
        // NOTE: zero_buffer does not have a size of zero. This test is only to see
        // if passing a size of zero will cause the function to return an error
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
        char zero_buffer[LARGE_BUFFER_SIZE]{};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        REQUIRE(SYS_GETENV_ERR == capture_string_property(env_var_key.c_str(), zero_buffer, ZERO_BUFFER_SIZE));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        REQUIRE(std::strlen(zero_buffer) == 0);
    }

    SECTION("test bounds checks for environment variables with null buffer issue 6866")
    {
        // Assert it fails with null input:
        // If the buffer is null, capture_string_property should
        // return a SYS_GET_ENV_ERR and the buffer should not be touched
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
        char* null_buffer = nullptr;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        REQUIRE(SYS_GETENV_ERR == capture_string_property(env_var_key.c_str(), null_buffer, TOO_SMALL_BUFFER_SIZE));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        REQUIRE(null_buffer == nullptr);
    }
}
