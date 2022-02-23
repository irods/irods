#ifndef IRODS_USER_VALIDATION_UTILITIES_HPP
#define IRODS_USER_VALIDATION_UTILITIES_HPP

#include <optional>
#include <string>
#include <string_view>
#include <utility>

struct RsComm;

namespace irods::user
{
    /// \brief Look up the user type of the given user
    ///
    /// \param[in/out] _comm Server connection handle
    /// \param[in] _user_name Name of the user whose user type is being fetched.\p
    /// \parblock
    /// _user_name should contain the zone name for a remote user. If no zone is found in the
    /// provided user name string, the local zone is used. The usual octothorpe-delimited
    /// format is expected for parsing the user and zone names.
    /// \endparblock
    ///
    /// \return The user type of the given user as stored in the catalog
    ///
    /// \throws irods::exception If _user_name does not exist
    ///
    /// \since 4.2.11
    auto get_type(RsComm& _comm, const std::string_view _user_name) -> std::string;

    /// \brief Validates the given user type
    ///
    /// \param[in] _user_type The user type to validate
    ///
    /// \retval true If _user_type is one of the following:\p
    /// \parblock
    ///     - rodsuser
    ///     - rodsadmin
    ///     - groupadmin
    ///     - rodsgroup
    /// \endparblock
    ///
    /// \retval false If _user_type is not one of the items above.
    ///
    /// \since 4.2.11
    auto type_is_valid(const std::string_view _user_type) -> bool;

    /// \brief Validates the provided user name based on a set of criteria.
    ///
    /// \parblock
    /// The regex matches user names with no hashes and optionally one at symbol (@), and then
    /// optionally a hash followed by a zone name containing no hashes.
    ///
    /// User name must be between 1 and 63 characters.
    /// User name may contain any combination of word characters, @ symbols, dashes, and dots.
    /// User name may not be . or .., as we create home directories for users
    /// Zone name must be less than 64 characters.
    /// \endparblock
    ///
    /// \param[in] _user_name The user name to validate
    ///
    /// \returns User and Zone name as a pair of strings if _user_name is valid.
    /// \retval std::nullopt If _user_name is not valid.
    ///
    /// \since 4.2.11
    auto validate_name(const std::string_view _user_name) -> std::optional<std::pair<std::string, std::string>>;
} // namespace irods::user

#endif // IRODS_USER_VALIDATION_UTILITIES_HPP
