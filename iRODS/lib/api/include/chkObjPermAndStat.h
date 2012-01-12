/*** Copyright (c), The Unregents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* chkObjPermAndStat.h
 */

#ifndef CHK_OBJ_PERM_AND_STAT_H
#define CHK_OBJ_PERM_AND_STAT_H

/* This is Object File I/O type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "dataObjInpOut.h"

/* definition for flags */
#define CHK_COLL_FOR_BUNDLE_OPR		0x1
#
typedef struct {
    char objPath[MAX_NAME_LEN];
    char permission[NAME_LEN];
    int flags;
    int status;
    keyValPair_t condInput;
} chkObjPermAndStat_t;

#define ChkObjPermAndStat_PI "str objPath[MAX_NAME_LEN]; str permission[NAME_LEN]; int flags; int status; struct KeyValPair_PI;"

#if defined(RODS_SERVER)
#define RS_CHK_OBJ_PERM_AND_STAT rsChkObjPermAndStat
/* prototype for the server handler */
int
rsChkObjPermAndStat (rsComm_t *rsComm, 
chkObjPermAndStat_t *chkObjPermAndStatInp);
int
_rsChkObjPermAndStat (rsComm_t *rsComm, 
chkObjPermAndStat_t *chkObjPermAndStatInp);
int
chkCollForBundleOpr (rsComm_t *rsComm,
chkObjPermAndStat_t *chkObjPermAndStatInp);
#else
#define RS_CHK_OBJ_PERM_AND_STAT NULL
#endif

/* prototype for the client call */
int
rcChkObjPermAndStat (rcComm_t *conn, chkObjPermAndStat_t *chkObjPermAndStatInp);

/* rcChkObjPermAndStat - Unregister a iRODS dataObject.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   chkObjPermAndStat_t *chkObjPermAndStatInp - the dataObjInfo to unregister
 *
 * OutPut -
 *   int status - status of the operation.
 */

#endif	/* CHK_OBJ_PERM_AND_STAT_H */
