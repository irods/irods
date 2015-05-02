/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* closeCollection.h
 */

#ifndef CLOSE_COLLECTION_H__
#define CLOSE_COLLECTION_H__

/* This is a Object File I/O API call */

#include "objInfo.h"
#include "dataObjInpOut.h"
#include "rcConnect.h"

#if defined(RODS_SERVER)
#define RS_CLOSE_COLLECTION rsCloseCollection
/* prototype for the server handler */
int
rsCloseCollection( rsComm_t *rsComm, int *handleInxInp );
#else
#define RS_CLOSE_COLLECTION NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */

/* rcCloseCollection - Close a iRODS collection.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   int handleInxInp - the handleInx (collection handle index) to close.
 *
 * Output -
 *   int status - status of the operation.
 */
int
rcCloseCollection( rcComm_t *conn, int handleInxInp );

#ifdef __cplusplus
}
#endif
#endif	// CLOSE_COLLECTION_H__
