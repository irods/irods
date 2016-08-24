#ifndef OBJ_STAT_H__
#define OBJ_STAT_H__

#include "rcConnect.h"
#include "rodsType.h"

// rodsObjStat_t - this is similar to rodsStat_t but has minimum set of parameter that are more irods specific
typedef struct rodsObjStat {
    rodsLong_t          objSize;        // file size
    objType_t           objType;        // DATA_OBJ_T or COLL_OBJ_T
    uint                dataMode;
    char                dataId[NAME_LEN];
    char                chksum[NAME_LEN];
    char                ownerName[NAME_LEN];
    char                ownerZone[NAME_LEN];
    char                createTime[TIME_LEN];
    char                modifyTime[TIME_LEN];
    specColl_t          *specColl;
    char                rescHier[MAX_NAME_LEN];
} rodsObjStat_t;


/* prototype for the client call */
/* rcObjStat - get the stat of an object specified by dataObjInp->objPath.
 * input:  dataObjInp
 *
 * output: rodsObjStatOut
 *   The objType can be COLL_OBJ_T (collection), DATA_OBJ_T (data object)
 *   or UNKNOWN_OBJ_T (object does not exist).
 *   If "specColl" is not NULL, the input objPath is a Special Collection or
 *   in a Special Collection. If objType is UNKNOWN_OBJ_T and "specColl"
 *   is not NULL, the object does not exist but the objPath is in a
 *   Special Collection.
 *   Important items in the specColl_t are:
 *     collClass - can be STRUCT_FILE_COLL (mounted structured file),
 *     MOUNTED_COLL (mounted collection) or LINKED_COLL (linked collection).
 *     objPath - If collClass is LINKED_COLL, this is the translated path
 *     for the input objPath. The client should use this path instead of
 *     the input "objPath" for further metadata query.
 */
#ifdef __cplusplus
extern "C"
#endif
int rcObjStat( rcComm_t *conn, dataObjInp_t *dataObjInp, rodsObjStat_t **rodsObjStatOut );

#endif
