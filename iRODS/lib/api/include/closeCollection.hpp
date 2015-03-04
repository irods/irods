/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* closeCollection.h
 */

#ifndef CLOSE_COLLECTION_HPP
#define CLOSE_COLLECTION_HPP

/* This is a Object File I/O API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "dataObjInpOut.hpp"

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

int
rcCloseCollection( rcComm_t *conn, int handleInxInp );

/* rcCloseCollection - Close a iRODS collection.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   int handleInxInp - the handleInx (collection handle index) to close.
 *
 * OutPut -
 *   int status - status of the operation.
 */
#ifdef __cplusplus
}
#endif
#endif	/* CLOSE_COLLECTION_H */
