#ifndef IRODS_GET_LOGICAL_QUOTA_H
#define IRODS_GET_LOGICAL_QUOTA_H

/// \file

#include "irods/objInfo.h"
#include "irods/rodsType.h"

struct RcComm;

/// The input type used to fetch a list of logical quotas.
///
/// \since 5.1.0
typedef struct GetLogicalQuotaInput {
    /// The collection name for which to fetch applicable quotas.
    /// Quotas will be fetched for the named collection, as well as any ancestors.
    /// e.g. "/tempZone/home" will fetch quotas for /, /tempZone, and /tempZone/home.
    ///
    /// nullptr or empty string will fetch all quotas.
    ///
    /// \since 5.1.0
    char* coll_name;

    /// The condInput structure.
    /// Currently unused.
    ///
    /// \since 5.1.0
    keyValPair_t cond_input;
} getLogicalQuotaInp_t;

#define getLogicalQuotaInp_PI "str *collName; struct KeyValPair_PI;"

/// The type that represents a single logical quota entry.
///
/// \since 5.1.0
typedef struct LogicalQuota {
    /// The collection name to which the current quota is applied.
    ///
    /// \since 5.1.0
    char* coll_name;

    /// The byte limit configured for the current quota.
    ///
    /// \since 5.1.0
    rodsLong_t max_bytes;

    /// The object limit configured for the current quota.
    ///
    /// \since 5.1.0
    rodsLong_t max_objects;

    /// The total number of bytes in the collection exceeds the the current quota's max_bytes by this amount.
    /// This value will be negative or zero if the limit has not been exceeded.
    ///
    /// \since 5.1.0
    rodsLong_t over_bytes;

    /// The total number of objects in the collection exceeds the current quota's max_objects by this amount.
    /// This value will be negative or zero if the limit has not been exceeded.
    ///
    /// \since 5.1.0
    rodsLong_t over_objects;
} logicalQuota_t;

#define logicalQuota_PI "str *collName; double maxBytes; double maxObjects; double overBytes; double overObjects;"

/// The type that represents a list of logical quota entries.
///
/// \since 5.1.0
typedef struct LogicalQuotaList {
    /// The length of the list.
    ///
    /// \since 5.1.0
    int len;

    /// A pointer to an array of logical quota entries.
    /// There are len entries in this array.
    ///
    /// \since 5.1.0
    logicalQuota_t* list;
} logicalQuotaList_t;

#define logicalQuotaList_PI "int len; struct *logicalQuota_PI(len);"

/// Fetch configured logical quotas as well as their respective calculated over-values.
///
/// \param[in] _comm A pointer to a RcComm.
/// \param[in] _getLogicalQuotaInp \parblock
/// A pointer to a GetLogicalQuotaInput. Stores the collection name that will be used to find applicable quotas.
/// e.g. Passing in "/tempZone/home" will find quotas for "/", "/tempZone", and "/tempZone/home", if any of those quotas exist.
/// \endparblock
/// \param[out] _logicalQuotaList A pointer to a LogicalQuotaList pointer that will hold the results of the fetch.
///
/// On success, *_logicalQuotaList will hold a heap-allocated LogicalQuotaList. Within *_logicalQuotaList, there will be a pointer to a heap-allocated array of len LogicalQuota. This array must be free()'d by the caller to leaks. *_logicalQuotaList must also be free()'d to avoid leaks.
///
/// \usage \parblock
/// \code{c}
///     RcComm* comm = // Our iRODS connection.
///
///     struct LogicalQuotaList *out = NULL;
///
///     struct GetLogicalQuotaInput inp;
///     memset(&inp, 0, sizeof(struct GetLogicalQuotaInput));
///     // Don't forget to free() this afterward
///     inp.coll_name = strdup("/tempZone/home/alice/foo");
///
///     // Returns a list of quotas applied to /, /tempZone,
///     // /tempZone/home, /tempZone/home/alice, and /tempZone/home/alice/foo
///     // if any of them exist.
///     const int ec = rc_get_logical_quota(comm, &inp, &out);
///     if (ec < 0) {
///         // Error handling.
///     }
///
///     // Process returned quotas.
///     for(int i = 0; i < (*out)->len; i++) {
///         struct LogicalQuota *entry = (*out)->list[i];
///         // Do something with the entry.
///     }
///     // Alternatively, use clearLogicalQuotaList().
///     free((*out)->list);
///     free(*out);
///
///     // strdup()'ed above
///     free(inp.coll_name);
/// \endcode
/// \endparblock
///
/// \return An integer representing an iRODS error code.
/// \retval 0 on success.
/// \retval <0 on failure.
///
/// \since 5.1.0
int rc_get_logical_quota( struct RcComm *_conn, getLogicalQuotaInp_t *_getLogicalQuotaInp, logicalQuotaList_t **_logicalQuotaList );

#endif  // IRODS_GET_LOGICAL_QUOTA_H
