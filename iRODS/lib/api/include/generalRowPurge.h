/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#ifndef GENERAL_ROW_PURGE_H__
#define GENERAL_ROW_PURGE_H__

/* This is a Metadata type API call */

#include "rcConnect.h"

typedef struct {
    char *tableName;
    char *secondsAgo;
} generalRowPurgeInp_t;

#define generalRowPurgeInp_PI "str *tableName; str *secondsAgo;"

#if defined(RODS_SERVER)
#define RS_GENERAL_ROW_PURGE rsGeneralRowPurge
/* prototype for the server handler */
int
rsGeneralRowPurge( rsComm_t *rsComm, generalRowPurgeInp_t *generalRowPurgeInp );

int
_rsGeneralRowPurge( rsComm_t *rsComm, generalRowPurgeInp_t *generalRowPurgeInp );
#else
#define RS_GENERAL_ROW_PURGE NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */
int
rcGeneralRowPurge( rcComm_t *conn, generalRowPurgeInp_t *generalRowPurgeInp );

#ifdef __cplusplus
}
#endif
#endif	// GENERAL_ROW_PURGE_H__
