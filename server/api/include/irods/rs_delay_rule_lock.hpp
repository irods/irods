#ifndef IRODS_RS_DELAY_RULE_LOCK_HPP
#define IRODS_RS_DELAY_RULE_LOCK_HPP

/// \file

#include "irods/delay_rule_lock.h"

struct RsComm;

/// Attaches information about the delay server to a delay rule in the catalog.
///
/// This API is designed to allow the delay server to block other delay servers from executing
/// a delay rule.
///
/// Requires \p rodsadmin level privileges.
///
/// On success, the following information will be stored in the catalog entry representing the
/// target delay rule:
/// - The hostname or IP of the delay server
/// - The PID of the delay server
/// - The time the entry was locked
///
/// This operation will fail if the delay rule has already been locked.
///
/// \param[in] _comm  A pointer to a RsComm.
/// \param[in] _input A pointer to a DelayRuleLockInput.
///
/// \return An integer.
/// \retval 0  On success.
/// \retval <0 On failure.
///
/// \b Example
/// \code{.cpp}
/// RsComm* comm = // Our iRODS connection representing a rodsadmin user.
///
/// // Configure the input object for the API call.
/// struct DelayRuleLockInput input;
/// memset(&input, 0, sizeof(struct DelayRuleLockInput));
///
/// strncpy(input.rule_id, "12345", sizeof(DelayRuleLockInput::rule_id) - 1);
/// strncpy(input.lock_host, "node1.example.com", sizeof(DelayRuleLockInput::lock_host) - 1);
///
/// // We're operating in the delay server, so we can use getpid().
/// input.lock_host_pid = getpid();
///
/// const int ec = rs_delay_rule_lock(comm, &input);
///
/// if (ec < 0) {
///     // Handle error.
/// }
/// \endcode
///
/// \since 5.0.0
int rs_delay_rule_lock(RsComm* _comm, DelayRuleLockInput* _input);

#endif // IRODS_RS_DELAY_RULE_LOCK_HPP
