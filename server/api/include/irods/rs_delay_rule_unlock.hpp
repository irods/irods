#ifndef IRODS_RS_DELAY_RULE_UNLOCK_HPP
#define IRODS_RS_DELAY_RULE_UNLOCK_HPP

/// \file

#include "irods/delay_rule_unlock.h"

struct RsComm;

/// Atomically removes the delay server lock information from one or more delay rules.
///
/// Requires \p rodsadmin level privileges.
///
/// On success, the following information will be removed from the delay rule catalog entries:
/// - The host identifying the delay server
/// - The PID of the delay server
/// - The time the entry was locked
///
/// On failure, all catalog updates are rolled back and an error code is returned to the client.
///
/// \param[in] _comm  A pointer to a RsComm.
/// \param[in] _input A pointer to a DelayRuleUnlockInput.
///
/// \return An integer.
/// \retval  0 On success.
/// \retval <0 On failure.
///
/// \b Example
/// \code{.cpp}
/// RsComm* comm = // Our iRODS connection representing a rodsadmin user.
///
/// // Configure the input object for the API call.
/// struct DelayRuleUnlockInput input;
/// memset(&input, 0, sizeof(struct DelayRuleUnlockInput));
///
/// // Don't forget to deallocate the memory pointed to by "input.rule_ids"
/// // following the completion of the API operation.
/// input.rule_ids = strdup("[\"12345\", \"67890\"]");
///
/// const int ec = rs_delay_rule_unlock(comm, &input);
///
/// if (ec < 0) {
///     // Handle error.
/// }
/// \endcode
///
/// \since 5.0.0
int rs_delay_rule_unlock(RsComm* _comm, DelayRuleUnlockInput* _input);

#endif // IRODS_RS_DELAY_RULE_UNLOCK_HPP
