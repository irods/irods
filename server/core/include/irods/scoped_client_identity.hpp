#ifndef IRODS_SCOPED_CLIENT_IDENTITY_HPP
#define IRODS_SCOPED_CLIENT_IDENTITY_HPP

#include "irods/rcConnect.h"

#include <cstring>
#include <string>
#include <string_view>
#include <stdexcept>

namespace irods::experimental
{
    /// This class provides a convenient RAII-style mechanism for becoming a different
    /// iRODS user for the duration of a scoped block.
    ///
    /// Instances of this class are not copyable or moveable.
    ///
    /// \since 4.2.8
    class scoped_client_identity
    {
    public:
        /// Constructs a scoped_client_identity object and sets the username of
        /// rsComm_t::clientUser to new_username.
        ///
        /// \param[in] comm         The server communication object.
        /// \param[in] new_username The name of the iRODS user to become.
        ///
        /// \exception std::length_error Thrown if new_username exceeds the buffer size
        ///                              of rsComm_t::clientUser::userName.
        scoped_client_identity(rsComm_t& comm, std::string_view new_username)
            : conn_{comm}
            , username_{conn_.clientUser.userName}
        {
            if (new_username.size() >= sizeof(conn_.clientUser.userName)) {
                throw std::length_error{"scoped_client_identity: new_username exceeds buffer size"};
            }

            std::strcpy(conn_.clientUser.userName, new_username.data());
        }

        scoped_client_identity(const scoped_client_identity&) = delete;
        auto operator=(const scoped_client_identity&) -> scoped_client_identity& = delete;

        /// Restores the username of the rsComm_t::clientUser to it's original value.
        ~scoped_client_identity()
        {
            std::strcpy(conn_.clientUser.userName, username_.data());
        }

    private:
        rsComm_t& conn_;
        const std::string username_;
    }; // class scoped_client_identity
} // namespace irods::experimental

#endif // IRODS_SCOPED_CLIENT_IDENTITY_HPP

