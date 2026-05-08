#ifndef IRODS_VAULT_PATH_POLICY_HPP
#define IRODS_VAULT_PATH_POLICY_HPP

/// \file

namespace irods::vault_path_policy
{
    // Keywords used by the random scheme microservices and vault path policy.
    inline constexpr const char* random_scheme_style = "random_scheme_style";
    inline constexpr const char* random_scheme_suffix_length = "random_scheme_suffix_length";

    // Default values used by the random scheme microservices and vault path policy.
    inline constexpr int random_scheme_config_default_style = 0;
    inline constexpr int random_scheme_config_default_suffix_length = 5;
} // namespace irods::vault_path_policy

#endif // IRODS_VAULT_PATH_POLICY_HPP
