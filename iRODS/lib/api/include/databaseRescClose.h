/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* databaseRescClose.h

   This client/server call is to close an open DBR.
 */

#ifndef DATABASE_RESC_CLOSE_H
#define DATABASE_RESC_CLOSE_H

/* This is a metadata type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "icatDefines.h"

typedef struct {
   char *dbrName;
} databaseRescCloseInp_t;
    
#define databaseRescCloseInp_PI "str *dbrName;"

#if defined(RODS_SERVER)
#define RS_DATABASE_RESC_CLOSE rsDatabaseRescClose
/* prototype for the server handler */
int
rsDatabaseRescClose (rsComm_t *rsComm, 
		     databaseRescCloseInp_t *databaseRescCloseInp);

int
_rsDatabaseRescClose (rsComm_t *rsComm,
		      databaseRescCloseInp_t *databaseRescCloseInp);
#else
#define RS_DATABASE_RESC_CLOSE NULL
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* prototype for the client call */
int
rcDatabaseRescClose (rcComm_t *conn, 
		     databaseRescCloseInp_t *databaseRescCloseInp);

#ifdef  __cplusplus
}
#endif

#endif	/* DATABASE_RESC_CLOSE_H */
