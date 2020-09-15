#ifndef IRODS_USER_ADMINISTRATION_HPP
#define IRODS_USER_ADMINISTRATION_HPP

/// \file

#undef NAMESPACE_IMPL
#undef rxComm

#ifdef IRODS_USER_ADMINISTRATION_ENABLE_SERVER_SIDE_API
    #define NAMESPACE_IMPL      server
    #define rxComm              RsComm
    struct RsComm;
#else 
    #define NAMESPACE_IMPL      client
    #define rxComm              RcComm
    struct RcComm;
#endif // IRODS_USER_ADMINISTRATION_ENABLE_SERVER_SIDE_API

#include "user.hpp"
#include "group.hpp"

#include <vector>
#include <string>
#include <optional>
#include <ostream>
#include <system_error>
#include <stdexcept>

/// \since 4.2.8
namespace irods::experimental::administration
{
    /// \since 4.2.8
    ///
    /// A namespace that allows other variations of the library while also making
    /// this interface (and implementation) the default version.
    inline namespace v1
    {
        /// \since 4.2.8
        ///
        /// \brief Defines the user types supported by the user administration library.
        enum class user_type
        {
            rodsuser,   ///< Identifies users that do not have administrative privileges.
            groupadmin, ///< Identifies users that have limited administrative privileges. 
            rodsadmin   ///< Identifies users that have full administrative privileges.
        }; // enum class user_type

        /// \since 4.2.8
        ///
        /// \brief Defines the zone types.
        enum class zone_type
        {
            local,  ///< Identifies the zone in which an operation originates.
            remote  ///< Identifies a foreign zone.
        }; // enum class zone_type

        /// \since 4.2.8
        ///
        /// \brief The generic exception type used by the user administration library.
        class user_management_error
            : std::runtime_error
        {
        public:
            using std::runtime_error::runtime_error;
        }; // class user_management_error
    } // namespace v1

    /// \since 4.2.8
    ///
    /// \brief A namespace defining the set of client-side or server-side API functions.
    ///
    /// This namespace's name changes based on the presence of a special macro. If the following macro
    /// is defined, then the NAMESPACE_IMPL will be \p server, else it will be \p client.
    /// 
    ///     - IRODS_USER_ADMINISTRATION_ENABLE_SERVER_SIDE_API
    ///
    namespace NAMESPACE_IMPL
    {
        /// \since 4.2.8
        inline namespace v1
        {
            /// \since 4.2.8
            ///
            /// \brief Adds a new user to the system.
            ///
            /// \throws user_management_error If the user type cannot be converted to a string.
            ///
            /// \param[in] conn      The communication object.
            /// \param[in] user      The user to add.
            /// \param[in] user_type The type of the user.
            /// \param[in] zone_type The zone that is responsible for the user.
            ///
            /// \return An error code.
            auto add_user(rxComm& conn,
                          const user& user,
                          user_type user_type = user_type::rodsuser,
                          zone_type zone_type = zone_type::local) -> std::error_code;

            /// \since 4.2.8
            ///
            /// \brief Removes a user from the system.
            ///
            /// \param[in] conn The communication object.
            /// \param[in] user The user to remove.
            ///
            /// \return An error code.
            auto remove_user(rxComm& conn, const user& user) -> std::error_code;

            /// \since 4.2.8
            ///
            /// \brief Changes the password of a user.
            ///
            /// \param[in] conn         The communication object.
            /// \param[in] user         The user to update.
            /// \param[in] new_password The new password of the user.
            ///
            /// \return An error code.
            auto set_user_password(rxComm& conn, const user& user, std::string_view new_password) -> std::error_code;

            /// \since 4.2.8
            ///
            /// \brief Changes the type of a user.
            ///
            /// \throws user_management_error If the new user type cannot be converted to a string.
            ///
            /// \param[in] conn          The communication object.
            /// \param[in] user          The user to update.
            /// \param[in] new_user_type The new type of the user.
            ///
            /// \return An error code.
            auto set_user_type(rxComm& conn, const user& user, user_type new_user_type) -> std::error_code;

            /// \since 4.2.8
            ///
            /// \brief Adds an authentication name to a user.
            ///
            /// \param[in] conn The communication object.
            /// \param[in] user The user to update.
            /// \param[in] auth The authentication name to add.
            ///
            /// \return An error code.
            auto add_user_auth(rxComm& conn, const user& user, std::string_view auth) -> std::error_code;

            /// \since 4.2.8
            ///
            /// \brief Removes an authentication name from a user.
            ///
            /// \param[in] conn The communication object.
            /// \param[in] user The user to update.
            /// \param[in] auth The authentication name to remove.
            ///
            /// \return An error code.
            auto remove_user_auth(rxComm& conn, const user& user, std::string_view auth) -> std::error_code;

            /// \since 4.2.8
            ///
            /// \brief Adds a new group to the system.
            ///
            /// \param[in] conn  The communication object.
            /// \param[in] group The group to add.
            ///
            /// \return An error code.
            auto add_group(rxComm& conn, const group& group) -> std::error_code;

            /// \since 4.2.8
            ///
            /// \brief Removes a group from the system.
            ///
            /// \param[in] conn  The communication object.
            /// \param[in] group The group to remove.
            ///
            /// \return An error code.
            auto remove_group(rxComm& conn, const group& group) -> std::error_code;

            /// \since 4.2.8
            ///
            /// \brief Adds a user to a group.
            ///
            /// \param[in] conn  The communication object.
            /// \param[in] group The group that will hold the user.
            /// \param[in] user  The user to add to the group.
            ///
            /// \return An error code.
            auto add_user_to_group(rxComm& conn, const group& group, const user& user) -> std::error_code;

            /// \since 4.2.8
            ///
            /// \brief Removes a user from a group.
            ///
            /// \param[in] conn  The communication object.
            /// \param[in] group The group that contains the user.
            /// \param[in] user  The user to remove from the group.
            ///
            /// \return An error code.
            auto remove_user_from_group(rxComm& conn, const group& group, const user& user) -> std::error_code;

            /// \since 4.2.8
            ///
            /// \brief Returns all users in the local zone.
            ///
            /// \param[in] conn The communication object.
            ///
            /// \return A list of users.
            auto users(rxComm& conn) -> std::vector<user>;

            /// \since 4.2.8
            ///
            /// \brief Returns all members of a group.
            ///
            /// \param[in] conn  The communication object.
            /// \param[in] group The group to check.
            ///
            /// \return A list of users.
            auto users(rxComm& conn, const group& group) -> std::vector<user>;

            /// \since 4.2.8
            ///
            /// \brief Returns all groups in the local zone.
            ///
            /// \param[in] conn The communication object.
            ///
            /// \return A list of groups.
            auto groups(rxComm& conn) -> std::vector<group>;

            /// \since 4.2.8
            ///
            /// \brief Returns all groups a user is a member of.
            ///
            /// \param[in] conn The communication object.
            /// \param[in] user The user to check for.
            ///
            /// \return A list of groups.
            auto groups(rxComm& conn, const user& user) -> std::vector<group>;

            /// \since 4.2.8
            ///
            /// \brief Checks if a user exists.
            ///
            /// \param[in] conn The communication object.
            ///
            /// \return A boolean.
            /// \retval true  If the user exists.
            /// \retval false Otherwise.
            auto exists(rxComm& conn, const user& user) -> bool;

            /// \since 4.2.8
            ///
            /// \brief Checks if a group exists.
            ///
            /// \param[in] conn The communication object.
            ///
            /// \return A boolean.
            /// \retval true  If the user exists.
            /// \retval false Otherwise.
            auto exists(rxComm& conn, const group& group) -> bool;

            /// \since 4.2.8
            ///
            /// \brief Returns the ID of a user.
            ///
            /// \param[in] conn The communication object.
            /// \param[in] user The user to find.
            ///
            /// \return A std::optional object containing the ID as a string if the user
            ///         exists in the local zone.
            auto id(rxComm& conn, const user& user) -> std::optional<std::string>;

            /// \since 4.2.8
            ///
            /// \brief Returns the ID of a group.
            ///
            /// \param[in] conn The communication object.
            /// \param[in] user The group to find.
            ///
            /// \return A std::optional object containing the ID as a string if the group
            ///         exists in the local zone.
            auto id(rxComm& conn, const group& group) -> std::optional<std::string>;

            /// \since 4.2.8
            ///
            /// \brief Returns the type of a user.
            ///
            /// \throws user_management_error If the value within the catalog cannot be converted
            ///                               to an appropriate user_type.
            ///
            /// \param[in] conn The communication object.
            /// \param[in] user The user to find.
            ///
            /// \return A std::optional object containing the user's type if the user
            ///         exists in the local zone.
            auto type(rxComm& conn, const user& user) -> std::optional<user_type>;

            /// \since 4.2.8
            ///
            /// \brief Returns the authentication names (methods) used by the user.
            ///
            /// \param[in] conn The communication object.
            /// \param[in] user The user to fetch authentication names for.
            ///
            /// \return A list of strings.
            auto auth_names(rxComm& conn, const user& user) -> std::vector<std::string>;

            /// \since 4.2.8
            ///
            /// \brief Checks if a user is a member of a group.
            ///
            /// \param[in] conn  The communication object.
            /// \param[in] group The group to check.
            /// \param[in] user  The user to look for.
            ///
            /// \return A boolean.
            /// \retval true  If the user is a member of the group.
            /// \retval false Otherwise.
            auto user_is_member_of_group(rxComm& conn, const group& group, const user& user) -> bool;

            /// \since 4.2.8
            ///
            /// \brief Returns the unique name of a user in the local zone.
            ///
            /// \throws user_management_error If the local zone cannot be retrieved (client-side only).
            ///
            /// \param[in] conn The communication object.
            /// \param[in] user The user to produce the unique name for.
            ///
            /// \return A string representing the unique name of the user within the local zone.
            auto local_unique_name(rxComm& conn, const user& user) -> std::string;
        } // namespace v1
    } // namespace NAMESPACE_IMPL
} // namespace irods::experimental::administration

#endif // IRODS_USER_ADMINISTRATION_HPP

