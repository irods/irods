/*** Copyright (c), The Regents of the University of California            ***
 *** For more openrmation please refer to files in the COPYRIGHT directory ***/
/* databaseRescOpen.h
 */

#ifndef DATABASE_RESC_OPEN_H
#define DATABASE_RESC_OPEN_H

/* This is a metadata type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "icatDefines.h"

typedef struct {
   char *dbrName; /* database resource name */
} databaseRescOpenInp_t;
    
#define databaseRescOpenInp_PI "str *dbrName;"

#if defined(RODS_SERVER)
#define RS_DATABASE_RESC_OPEN rsDatabaseRescOpen
/* prototype for the server handler */
int
rsDatabaseRescOpen (rsComm_t *rsComm, 
		      databaseRescOpenInp_t *databaseRescOpenInp);
int
_rsDatabaseRescOpen (rsComm_t *rsComm,
		       databaseRescOpenInp_t *databaseRescOpenInp);
#else
#define RS_DATABASE_RESC_OPEN NULL
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* prototype for the client call */
int
rcDatabaseRescOpen (rcComm_t *conn, 
		      databaseRescOpenInp_t *databaseRescOpenInp);

#ifdef  __cplusplus
}
#endif

#endif	/* DATABASE_RESC_OPEN_H */
