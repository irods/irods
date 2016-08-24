/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* dataObjOpen.h
 */

#ifndef DATA_OBJ_OPEN_H__
#define DATA_OBJ_OPEN_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"
#include "objInfo.h"

/* definition for phyOpenFlag */
#define DO_PHYOPEN      0
#define DO_NOT_PHYOPEN  1
#define PHYOPEN_BY_SIZE 2

/* prototype for the client call */
/* rcDataObjOpen - Open a iRODS data object.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInp_t *dataObjInp - generic dataObj input. Relevant items are:
 *	objPath - the path of the data object.
 *	dataType - the data type of the object (optional).
 *	openFlag - O_WRONLY, O_RDONLY , etc. The O_CREAT will create the
 *          file if it does not exist.
 *	condInput - condition input (optional).
 *          DATA_TYPE_KW - "value" = the data type of the object
 *          FORCE_FLAG_KW - overwrite an existing data object
 *          REG_CHKSUM_KW - compute the checksum value
 *          VERIFY_CHKSUM_KW - compute and verify the checksum on the data.
 *          FILE_PATH_KW - "value" = the physical path of the
 *              destination file. Vaild only if O_CREAT is on.
 *          REPL_NUM_KW  - "value" = The replica number of the copy to
 *              open.
 *          DEST_RESC_NAME_KW - "value" = The destination Resource. Vaild
 *	        only if O_CREAT is on.
 *
 * OutPut -
 *   int l1descInx - an integer descriptor.
 */


#ifdef __cplusplus
extern "C"
#endif
int rcDataObjOpen( rcComm_t *conn, dataObjInp_t *dataObjInp );

#endif
