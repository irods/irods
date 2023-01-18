#ifndef IRODS_SCOPED_CLIENT_IDENTITY_HPP
#define IRODS_SCOPED_CLIENT_IDENTITY_HPP

#include "irods/irods_exception.hpp"
#include "irods/rcConnect.h"
#include "irods/rodsConnect.h"
#include "irods/rodsErrorTable.h"

#include <cstring>
#include <string>

namespace irods::experimental
{
    /// This class provides a convenient RAII-style mechanism for becoming a different
    /// iRODS user for the duration of a scoped block.
    ///
    /// Instances of this class are not copyable or moveable.
    ///
    /// \since 4.2.8
    class scoped_client_identity // NOLINT(cppcoreguidelines-special-member-functions)
    {
      public:
        /// Constructs a scoped_client_identity object and updates the RsComm to represent
        /// a user in the local zone.
        ///
        /// The zone will match the string returned by getLocalZoneName().
        ///
        /// \param[in] _comm         The server communication object.
        /// \param[in] _new_username The name of the user to become.
        ///
        /// \throws irods::exception If an error occurs.
        scoped_client_identity(RsComm& _comm, const std::string& _new_username)
            : scoped_client_identity{_comm, _new_username, ""}
        {
        }

        /// Constructs a scoped_client_identity object and updates the RsComm to represent
        /// a user in a zone.
        ///
        /// \param[in] _comm         The server communication object.
        /// \param[in] _new_username The name of the user to become.
        /// \param[in] _new_zone     The name of the zone the user is a member of.
        ///                          If _new_zone is empty, it defaults to the value returned
        ///                          by getLocalZoneName().
        ///
        /// \throws irods::exception If an error occurs.
        ///
        /// \since 4.2.12
        scoped_client_identity(RsComm& _comm, const std::string& _new_username, const std::string& _new_zone)
            : comm_{_comm}
            , old_username_{comm_.clientUser.userName}
            , old_zone_{comm_.clientUser.rodsZone}
        {
            if (_new_username.empty()) {
                THROW(SYS_INVALID_INPUT_PARAM, "scoped_client_identity: username is empty.");
            }

            if (_new_username.size() >= sizeof(UserInfo::userName)) {
                THROW(SYS_INVALID_INPUT_PARAM, "scoped_client_identity: username is too long.");
            }

            if (_new_zone.size() >= sizeof(UserInfo::rodsZone)) {
                THROW(SYS_INVALID_INPUT_PARAM, "scoped_client_identity: zone is too long.");
            }

            const char* new_zone = _new_zone.c_str();

            if (_new_zone.empty()) {
                new_zone = getLocalZoneName();
            }

            std::strcpy(comm_.clientUser.userName, _new_username.c_str());
            std::strcpy(comm_.clientUser.rodsZone, new_zone);
        }

        scoped_client_identity(const scoped_client_identity&) = delete;
        auto operator=(const scoped_client_identity&) -> scoped_client_identity& = delete;

        /// Restores the identity of the RsComm::clientUser to its original value.
        ~scoped_client_identity()
        {
            std::strcpy(comm_.clientUser.userName, old_username_.c_str());
            std::strcpy(comm_.clientUser.rodsZone, old_zone_.c_str());
        }

      private:
        RsComm& comm_;
        std::string old_username_;
        std::string old_zone_;
    }; // class scoped_client_identity
} // namespace irods::experimental

#endif // IRODS_SCOPED_CLIENT_IDENTITY_HPP
