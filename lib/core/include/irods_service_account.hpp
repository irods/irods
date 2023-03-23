#ifndef IRODS_SERVICE_ACCOUNT_HPP
#define IRODS_SERVICE_ACCOUNT_HPP

namespace irods
{
    /// Attempts to determine whether the caller is running as the irods service unix account.
    ///
    /// \exception irods::exception If an error occurred.
    ///
    /// \since 4.3.1 and 4.2.12
    auto is_service_account() -> bool;
} //namespace irods

#endif // IRODS_SERVICE_ACCOUNT_HPP
