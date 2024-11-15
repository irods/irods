#ifndef IRODS_DELAY_RULE_LOCK_H
#define IRODS_DELAY_RULE_LOCK_H

/// \file

#include "irods/objInfo.h"
#include "irods/rodsDef.h" // For TIME_LEN

struct RcComm;

/// The input data type used to invoke #rc_delay_rule_lock.
///
/// \since 5.0.0
typedef struct DelayRuleLockInput
{
    /// The catalog ID of the delay rule to lock.
    ///
    /// Must be a non-empty string.
    ///
    /// \since 5.0.0
    char rule_id[32]; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)

    /// The FQDN, hostname, or IP of the delay server which is attempting to lock the
    /// delay rule.
    ///
    /// Must be a non-empty string.
    ///
    /// \since 5.0.0
    char lock_host[300]; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)

    /// The PID of the delay server which is attempting to lock the delay rule.
    ///
    /// \since 5.0.0
    int lock_host_pid;

    /// The set of key-value pair strings used to influence the behavior of the API
    /// operation.
    ///
    /// \since 5.0.0
    struct KeyValPair cond_input;
} delayRuleLockInp_t;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DelayRuleLockInput_PI "str rule_id[32]; str lock_host[300]; int lock_host_pid; struct KeyValPair_PI;"

#ifdef __cplusplus
extern "C" {
#endif

/// Attaches lock information about the delay server to a delay rule in the catalog.
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
/// \param[in] _comm  A pointer to a RcComm.
/// \param[in] _input A pointer to a DelayRuleLockInput.
///
/// \return An integer.
/// \retval 0  On success.
/// \retval <0 On failure.
///
/// \b Example
/// \code{.cpp}
/// RcComm* comm = // Our iRODS connection representing a rodsadmin user.
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
/// const int ec = rc_delay_rule_lock(comm, &input);
///
/// if (ec < 0) {
///     // Handle error.
/// }
/// \endcode
///
/// \since 5.0.0
int rc_delay_rule_lock(struct RcComm* _comm, struct DelayRuleLockInput* _input);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_DELAY_RULE_LOCK_H
