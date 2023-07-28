#ifndef IRODS_USER_ADMINISTRATION_HPP
#define IRODS_USER_ADMINISTRATION_HPP

/// \file

#undef NAMESPACE_IMPL
#undef RxComm
#undef rxGeneralAdmin

#ifdef IRODS_USER_ADMINISTRATION_ENABLE_SERVER_SIDE_API
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#  define NAMESPACE_IMPL server
#  define RxComm         RsComm
#  define rxGeneralAdmin rsGeneralAdmin
// NOLINTEND(cppcoreguidelines-macro-usage)

#  include "irods/rsGeneralAdmin.hpp"

struct RsComm;
#else
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#  define NAMESPACE_IMPL client
#  define RxComm         RcComm
#  define rxGeneralAdmin rcGeneralAdmin
// NOLINTEND(cppcoreguidelines-macro-usage)

#  include "irods/generalAdmin.h"

struct RcComm;
#endif // IRODS_USER_ADMINISTRATION_ENABLE_SERVER_SIDE_API

#include "irods/user.hpp"
#include "irods/group.hpp"
#include "irods/zone_type.hpp"
#include "irods/irods_exception.hpp"
#include "irods/rodsErrorTable.h"

#include <fmt/format.h>

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

/// \since 4.2.8
namespace irods::experimental::administration
{
    /// Defines the user types supported by the user administration library.
    ///
    /// \since 4.2.8
    enum class user_type
    {
        // clang-format off
        rodsuser,   ///< Identifies users that do not have administrative privileges.
        groupadmin, ///< Identifies users that have limited administrative privileges.
        rodsadmin   ///< Identifies users that have full administrative privileges.
        // clang-format on
    }; // enum class user_type

    /// Defines the set of operations for manipulating user authentication names.
    ///
    /// \since 4.3.1
    enum class user_authentication_operation
    {
        // clang-format off
        add,   ///< Indicates that the authentication name should be available to the user.
        remove ///< Indicates that the authentication name should not be available to the user.
        // clang-format on
    }; // enum class user_auth_property_operation

    /// Holds the new password for a user. Primarily used to modify a user.
    ///
    /// \since 4.3.1
    struct user_password_property
    {
        /// Constructs an instance of #user_password_property.
        ///
        /// \param[in] _value              The new password for a user.
        /// \param[in] _requester_password The plaintext password of the user requesting the
        ///                                change. If passed, #obfGetPw will not be used.
        ///
        /// \since 4.3.1
        explicit user_password_property(std::string _value,
                                        std::optional<std::string> _requester_password = std::nullopt)
            : value{std::move(_value)}
            , requester_password{std::move(_requester_password)}
        {
        }

        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        std::string value; ///< The new password.

        /// The password of the user requesting the change.
        /// If not set, obfGetPw() is used. Use of obfGetPw() implies a .irods/.irodsA file
        /// exists locally.
        std::optional<std::string> requester_password; // NOLINT(misc-non-private-member-variables-in-classes)
    }; // struct user_password_property

    /// Holds the new type of a user. Primarily used to modify a user.
    ///
    /// \since 4.3.1
    struct user_type_property
    {
        user_type value;
    }; // struct user_type_property

    /// Holds the new password for a user. Primarily used to modify a user.
    ///
    /// \since 4.3.1
    struct user_authentication_property
    {
        // clang-format off
        user_authentication_operation op; ///< The operation to execute.
        std::string value;                ///< The authentication name.
        // clang-format on
    }; // struct user_authentication_property

    // clang-format off
    /// The \p UserProperty concept is satisfied if and only if \p T matches one of the following types:
    /// - \p user_password_property
    /// - \p user_type_property
    /// - \p user_authentication_property
    ///
    /// \since 4.3.1
    template <typename T>
    concept UserProperty = std::same_as<T, user_password_property> ||
        std::same_as<T, user_type_property> ||
        std::same_as<T, user_authentication_property>;
    // clang-format on

    /// Converts a string to a user_type enumeration.
    ///
    /// \param[in] user_type_string The string to convert.
    ///
    /// \throws irods::exception If an error occurs.
    ///
    /// \return A user_type enumeration.
    ///
    /// \since 4.3.1
    auto to_user_type(const std::string_view user_type_string) -> user_type;

    /// Converts a user_type enumeration to a string.
    ///
    /// The string returned is a read-only string and must not be modified by the caller. Attempting
    /// to modify the string will result in undefined behavior.
    ///
    /// \param[in] user_type The enumeration to convert.
    ///
    /// \throws irods::exception If an error occurs.
    ///
    /// \return A C string.
    ///
    /// \since 4.3.1
    auto to_c_str(user_type user_type) -> const char*;

    /// Obfuscates a user password.
    ///
    /// See #user_password_property for additional details.
    ///
    /// \since 4.3.1
    auto obfuscate_password(const user_password_property& _property) -> std::string;

    /// A namespace defining the set of client-side or server-side API functions.
    ///
    /// This namespace's name changes based on the presence of a special macro. If the following macro
    /// is defined, then the NAMESPACE_IMPL will be \p server, else it will be \p client.
    ///
    ///     - IRODS_USER_ADMINISTRATION_ENABLE_SERVER_SIDE_API
    ///
    /// \since 4.2.8
    namespace NAMESPACE_IMPL
    {
        /// Returns the unique name of a user in the local zone.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _user The user to produce the unique name for.
        ///
        /// \throws irods::exception If an error occurs.
        ///
        /// \return A string representing the unique name of the user within the local zone.
        ///
        /// \since 4.2.8
        auto local_unique_name(RxComm& _comm, const user& _user) -> std::string;

        /// Adds a new user to the system.
        ///
        /// \param[in] _comm      The communication object.
        /// \param[in] _user      The user to add.
        /// \param[in] _user_type The type of the user.
        /// \param[in] _zone_type The zone that is responsible for the user.
        ///
        /// \throws irods::exception If an error occurs.
        ///
        /// \return An error code.
        ///
        /// \since 4.2.8
        auto add_user(
            RxComm& _comm,
            const user& _user,
            user_type _user_type = user_type::rodsuser,
            zone_type _zone_type = zone_type::local) -> void;

        /// Removes a user from the system.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _user The user to remove.
        ///
        /// \return An error code.
        ///
        /// \since 4.2.8
        auto remove_user(RxComm& _comm, const user& _user) -> void;

        /// Modifies a property of a user.
        ///
        /// \tparam Property \parblock The type of the property to modify.
        ///
        /// Property must satisfy the UserProperty concept. Failing to do so will result in a compile-time error.
        /// \endparblock
        ///
        /// \param[in] _comm     The communication object.
        /// \param[in] _user     The user to modify.
        /// \param[in] _property An object containing the required information for applying the change.
        ///
        /// \throws irods::exception If an error occurs.
        ///
        /// \since 4.3.1
        template <UserProperty Property>
        auto modify_user(RxComm& _comm, const user& _user, const Property& _property) -> void
        {
            const auto name = local_unique_name(_comm, _user);
            [[maybe_unused]] std::string obfuscated_password;

            GeneralAdminInput input{};
            input.arg0 = "modify";
            input.arg1 = "user";
            input.arg2 = name.c_str();

            // clang-format off
            const auto execute = [&]<typename... Args>(fmt::format_string<Args...> _msg, Args&&... _args)
            {
                if (const auto ec = rxGeneralAdmin(&_comm, &input); ec != 0) {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                    THROW(ec, fmt::format(_msg, std::forward<Args>(_args)...));
                }
            };
            // clang-format on

            if constexpr (std::is_same_v<Property, user_password_property>) {
                obfuscated_password = obfuscate_password(_property);
                input.arg3 = "password";
                input.arg4 = obfuscated_password.c_str();
                execute("Failed to set password for user [{}].", name);
            }
            else if constexpr (std::is_same_v<Property, user_type_property>) {
                input.arg3 = "type";
                input.arg4 = to_c_str(_property.value);
                execute("Failed to set user type to [{}] for user [{}].", input.arg4, name);
            }
            else if constexpr (std::is_same_v<Property, user_authentication_property>) {
                input.arg4 = _property.value.c_str();

                if (_property.op == user_authentication_operation::add) {
                    input.arg3 = "addAuth";
                    execute("Failed to add authentication name [{}] to user [{}].", input.arg4, name);
                }
                else {
                    input.arg3 = "rmAuth";
                    execute("Failed to remove authentication name [{}] from user [{}].", input.arg4, name);
                }
            }
        } // modify_user

        /// Adds a new group to the system.
        ///
        /// \param[in] _comm  The communication object.
        /// \param[in] _group The group to add.
        ///
        /// \throws irods::exception If an error occurs.
        ///
        /// \since 4.2.8
        auto add_group(RxComm& _comm, const group& _group) -> void;

        /// Removes a group from the system.
        ///
        /// \param[in] _comm  The communication object.
        /// \param[in] _group The group to remove.
        ///
        /// \throws irods::exception If an error occurs.
        ///
        /// \since 4.2.8
        auto remove_group(RxComm& _comm, const group& _group) -> void;

        /// Adds a user to a group.
        ///
        /// \param[in] _comm  The communication object.
        /// \param[in] _group The group that will hold the user.
        /// \param[in] _user  The user to add to the group.
        ///
        /// \throws irods::exception If an error occurs.
        ///
        /// \since 4.2.8
        auto add_user_to_group(RxComm& _comm, const group& _group, const user& _user) -> void;

        /// Removes a user from a group.
        ///
        /// \param[in] _comm  The communication object.
        /// \param[in] _group The group that contains the user.
        /// \param[in] _user  The user to remove from the group.
        ///
        /// \throws irods::exception If an error occurs.
        ///
        /// \since 4.2.8
        auto remove_user_from_group(RxComm& _comm, const group& _group, const user& _user) -> void;

        /// Returns all users in the local zone.
        ///
        /// \param[in] _comm The communication object.
        ///
        /// \return A list of users.
        ///
        /// \since 4.2.8
        auto users(RxComm& _comm) -> std::vector<user>;

        /// Returns all users in a group.
        ///
        /// \param[in] _comm  The communication object.
        /// \param[in] _group The group to check.
        ///
        /// \return A list of users.
        ///
        /// \since 4.2.8
        auto users(RxComm& _comm, const group& _group) -> std::vector<user>;

        /// Returns all groups in the local zone.
        ///
        /// \param[in] _comm The communication object.
        ///
        /// \return A list of groups.
        ///
        /// \since 4.2.8
        auto groups(RxComm& _comm) -> std::vector<group>;

        /// Returns all groups a user is a member of.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _user The user to check for.
        ///
        /// \return A list of groups.
        ///
        /// \since 4.2.8
        auto groups(RxComm& _comm, const user& _user) -> std::vector<group>;

        /// Checks if a user exists.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _user The user to verify exists.
        ///
        /// \return A boolean.
        /// \retval true  If the user exists.
        /// \retval false Otherwise.
        ///
        /// \since 4.2.8
        auto exists(RxComm& _comm, const user& _user) -> bool;

        /// Checks if a group exists.
        ///
        /// \param[in] _comm  The communication object.
        /// \param[in] _group The group to verify exists.
        ///
        /// \return A boolean.
        /// \retval true  If the user exists.
        /// \retval false Otherwise.
        ///
        /// \since 4.2.8
        auto exists(RxComm& _comm, const group& _group) -> bool;

        /// Returns the ID of a user.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _user The user to find.
        ///
        /// \return A std::optional object containing the ID as a string if the user
        ///         exists in the local zone.
        ///
        /// \since 4.2.8
        auto id(RxComm& _comm, const user& _user) -> std::optional<std::string>;

        /// Returns the ID of a group.
        ///
        /// \param[in] _comm  The communication object.
        /// \param[in] _group The group to find.
        ///
        /// \return A std::optional object containing the ID as a string if the group
        ///         exists in the local zone.
        ///
        /// \since 4.2.8
        auto id(RxComm& _comm, const group& _group) -> std::optional<std::string>;

        /// Returns the type of a user.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _user The user to find.
        ///
        /// \throws irods::exception If an error occurs.
        ///
        /// \return A std::optional object containing the user's type if the user
        ///         exists in the local zone.
        ///
        /// \since 4.2.8
        auto type(RxComm& _comm, const user& _user) -> std::optional<user_type>;

        /// Returns the authentication names (methods) used by the user.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _user The user to fetch authentication names for.
        ///
        /// \return A list of strings.
        ///
        /// \since 4.2.8
        auto auth_names(RxComm& _comm, const user& _user) -> std::vector<std::string>;

        /// Checks if a user is a member of a group.
        ///
        /// \param[in] _comm  The communication object.
        /// \param[in] _group The group to check.
        /// \param[in] _user  The user to look for.
        ///
        /// \return A boolean.
        /// \retval true  If the user is a member of the group.
        /// \retval false Otherwise.
        ///
        /// \since 4.2.8
        auto user_is_member_of_group(RxComm& _comm, const group& _group, const user& _user) -> bool;
    } // namespace NAMESPACE_IMPL
} // namespace irods::experimental::administration

#endif // IRODS_USER_ADMINISTRATION_HPP
