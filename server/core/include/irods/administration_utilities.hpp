#ifndef IRODS_ADMINISTRATION_UTILITIES_HPP
#define IRODS_ADMINISTRATION_UTILITIES_HPP

#include <string_view>

struct RsComm;

namespace irods
{
    /// \brief Create a user with the given name, type, and auth string
    ///
    /// \param[in/out] _comm Server communication handle
    /// \param[in] _user_name The name of the to-be-created user (including zone name)
    /// \param[in] _user_type The type of the to-be-created user
    /// \param[in] _auth_string The auth string for the to-be-created user
    /// \param[in] _zone_name The zone name for the to-be-created user\parblock
    /// If the _zone_name is not provided and the _user_name does not contain the zone name
    /// using the octothorpe delimiter, the local zone will be used.
    /// \endparblock
    ///
    /// \retval 0 Success
    /// \retval !0 Failure
    ///
    /// \since 4.2.11
    auto create_user(RsComm& _comm,
                     const std::string_view _user_name,
                     const std::string_view _user_type,
                     const std::string_view _auth_string,
                     const std::string_view _zone_name) -> int;

    /// \brief Remove a user with the given name
    ///
    /// \param[in/out] _comm Server communication handle
    /// \param[in] _user_name The name of the user to create (including zone name)
    /// \param[in] _zone_name The zone name for the to-be-created user\parblock
    /// If the _zone_name is not provided and the _user_name does not contain the zone name
    /// using the octothorpe delimiter, the local zone will be used.
    /// \endparblock
    ///
    /// \retval 0 Success
    /// \retval !0 Failure
    ///
    /// \since 4.2.11
    auto remove_user(RsComm& _comm,
                     const std::string_view _user_name,
                     const std::string_view _zone_name) -> int;
} // namespace irods

#endif // IRODS_ADMINISTRATION_UTILITIES_HPP

