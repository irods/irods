#ifndef CHK_OBJ_PERM_AND_STAT_H__
#define CHK_OBJ_PERM_AND_STAT_H__

#include "rcConnect.h"

// definition for flags
#define CHK_COLL_FOR_BUNDLE_OPR      0x1

typedef struct {
    char objPath[MAX_NAME_LEN];
    char permission[NAME_LEN];
    int flags;
    int status;
    keyValPair_t condInput;
} chkObjPermAndStat_t;

#define ChkObjPermAndStat_PI "str objPath[MAX_NAME_LEN]; str permission[NAME_LEN]; int flags; int status; struct KeyValPair_PI;"

/* rcChkObjPermAndStat - Unregister a iRODS dataObject.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   chkObjPermAndStat_t *chkObjPermAndStatInp - the dataObjInfo to unregister
 *
 * OutPut -
 *   int status - status of the operation.
 */
int rcChkObjPermAndStat( rcComm_t *conn, chkObjPermAndStat_t *chkObjPermAndStatInp );

#endif
