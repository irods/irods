/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
/* structFileSync.h
 */

#ifndef STRUCT_FILE_SYNC_HPP
#define STRUCT_FILE_SYNC_HPP

/* This is Object File I/O type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"

/* definition for flags */

typedef struct StructFileOprInp {
    rodsHostAddr_t addr;
    int oprType;  /* see syncMountedColl.h */ // JMC - backport 4643
    int flags;
    specColl_t *specColl;
    keyValPair_t condInput;   /* include chksum flag and value */
} structFileOprInp_t;

#define StructFileOprInp_PI "struct RHostAddr_PI; int oprType; int flags; struct *SpecColl_PI; struct KeyValPair_PI;"

#if defined(RODS_SERVER)
#define RS_STRUCT_FILE_SYNC rsStructFileSync
/* prototype for the server handler */
int
rsStructFileSync( rsComm_t *rsComm, structFileOprInp_t *structFileOprInp );
int
_rsStructFileSync( rsComm_t *rsComm, structFileOprInp_t *structFileOprInp );
int
remoteStructFileSync( rsComm_t *rsComm, structFileOprInp_t *structFileOprInp,
                      rodsServerHost_t *rodsServerHost );
#else
#define RS_STRUCT_FILE_SYNC NULL
#endif

/* prototype for the client call */
int
rcStructFileSync( rcComm_t *conn, structFileOprInp_t *structFileOprInp );

#endif	/* STRUCT_FILE_SYNC_H */
