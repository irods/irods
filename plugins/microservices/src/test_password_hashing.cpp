/// \file

#include "irods/irods_exception.hpp"
#include "irods/irods_ms_plugin.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/msParam.h"
#include "irods/password_hash.hpp"

#include "msi_assertion_macros.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cstdint>
#include <tuple>

namespace
{
    // These are useful as valid inputs for scrypt-specific tests.
    // See "STRONGER KEY DERIVATION VIA SEQUENTIAL MEMORY-HARD FUNCTIONS", Appendix B, test vector 3.
    constexpr const char* normal_password = "pleaseletmein";
    constexpr const char* normal_salt = "SodiumChloride";
    constexpr const char* scrypt_algorithm_name = "scrypt";
    constexpr auto maximum_password_length = 1024 * 1024;

    auto scrypt_test_params() -> const nlohmann::json*
    {
        static const auto params = nlohmann::json{{"key_length", 64}, {"N", 16384}, {"r", 8}, {"p", 1}};
        return &params;
    } // scrypt_test_params

    auto test_empty_parameters_to_hash_password_result_in_errors([[maybe_unused]] RuleExecInfo& _rei) -> int
    {
        // Note: This test is not specific to the scrypt algorithm.

        IRODS_MSI_TEST_BEGIN("test empty parameters given to irods::hash_password result in errors")

        constexpr const char* empty_msg = "hash_password: Cannot derive key from password - password or salt is empty.";

        // Empty password.
        IRODS_MSI_THROWS_MSG(irods::hash_password("", normal_salt, scrypt_algorithm_name, *scrypt_test_params()),
                             irods::exception,
                             empty_msg)

        // Empty salt.
        IRODS_MSI_THROWS_MSG(irods::hash_password(normal_password, "", scrypt_algorithm_name, *scrypt_test_params()),
                             irods::exception,
                             empty_msg)

        // The configuration for the algorithm depends heavily on the algorithm, so this does not test an empty
        // configuration.

        IRODS_MSI_TEST_END
    } // test_empty_parameters_to_hash_password_result_in_errors

    auto test_unsupported_algorithm_name_results_in_error([[maybe_unused]] RuleExecInfo& _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("test unsupported algorithm name results in error")

        constexpr const char* unsupported_algorithm_name_msg_template =
            "get_kdf: Hashing algorithm [{}] not supported.";

        constexpr const char* unsupported_algorithm_name = "unsupported_algorithm_name";
        IRODS_MSI_THROWS_MSG(
            irods::hash_password(normal_password, normal_salt, unsupported_algorithm_name, *scrypt_test_params()),
            irods::exception,
            fmt::format(unsupported_algorithm_name_msg_template, unsupported_algorithm_name).c_str())

        IRODS_MSI_THROWS_MSG(irods::hash_password(normal_password, normal_salt, "", *scrypt_test_params()),
                             irods::exception,
                             fmt::format(unsupported_algorithm_name_msg_template, "").c_str())

        IRODS_MSI_TEST_END
    } // test_unsupported_algorithm_name_results_in_error

    auto test_scrypt_invalid_algorithm_configurations_result_in_errors([[maybe_unused]] RuleExecInfo& _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("test invalid configurations for scrypt results in errors")

        // Empty JSON object should fail.
        IRODS_MSI_THROWS_CODE(
            irods::hash_password(normal_password, normal_salt, scrypt_algorithm_name, nlohmann::json{{}}),
            SYS_INVALID_INPUT_PARAM)

        // The parameters all have a bunch of shared invalid values.
        constexpr std::array<const char*, 4> int32_params = {"key_length", "N", "r", "p"};
        for (const auto& param : int32_params) {
            irods::experimental::log::microservice::info("testing parameter [{}]", param);

            // Copy the base scrypt configuration for manipulation.
            auto config = *scrypt_test_params();

            // Parameter missing
            config.erase(param);
            IRODS_MSI_THROWS_CODE(irods::hash_password(normal_password, normal_salt, scrypt_algorithm_name, config),
                                  SYS_INVALID_INPUT_PARAM)

            // Wrong type
            config[param] = param;
            IRODS_MSI_THROWS_CODE(irods::hash_password(normal_password, normal_salt, scrypt_algorithm_name, config),
                                  SYS_INVALID_INPUT_PARAM)

            // Underflow - use std::int64_t to hold a value that will underflow with std::int32_t. This cannot be
            // detected because there is no upper limit on these values. The value is large enough that OpenSSL returns
            // an error. key_length does have an upper limit because OpenSSL will just sit there and try to compute a
            // key that's 2GiB large if you ask it to. Therefore, it has a different error code for this case.
            const auto expected_error_code =
                ("key_length" == std::string_view{param}) ? SYS_INVALID_INPUT_PARAM : CAT_PASSWORD_ENCODING_ERROR;
            config[param] = static_cast<std::int64_t>(INT32_MIN) - 1;
            IRODS_MSI_THROWS_CODE(
                irods::hash_password(normal_password, normal_salt, scrypt_algorithm_name, config), expected_error_code)

            // Negative value
            config[param] = -1;
            IRODS_MSI_THROWS_CODE(irods::hash_password(normal_password, normal_salt, scrypt_algorithm_name, config),
                                  SYS_INVALID_INPUT_PARAM)

            // Zero
            config[param] = 0;
            IRODS_MSI_THROWS_CODE(irods::hash_password(normal_password, normal_salt, scrypt_algorithm_name, config),
                                  SYS_INVALID_INPUT_PARAM)

            // Overflow - use std::int64_t to hold a value that will overflow with std::int32_t.
            config[param] = static_cast<std::int64_t>(INT32_MAX) + 1;
            IRODS_MSI_THROWS_CODE(irods::hash_password(normal_password, normal_salt, scrypt_algorithm_name, config),
                                  SYS_INVALID_INPUT_PARAM)
        }

        IRODS_MSI_TEST_END
    } // test_scrypt_invalid_algorithm_configurations_result_in_errors

    auto test_password_that_is_one_character_too_long_results_in_an_error([[maybe_unused]] RuleExecInfo& _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("test password that is one character too long results in an error")

        const auto toolong_password = std::string(maximum_password_length + 1, 'a');

        IRODS_MSI_THROWS_MSG(
            irods::hash_password(toolong_password, normal_salt, scrypt_algorithm_name, *scrypt_test_params()),
            irods::exception,
            "hash_password: Cannot derive key from password - password length exceeds maximum size.")

        IRODS_MSI_TEST_END
    } // test_password_that_is_one_character_too_long_results_in_an_error

    auto test_scrypt_basic_password_hashing_from_scrypt_paper([[maybe_unused]] RuleExecInfo& _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("test basic scrypt password hashing from scrypt paper")

        // These test cases come from the scrypt paper "Stronger Key Derivation via Sequential Memory-hard Functions"
        // by Colin Percival (2009). To compare results, please consult the paper and find Appendix B: "Test Vectors".
        // Each expected result is a base64-encoded string of the key derived with the specified parameters and are
        // labeled as "vX" where X is the test vector from the paper represented here.

        // Test vector 1 (v1) is not tested here because it uses an empty password and an empty salt. The function
        // being tested here considers these to be invalid inputs.

        constexpr const char* expected_result_v2 =
            "/bq+HJ00cgB4VucZDQHp/nxq18vII3gw53N2Y0s3MWIurzDZLiKjiG/xCSedmDDaxyevuUqD7m2DYMvfoswGQA==";
        const auto params_v2 = nlohmann::json{{"key_length", 64}, {"N", 1024}, {"r", 8}, {"p", 16}};
        const auto hash_v2 = irods::hash_password("password", "NaCl", scrypt_algorithm_name, params_v2);
        IRODS_MSI_ASSERT(expected_result_v2 == hash_v2)

        constexpr const char* expected_result_v3 =
            "cCO9yzr9c0hGHAbNgf046/2o+7qQT44+qbVD9lRdofLVQylVYT8Pz2LUlwUkKpr55h6F3A1lHkDfzwF7RVdYhw==";
        const auto& params_v3 = *scrypt_test_params();
        const auto hash_v3 = irods::hash_password(normal_password, normal_salt, scrypt_algorithm_name, params_v3);
        IRODS_MSI_ASSERT(expected_result_v3 == hash_v3)

        constexpr const char* expected_result_v4 =
            "IQHLm2pRGq6t274Jz3D4gexWjVdKL/1Nq+XumCCtqkeOVv2PS6XQn/ocbZJ8QPTDNzBASeipUvvL9Fxvp3pBpA==";
        const auto params_v4 = nlohmann::json{{"key_length", 64}, {"N", 1048576}, {"r", 8}, {"p", 1}};
        const auto hash_v4 = irods::hash_password(normal_password, normal_salt, scrypt_algorithm_name, params_v4);
        IRODS_MSI_ASSERT(expected_result_v4 == hash_v4)

        IRODS_MSI_TEST_END
    } // test_scrypt_basic_password_hashing_from_scrypt_paper

    auto test_scrypt_with_maximum_password_length([[maybe_unused]] RuleExecInfo& _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("test scrypt with maximum password length")

        constexpr const char* expected_result =
            "rZawbhCL0d1z+T31lJPM41Kpu2texBiMpIkT37EzoqUkGtNaJPRRMhfYV3WlfcW+TXhloSQ8dQ95zyx8sdiOdQ==";
        const auto long_password = std::string(maximum_password_length, 'a');
        const auto hash =
            irods::hash_password(long_password, normal_salt, scrypt_algorithm_name, *scrypt_test_params());
        IRODS_MSI_ASSERT(expected_result == hash)

        IRODS_MSI_TEST_END
    } // test_scrypt_with_maximum_password_length

    auto msi_impl(RuleExecInfo* _rei) -> int
    {
        IRODS_MSI_TEST_CASE(test_empty_parameters_to_hash_password_result_in_errors, *_rei)
        IRODS_MSI_TEST_CASE(test_unsupported_algorithm_name_results_in_error, *_rei)
        IRODS_MSI_TEST_CASE(test_scrypt_invalid_algorithm_configurations_result_in_errors, *_rei)
        IRODS_MSI_TEST_CASE(test_password_that_is_one_character_too_long_results_in_an_error, *_rei)
        IRODS_MSI_TEST_CASE(test_scrypt_basic_password_hashing_from_scrypt_paper, *_rei)
        IRODS_MSI_TEST_CASE(test_scrypt_with_maximum_password_length, *_rei)

        return 0;
    } // msi_impl

    template <typename... Args, typename Function>
    auto make_msi(const std::string& _name, Function _func) -> irods::ms_table_entry*
    {
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        auto* msi = new irods::ms_table_entry{sizeof...(Args)};
        msi->add_operation(_name, std::function<int(Args..., ruleExecInfo_t*)>(_func));
        return msi;
    } // make_msi
} // anonymous namespace

extern "C" auto plugin_factory() -> irods::ms_table_entry*
{
    return make_msi<>("msi_test_password_hashing", msi_impl);
} // plugin_factory
