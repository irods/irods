#ifndef DATA_OBJ_COPY_H__
#define DATA_OBJ_COPY_H__

#include "objInfo.h"
#include "dataObjInpOut.h"
#include "rcConnect.h"

/**
 * \var dataObjCopyInp_t
 * \brief Input struct for Copy Data object operation
 * \since 1.0
 *
 * \ingroup capi_input_data_structures
 *
 * \remark none
 *
 * \note
 * Elements of dataObjCopyInp_t:
 * \li dataObjInp_t srcDataObjInp - dataObjInp_t for the source data object.
 * \li dataObjInp_t destDataObjInp - dataObjInp_t for the target data object.
 *
 * \sa none
 */

typedef struct DataObjCopyInp {
    dataObjInp_t srcDataObjInp;
    dataObjInp_t destDataObjInp;
} dataObjCopyInp_t;
#define DataObjCopyInp_PI "struct DataObjInp_PI; struct DataObjInp_PI;"

/* prototype for the client call */
/* rcDataObjCopy - Copy a iRODS data object.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjCopyInp_t *dataObjCopyInp - Relevant items are:
 *      dataObjInp_t srcDataObjInp - The source dataObj. Relevant items are:
 *          objPath - the source object path.
 *      destDataObjInp - The destination dataObj. Relevant items are:
 *          objPath - the destination object path.
 *          condInput - DATA_TYPE_KW - "value" = the data type of the object
 *                      FORCE_FLAG_KW - overwrite an existing data object
 *                      REG_CHKSUM_KW - compute the checksum value
 *                      VERIFY_CHKSUM_KW - compute and verify the checksum on
 *                        the data.
 *                      FILE_PATH_KW - "value" = the physical path of the
 *                        destination file.
 *                      DEST_RESC_NAME_KW - "value" = The destination Resource.
 *          numThreads - number of threads to use. NO_THREADING = no threading.
 *          openFlags - Open flag for the copy operation. Should be O_WRONLY.
 *          createMode - the file mode (optional).
 *
 * OutPut -
 *    int status of the operation - >= 0 ==> success, < 0 ==> failure.
 */

#ifdef __cplusplus
extern "C"
#endif
int rcDataObjCopy( rcComm_t *conn, dataObjCopyInp_t *dataObjCopyInp );
int _rcDataObjCopy( rcComm_t *conn, dataObjCopyInp_t *dataObjCopyInp, transferStat_t **transferStat );

#endif
