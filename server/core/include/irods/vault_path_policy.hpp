#ifndef IRODS_VAULT_PATH_POLICY_HPP
#define IRODS_VAULT_PATH_POLICY_HPP

/// \file

#include <optional>
#include <string_view>

namespace irods::vault_path_policy
{
    enum class file_naming_policy
    {
        consistent,
        random,
        reversed_dataid
    };

    /// Resource context keyword controlling physical path naming for new writes and logical move sync.
    ///
    /// Supported values are "consistent", "random", and "reversed_dataid". The value is case-sensitive,
    /// leaf-resource scoped, and defaults to "consistent" when absent or invalid.
    inline constexpr const char* file_naming_policy_key = "file_naming_policy";
    inline constexpr const char* file_naming_policy_consistent = "consistent";
    inline constexpr const char* file_naming_policy_random = "random";
    inline constexpr const char* file_naming_policy_reversed_dataid = "reversed_dataid";

    constexpr auto to_file_naming_policy(std::string_view _policy) -> std::optional<file_naming_policy>
    {
        if (_policy == file_naming_policy_consistent) {
            return file_naming_policy::consistent;
        }

        if (_policy == file_naming_policy_random) {
            return file_naming_policy::random;
        }

        if (_policy == file_naming_policy_reversed_dataid) {
            return file_naming_policy::reversed_dataid;
        }

        return std::nullopt;
    } // to_file_naming_policy

    /// Resource context keys used when file_naming_policy=random. These affect new writes only.
    inline constexpr const char* random_scheme_style = "random_scheme_style";
    inline constexpr const char* random_scheme_suffix_length = "random_scheme_suffix_length";

    // Default values used by the random scheme microservices and vault path policy.
    inline constexpr int random_scheme_config_default_style = 0;
    inline constexpr int random_scheme_config_default_suffix_length = 5;
} // namespace irods::vault_path_policy

#endif // IRODS_VAULT_PATH_POLICY_HPP
