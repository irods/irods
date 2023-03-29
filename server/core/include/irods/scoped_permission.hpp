#ifndef IRODS_SCOPED_PERMISSION_HPP
#define IRODS_SCOPED_PERMISSION_HPP

/// \file

#include "irods/rcConnect.h"
#include "irods/scoped_privileged_client.hpp"
#include "irods/rodsLog.h"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "irods/filesystem.hpp"

#include <fmt/format.h>

namespace irods::experimental
{
    /// This class provides a convenient RAII-style mechanism for setting permissions
    /// on a collection or data object for the duration of a scoped block.
    ///
    /// Instances of this class are not copyable or moveable.
    ///
    /// \since 4.2.11
    class scoped_permission
    {
    public:
        /// Constructs a scoped_permission object and sets the requested permissions on the given
        /// collection or data object to the client in RsComm::clientUser.
        ///
        /// \param[in] _comm The server communication object.
        /// \param[in] _path A logical path representing a collection or data object.
        /// \param[in] _perm The permissions to set on the collection or data object for the client.
        ///
        /// \exception irods::experimental::filesystem::filesystem_error
        scoped_permission(RsComm& _comm, const filesystem::path& _path, filesystem::perms _perm)
            : comm_{_comm}
            , path_{_path}
            , old_perm_{filesystem::perms::null}
            , qualified_username_{}
        {
            namespace fs = filesystem;

            // Elevate privileges so that we can see the permissions.
            scoped_privileged_client spc{comm_};

            // If the zone in the RsComm.clientUser is empty, we assume that it refers to the local zone.
            const auto* client_zone =
                std::strlen(comm_.clientUser.rodsZone) > 0 ? comm_.clientUser.rodsZone : getLocalZoneName();

            // Capture the client's current permissions if any.
            const auto s = fs::server::status(comm_, path_);
            for (auto&& entity : s.permissions()) {
                if (entity.name == comm_.clientUser.userName && entity.zone == client_zone) {
                    old_perm_ = entity.prms;
                    break;
                }
            }

            qualified_username_ = fmt::format("{}#{}", comm_.clientUser.userName, client_zone);

            // Set the client's requested permissions on the collection or data object.
            fs::server::permissions(fs::admin, comm_, path_, qualified_username_, _perm);
        }

        scoped_permission(const scoped_permission&) = delete;
        auto operator=(const scoped_permission&) -> scoped_permission& = delete;

        /// Restores the permissions on the collection or data object to their original value.
        ~scoped_permission()
        {
            namespace fs = filesystem;

            // Elevate privileges in case the client did not set adequate permissions
            // to restore them.
            scoped_privileged_client spc{comm_};

            try {
                fs::server::permissions(fs::admin, comm_, path_, qualified_username_, old_perm_);
            }
            catch (const fs::filesystem_error& e) {
                rodsLog(LOG_ERROR, "%s: %s [error_code=%d]", __func__, e.what(), e.code().value());
            }
        }

    private:
        RsComm& comm_;
        const filesystem::path path_;
        filesystem::perms old_perm_;
        std::string qualified_username_;
    }; // class scoped_permission
} // namespace irods::experimental

// Remove the macro so that it does not leak into the file that included it.
// This keeps other headers from being affected by the presence of this macro.
#undef IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API

#endif // IRODS_SCOPED_PERMISSION_HPP

