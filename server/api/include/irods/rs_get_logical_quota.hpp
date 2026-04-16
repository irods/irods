#ifndef IRODS_RS_GET_LOGICAL_QUOTA_HPP
#define IRODS_RS_GET_LOGICAL_QUOTA_HPP

/// \file

#include "irods/get_logical_quota.h"

struct RsComm;


/// \brief Fetch configured logical quotas as well as their respective calculated over-values.
///
/// \param[in] _comm A pointer to a RsComm.
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
///     const int ec = rs_get_logical_quota(comm, &inp, &out);
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
int rs_get_logical_quota( struct RsComm *_rsComm, getLogicalQuotaInp_t *_getLogicalQuotaInp, logicalQuotaList_t **_logicalQuotaList );

#endif // IRODS_RS_GET_LOGICAL_QUOTA_HPP
