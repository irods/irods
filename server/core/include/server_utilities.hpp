#ifndef IRODS_SERVER_UTILITIES_HPP
#define IRODS_SERVER_UTILITIES_HPP

/// \file

struct RsComm;
struct DataObjInp;

namespace irods
{
    /// A utility function primarily meant to be used with ::rsDataObjPut and ::rsDataObjCopy.
    ///
    /// \since 4.2.9
    ///
    /// \param[in] _comm  A reference to the communication object.
    /// \param[in] _input A reference to the ::DataObjInp containing the ::KeyValPair.
    ///
    /// \return A boolean value indicating whether the FORCE_FLAG_KW is required or not.
    /// \retval true  If the keyword is required.
    /// \retval false If the keyword is not required.
    auto is_force_flag_required(RsComm& _comm, const DataObjInp& _input) -> bool;
} // namespace irods

#endif // IRODS_SERVER_UTILITIES_HPP

