#ifndef IRODS_SCOPED_PRIVILEGED_CLIENT_HPP
#define IRODS_SCOPED_PRIVILEGED_CLIENT_HPP

#include "irods/rcConnect.h"

namespace irods::experimental
{
    /// This class provides a convenient RAII-style mechanism for elevating client privileges
    /// for the duration of a scoped block.
    ///
    /// Instances of this class are not copyable or moveable.
    ///
    /// \since 4.2.8
    class scoped_privileged_client
    {
    public:
        /// Constructs a scoped_privileged_client object and elevates privileges of
        /// rsComm_t::clientUser to LOCAL_PRIV_USER_AUTH.
        ///
        /// \param[in] comm The server communication object.
        explicit scoped_privileged_client(rsComm_t& comm) noexcept
            : conn_{comm}
            , original_auth_flag_{conn_.clientUser.authInfo.authFlag}
        {
            conn_.clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
        }

        scoped_privileged_client(const scoped_privileged_client&) = delete;
        auto operator=(const scoped_privileged_client&) -> scoped_privileged_client& = delete;

        /// Restores the privileges of rsComm_t::clientUser to the original values.
        ~scoped_privileged_client()
        {
            conn_.clientUser.authInfo.authFlag = original_auth_flag_;
        }

    private:
        rsComm_t& conn_;
        const int original_auth_flag_;
    }; // class scoped_privileged_client
} // namespace irods::experimental

#endif // IRODS_SCOPED_PRIVILEGED_CLIENT_HPP

