#ifndef IRODS_RE_SYS_DATA_OBJ_OPR_HPP
#define IRODS_RE_SYS_DATA_OBJ_OPR_HPP

/// \file

#include "irods/rods.h"
#include "irods/objMetaOpr.hpp"
#include "irods/dataObjRepl.h"
#include "irods/modDataObjMeta.h"

int
msiSetDataObjPreferredResc( msParam_t *preferredResc, ruleExecInfo_t *rei );
int
msiSetDataObjAvoidResc( msParam_t *preferredResc, ruleExecInfo_t *rei );
int
msiSetDataTypeFromExt( ruleExecInfo_t *rei );
int
msiSetNoDirectRescInp( msParam_t *rescList, ruleExecInfo_t *rei );
int
msiSetDefaultResc( msParam_t *defaultResc, msParam_t *forceStr, ruleExecInfo_t *rei );
int
msiSetNumThreads( msParam_t *sizePerThrInMbStr, msParam_t *maxNumThrStr,
                  msParam_t *windowSizeStr, ruleExecInfo_t *rei );
int
msiDeleteDisallowed( ruleExecInfo_t *rei );
int
msiOprDisallowed( ruleExecInfo_t *rei );
int
msiNoChkFilePathPerm( ruleExecInfo_t *rei );
int // JMC - backport 4774
msiSetChkFilePathPerm( msParam_t *xchkType, ruleExecInfo_t *rei );
int
msiNoTrashCan( ruleExecInfo_t *rei );
int
msiSetPublicUserOpr( msParam_t *xoprList, ruleExecInfo_t *rei );
int
setApiPerm( int apiNumber, int proxyPerm, int clientPerm );
int
msiSetGraftPathScheme( msParam_t *xaddUserName, msParam_t *xtrimDirCnt,
                       ruleExecInfo_t *rei );
int
msiSetRandomScheme( ruleExecInfo_t *rei );

/// Sets the random scheme style.
///
/// This microservice allows administrators to change how physical paths are generated. The effects of
/// this microservice are not applied unless the vault path policy is set to the random scheme.
///
/// This microservice has no effect if invoked outside of acSetVaultPathPolicy() or an error occurs.
///
/// Styles:
/// - 0: Default style, i.e. <vault_root>/<int>/<int>/<filename>.<epoch_seconds>
/// - 1: Append random characters, i.e. <vault_root>/<int>/<int>/<filename>.<epoch_seconds>.<random_characters>
///
/// \param[in] _style \parblock The new style of the randomly-generated string. A value of 1 results
///                   in 5 randomly generated alphanumeric characters being appended to the physical
///                   path. Attempting to set the value to anything other than 0 or 1 will result in
///                   an error. See #msi_random_scheme_set_suffix_length for information about changing
///                   the length of the generated suffix string.
///                   \endparblock
/// \param[in] _rei   A pointer to a #RuleExecInfo object. The execution information. Hidden within
///                   the iRODS Rule Language.
///
/// \returns An integer indicating the status of the operation.
/// \retval  0 On success.
/// \retval <0 On failure.
///
/// \since 5.1.0
int msi_random_scheme_set_style(MsParam* _style, ruleExecInfo_t* _rei);

/// Sets the random scheme suffix length.
///
/// The effects of this microservice are not applied unless the random scheme style is set to 1.
/// See #msi_random_scheme_set_style for more information.
///
/// This microservice has no effect if invoked outside of acSetVaultPathPolicy() or an error occurs.
///
/// \param[in] _suffix_length The new length of the randomly-generated string. The value must
///                           satisfy the range [1, 32]. Failing to satisfy this requirement will
///                           result in an error.
/// \param[in] _rei           A pointer to a #RuleExecInfo object. The execution information. Hidden
///                           within the iRODS Rule Language.
///
/// \returns An integer indicating the status of the operation.
/// \retval  0 On success.
/// \retval <0 On failure.
///
/// \since 5.1.0
int msi_random_scheme_set_suffix_length(MsParam* _suffix_length, ruleExecInfo_t* _rei);

int
msiSetRescQuotaPolicy( msParam_t *xflag, ruleExecInfo_t *rei );

#endif // IRODS_RE_SYS_DATA_OBJ_OPR_HPP
