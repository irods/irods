/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* collRepl.h - recursively replicate a collection
 */

#ifndef COLL_REPL_HPP
#define COLL_REPL_HPP

/* This is a Object File I/O API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "dataObjWrite.hpp"
#include "dataObjClose.hpp"
#include "dataCopy.hpp"

#if defined(RODS_SERVER)
#define RS_COLL_REPL rsCollRepl
/* prototype for the server handler */
int
rsCollRepl( rsComm_t *rsComm, collInp_t *collReplInp,
            collOprStat_t **collOprStat );
#else
#define RS_COLL_REPL NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */
int
rcCollRepl( rcComm_t *conn, collInp_t *collReplInp, int vFlag );
int
_rcCollRepl( rcComm_t *conn, collInp_t *collReplInp,
             collOprStat_t **collOprStat );

#ifdef __cplusplus
}
#endif
#endif	/* COLL_REPL_H */
