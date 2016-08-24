#ifndef DATA_OBJ_CREATE_AND_STAT_H__
#define DATA_OBJ_CREATE_AND_STAT_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"
#include "dataObjOpenAndStat.h"

/* prototype for the client call */
/* rcDataObjCreateAndStat - Create And Stat a iRODS data object. This is the same
 * as the rcDataObjCreate call except it returns a openStat_t output.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInp_t *dataObjInp - generic dataObj input. Relevant items are:
 *      objPath - the path of the data object.
 *      rescName - the input resource (optional)
 *      dataType - the data type of the object (optional).
 *      filePath - the physical file path (optional).
 *      numThreads - number of threads to use. NO_THREADING = no threading.
 *      createMode - the file mode (optional)
 *      openFlag - O_WRONLY, O_RDONLY , etc
 *      condInput - condition input (optional).
 *          DATA_TYPE_KW - "value" = the data type of the object
 *          FORCE_FLAG_KW - overwrite an existing data object
 *          REG_CHKSUM_KW - compute the checksum value
 *          VERIFY_CHKSUM_KW - compute and verify the checksum on the data.
 *          FILE_PATH_KW - "value" = the physical path of the
 *              destination file.
 *          REPL_NUM_KW  - "value" = The replica number of the copy to
 *              overwrite.
 *          DEST_RESC_NAME_KW - "value" = The destination Resource.
 *
 * OutPut -
 *   int l1descInx - an integer descriptor.
 */
#ifdef __cplusplus
extern "C"
#endif
int rcDataObjCreateAndStat( rcComm_t *conn, dataObjInp_t *dataObjInp, openStat_t **openStat );

#endif
