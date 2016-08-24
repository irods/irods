#ifndef DATA_OBJ_OPEN_AND_STAT_H__
#define DATA_OBJ_OPEN_AND_STAT_H__

#include "rodsType.h"
#include "rcConnect.h"
#include "dataObjInpOut.h"

typedef struct OpenStat {
    rodsLong_t dataSize;
    char dataType[NAME_LEN];
    char dataMode[SHORT_STR_LEN];
    int l3descInx;
    int replStatus;
    int rescTypeInx;
    int replNum;
} openStat_t;
#define OpenStat_PI "double dataSize; str dataType[NAME_LEN]; str dataMode[SHORT_STR_LEN]; int l3descInx; int replStatus; int rescTypeInx; int replNum;"

/* prototype for the client call */
/* rcDataObjOpenAndStat - Open And Stat a iRODS data object. This is the same
 * as the rcDataObjOpen call except it returns a openStat_t output.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInp_t *dataObjInp - generic dataObj input. Relevant items are:
 *      objPath - the path of the data object.
 *      dataType - the data type of the object (optional).
 *      openFlag - O_WRONLY, O_RDONLY , etc. The O_CREAT will create the
 *          file if it does not exist.
 *      condInput - condition input (optional).
 *          DATA_TYPE_KW - "value" = the data type of the object
 *          FORCE_FLAG_KW - overwrite an existing data object
 *          REG_CHKSUM_KW - compute the checksum value
 *          VERIFY_CHKSUM_KW - compute and verify the checksum on the data.
 *          FILE_PATH_KW - "value" = the physical path of the
 *              destination file. Vaild only if O_CREAT is on.
 *          REPL_NUM_KW  - "value" = The replica number of the copy to
 *              open.
 *          DEST_RESC_NAME_KW - "value" = The destination Resource. Vaild
 *              only if O_CREAT is on.
 *
 * OutPut -
 *   int l1descInx - an integer descriptor.
 */

#ifdef __cplusplus
extern "C"
#endif
int rcDataObjOpenAndStat( rcComm_t *conn, dataObjInp_t *dataObjInp, openStat_t **openStat );

#endif
