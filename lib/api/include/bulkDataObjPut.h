#ifndef BULK_DATA_OBJ_PUT_H__
#define BULK_DATA_OBJ_PUT_H__

#include "rodsGenQuery.h"
#include "rodsDef.h"
#include "rcConnect.h"

#define TMP_PHY_BUN_DIR "tmpPhyBunDir"

typedef struct BulkOperationInp {
    char objPath[MAX_NAME_LEN];
    genQueryOut_t attriArray;   /* arrays of attrib - chksum */
    keyValPair_t condInput;   /* include chksum flag and value */
} bulkOprInp_t;

typedef struct RenamedPhyFiles {
    int count;
    char objPath[MAX_NUM_BULK_OPR_FILES][MAX_NAME_LEN];
    char origFilePath[MAX_NUM_BULK_OPR_FILES][MAX_NAME_LEN];
    char newFilePath[MAX_NUM_BULK_OPR_FILES][MAX_NAME_LEN];
} renamedPhyFiles_t;

#define BulkOprInp_PI "str objPath[MAX_NAME_LEN]; struct GenQueryOut_PI; struct KeyValPair_PI;"

#ifdef __cplusplus
extern "C" {
#endif

/* prototype for the client call */
/* rcBulkDataObjPut - Bulk Put (upload) a number of local files to iRODS.
 * bulkOprInpBBuf contains the bundled local files in tar format.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   bulkOprInp_t *bulkOprInp - generic dataObj input. Relevant items are:
 *      objPath - the collection path under which the files are to be untar.
 *      condInput - conditional Input
 *          FORCE_FLAG_KW - overwrite an existing data object
 *          DATA_TYPE_KW - "value" = the data type of the file.
 *          DEST_RESC_NAME_KW - "value" = The destination Resource.
 *   return value - The status of the operation.
 */
int rcBulkDataObjPut(rcComm_t* conn, bulkOprInp_t* bulkOprInp, bytesBuf_t* bulkOprInpBBuf);

#ifdef __cplusplus
}
#endif

#endif  // BULK_DATA_OBJ_PUT_H__
