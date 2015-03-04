/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* regColl.h
 */

#ifndef REG_COLL_HPP
#define REG_COLL_HPP

/* This is Object File I/O type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "dataObjInpOut.hpp"

#if defined(RODS_SERVER)
#define RS_REG_COLL rsRegColl
/* prototype for the server handler */
int
rsRegColl( rsComm_t *rsComm, collInp_t *regCollInp );
int
_rsRegColl( rsComm_t *rsComm, collInp_t *regCollInp );
#else
#define RS_REG_COLL NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */
int
rcRegColl( rcComm_t *conn, collInp_t *regCollInp );

/* rcRegColl - Register a iRODS collection.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   collInp_t *collInp - generic coll input. Relevant items are:
 *      collName - the collection to be registered.
 *      condInput - condition input (optional) - currently not used.
 *
 * OutPut -
 *   int status - status of the operation.
 */

#ifdef __cplusplus
}
#endif
#endif	/* REG_COLL_H */
