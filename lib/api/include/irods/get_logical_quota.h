#ifndef IRODS_GET_LOGICAL_QUOTA_H
#define IRODS_GET_LOGICAL_QUOTA_H

/// \file

#include "irods/objInfo.h"
#include "irods/rodsType.h"

struct RcComm;

#ifdef __cplusplus
extern "C" {
#endif

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

    /// The number of bytes by which the collection size exceeds the current quota's max_bytes.
    /// e.g. over_bytes = total_collection_size - max_bytes
    /// This value will be negative or zero if the limit has not been exceeded.
    ///
    /// \since 5.1.0
    rodsLong_t over_bytes;

    /// The number of objects by which the collection size exceeds the current quota's max_objects.
    /// e.g. over_objects = total_collection_size - max_objects
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
    /// There are "len" entries in this array.
    ///
    /// \since 5.1.0
    logicalQuota_t* list;
} logicalQuotaList_t;

#define logicalQuotaList_PI "int len; struct *logicalQuota_PI(len);"

/// \brief Free memory associated with a heap-allocated GetLogicalQuotaInput.
///
/// This function assumes that the coll_name inside of the structure is also heap-allocated. Do not use this function if that pointer is not to be free()'ed.
///
/// \param[in] _get_logical_quota_input A void pointer that can be casted to a struct GetLogicalQuotaInput pointer.
///
/// This function has no effect if the input pointer is null.
///
/// \since 5.1.0
void clear_get_logical_quota_input(void* _get_logical_quota_input);

/// \brief Free memory associated with a heap-allocated LogicalQuotaList.
///
/// \param[in] _logical_quota_list A void pointer that can be casted to a struct LogicalQuotaList pointer.
///
/// This function has no effect if the input pointer is null.
///
/// \since 5.1.0
void clear_logical_quota_list(void* _logical_quota_list);

/// \brief Fetch configured logical quotas as well as their respective calculated over-values.
///
/// \parblock
/// On success, *_logicalQuotaList will be a pointer to a heap-allocated LogicalQuotaList. Within *_logicalQuotaList, there will be a pointer to a heap-allocated array of "len" LogicalQuota structs. This array must be free()'d by the caller to avoid leaks. *_logicalQuotaList must also be free()'d to avoid leaks.
/// To avoid free()ing the LogicalQuotaList internals by hand, clear_logical_quota_list() can be used. Note that the struct itself, *_logicalQuotaList, still needs to be free()'d by the caller.
/// \endparblock
///
/// \param[in] _comm A pointer to a RcComm.
/// \param[in] _getLogicalQuotaInp \parblock
/// A pointer to a GetLogicalQuotaInput. Stores the collection name that will be used to find applicable quotas.
/// e.g. Passing in "/tempZone/home" will find quotas for "/", "/tempZone", and "/tempZone/home", if any of those quotas exist.
/// \endparblock
/// \param[out] _logicalQuotaList A pointer to a LogicalQuotaList pointer that will hold the results of the fetch.
///
/// \return An integer representing an iRODS error code.
/// \retval 0 on success.
/// \retval <0 on failure.
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
///     for (int i = 0; i < out->len; i++) {
///         struct LogicalQuota *entry = out->list[i];
///         // Do something with the entry.
///     }
///
///     // Alternatively, free by hand-- see implementation for details.
///     clear_logical_quota_list(out);
///     free(out);
///     clear_get_logical_quota_input(&inp);
///
/// \endcode
/// \endparblock
///
/// \since 5.1.0
int rc_get_logical_quota( struct RcComm *_conn, getLogicalQuotaInp_t *_getLogicalQuotaInp, logicalQuotaList_t **_logicalQuotaList );

#ifdef __cplusplus
} // extern "C"
#endif

#endif  // IRODS_GET_LOGICAL_QUOTA_H
