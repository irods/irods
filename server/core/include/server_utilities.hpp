#ifndef IRODS_SERVER_UTILITIES_HPP
#define IRODS_SERVER_UTILITIES_HPP

/// \file

#include <string_view>

struct RsComm;
struct DataObjInp;

namespace irods
{
    /// A utility function primarily meant to be used with ::rsDataObjPut and ::rsDataObjCopy.
    ///
    /// \param[in] _comm  A reference to the communication object.
    /// \param[in] _input A reference to the ::DataObjInp containing the ::KeyValPair.
    ///
    /// \return A boolean value indicating whether the FORCE_FLAG_KW is required or not.
    /// \retval true  If the keyword is required.
    /// \retval false If the keyword is not required.
    ///
    /// \since 4.2.9
    auto is_force_flag_required(RsComm& _comm, const DataObjInp& _input) -> bool;

    /// Checks if the provided rule text contains session variables.
    ///
    /// This function will reject any text that contains a session variable. This includes text
    /// that contains session variables in comments.
    ///
    /// \param[in] _rule_text The rule text to check.
    ///
    /// \return A boolean value.
    /// \retval true  If the rule text contains session variables.
    /// \retval false Otherwise.
    ///
    /// \since 4.2.9
    auto contains_session_variables(const std::string_view _rule_text) -> bool;
} // namespace irods

#endif // IRODS_SERVER_UTILITIES_HPP

