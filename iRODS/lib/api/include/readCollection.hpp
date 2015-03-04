/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* readCollection.h
 */

#ifndef READ_COLLECTION_HPP
#define READ_COLLECTION_HPP

/* This is a high level type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "dataObjInpOut.hpp"
#include "dataObjRead.hpp"
#include "miscUtil.hpp"

#if defined(RODS_SERVER)
#define RS_READ_COLLECTION rsReadCollection
/* prototype for the server handler */
int
rsReadCollection( rsComm_t *rsComm, int *handleInxInp,
                  collEnt_t **collEnt );
#else
#define RS_READ_COLLECTION NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */

int
rcReadCollection( rcComm_t *conn, int handleInxInp,
                  collEnt_t **collEnt );

/* rcReadCollection - Read a iRODS collection. rcOpenCollection must be
 *    called first.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjReadInp_t *readCollInp - generic read input. Relevant items are:
 *      l1descInx - the handleInx from the rcOpenCollection call.
 *
 * OutPut -
 *   collEnt_t **collEnt - the metadata of an object in the collection.
 *   int status - status of the operation. -1 means no more collEnt
 */
#ifdef __cplusplus
}
#endif
#endif	/* READ_COLLECTION_H */
