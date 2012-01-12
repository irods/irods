/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* dataObjCreate.h
 */

#ifndef DATA_OBJ_CREATE_H
#define DATA_OBJ_CREATE_H

/* This is a high level type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "dataObjInpOut.h"
#include "fileCreate.h"

#if defined(RODS_SERVER)
#define RS_DATA_OBJ_CREATE rsDataObjCreate
/* prototype for the server handler */
int
rsDataObjCreate (rsComm_t *rsComm, dataObjInp_t *dataObjInp);
int
_rsDataObjCreate (rsComm_t *rsComm, dataObjInp_t *dataObjInp);
int
specCollSubCreate (rsComm_t *rsComm, dataObjInp_t *dataObjInp);
int
_rsDataObjCreateWithRescInfo (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
rescInfo_t *rescInfo, char *rescGroupName);
int
dataObjCreateAndReg (rsComm_t *rsComm, int l1descInx);
int
dataCreate (rsComm_t *rsComm, int l1descInx);
int
l3Create (rsComm_t *rsComm, int l1descInx);
int
l3CreateByObjInfo (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
dataObjInfo_t *dataObjInfo);
int
getRescGrpForCreate (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
rescGrpInfo_t **myRescGrpInfo);
#else
#define RS_DATA_OBJ_CREATE NULL
#endif

/* prototype for the client call */
/* rcDataObjCreate - Create a iRODS data object.
 * Input - 
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInp_t *dataObjInp - generic dataObj input. Relevant items are:
 *	objPath - the path of the data object.
 *	rescName - the input resource (optional)
 *	dataType - the data type of the object (optional).
 *	filePath - the physical file path (optional).
 *      numThreads - number of threads to use. NO_THREADING = no threading.
 *	createMode - the file mode (optional)
 *	openFlag - O_WRONLY, O_RDONLY , etc 
 *	condInput - condition input (optional).
 *	    DATA_TYPE_KW - "value" = the data type of the object
 *	    FORCE_FLAG_KW - overwrite an existing data object
 *          REG_CHKSUM_KW - compute the checksum value
 *          VERIFY_CHKSUM_KW - compute and verify the checksum on the data.
 *	    FILE_PATH_KW - "value" = the physical path of the
 *              destination file.
 *	    REPL_NUM_KW  - "value" = The replica number of the copy to 
 *		overwrite.
 *	    DEST_RESC_NAME_KW - "value" = The destination Resource.
 *
 * OutPut - 
 *   int l1descInx - an integer descriptor.   
 */

#ifdef  __cplusplus
extern "C" {
#endif

int
rcDataObjCreate (rcComm_t *conn, dataObjInp_t *dataObjInp);

#ifdef  __cplusplus
}
#endif

#endif	/* DATA_OBJ_CREATE_H */
