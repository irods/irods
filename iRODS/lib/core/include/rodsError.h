/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rodsError.h - header file for rods error
 */



#ifndef RODS_ERROR_H
#define RODS_ERROR_H

#define USE_EXTERN_TABLE
#include "rodsErrorTable.h"

/* The error struct */

#define ERR_MSG_LEN     1024

typedef struct {
    int status;
    char msg[ERR_MSG_LEN];
} rErrMsg_t;

typedef struct {
    int len;          /* number of error in the stack */
    rErrMsg_t **errMsg; /* an array of pointers to the rErrMsg_t struct */
} rError_t;

#endif	/* RODS_ERROR_H */
