/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* generalRowPurge.h

   This client/server calls is used to purge rows from certain
   special-purpose tables.  See the rs file for the current list.
   Admin only.  Also see generalRowInsert.
 */

#ifndef GENERAL_ROW_PURGE_H
#define GENERAL_ROW_PURGE_H

/* This is a Metadata type API call */


#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "icatDefines.h"

typedef struct {
   char *tableName;
   char *secondsAgo;
} generalRowPurgeInp_t;
    
#define generalRowPurgeInp_PI "str *tableName; str *secondsAgo;"

#ifdef  __cplusplus
extern "C" {
#endif

#if defined(RODS_SERVER)
#define RS_GENERAL_ROW_PURGE rsGeneralRowPurge
/* prototype for the server handler */
int
rsGeneralRowPurge (rsComm_t *rsComm, generalRowPurgeInp_t *generalRowPurgeInp );

int
_rsGeneralRowPurge (rsComm_t *rsComm, generalRowPurgeInp_t *generalRowPurgeInp );
#else
#define RS_GENERAL_ROW_PURGE NULL
#endif

/* prototype for the client call */
int
rcGeneralRowPurge (rcComm_t *conn, generalRowPurgeInp_t *generalRowPurgeInp);

#ifdef  __cplusplus
}
#endif

#endif	/* GENERAL_ROW_PURGE_H */
