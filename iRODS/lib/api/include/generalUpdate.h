/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#ifndef GENERAL_UPDATE_H__
#define GENERAL_UPDATE_H__

#include "rcConnect.h"
#include "rodsGeneralUpdate.h"  /* for input/output structs, etc */

#if defined(RODS_SERVER)
#define RS_GENERAL_UPDATE rsGeneralUpdate
/* prototype for the server handler */
int
rsGeneralUpdate( rsComm_t *rsComm, generalUpdateInp_t *generalUpdateInp );

int
_rsGeneralUpdate( generalUpdateInp_t *generalUpdateInp );
#else
#define RS_GENERAL_UPDATE NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */
int
rcGeneralUpdate( rcComm_t *conn, generalUpdateInp_t *generalUpdateInp );

#ifdef __cplusplus
}
#endif
#endif	// GENERAL_UPDATE_H__
