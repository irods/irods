/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* getTempPassword.h
 */

#ifndef GET_TEMP_PASSWORD_H
#define GET_TEMP_PASSWORD_H

/* This is a high level Metadata type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"

typedef struct {
   char stringToHashWith[MAX_PASSWORD_LEN];
} getTempPasswordOut_t;

/* #define getTempPasswordOut_PI "str stringToHashWith[NAME_LEN];" */
#define getTempPasswordOut_PI "str stringToHashWith[MAX_PASSWORD_LEN];"

#if defined(RODS_SERVER)
#define RS_GET_TEMP_PASSWORD rsGetTempPassword
/* prototype for the server handler */
int
rsGetTempPassword (rsComm_t *rsComm, 
		   getTempPasswordOut_t **getTempPasswordOut);
int
_rsGetTempPassword (rsComm_t *rsComm, 
		    getTempPasswordOut_t **getTempPasswordOut);
#else
#define RS_GET_TEMP_PASSWORD NULL
#endif

/* prototype for the client call */
int
rcGetTempPassword (rcComm_t *conn, getTempPasswordOut_t **getTempPasswordOut);

#endif	/* GET_TEMP_PASSWORD_H */
