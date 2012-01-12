/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* databaseObjControl.h
 */

#ifndef DATABASE_OBJ_CONTROL_H
#define DATABASE_OBJ_CONTROL_H

/* This is a metadata type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "icatDefines.h"

#define DBO_EXECUTE 1   /* Option to execute a DBO */
#define DBR_COMMIT  2   /* Option to commit */
#define DBR_ROLLBACK 3  /* Option to rollback */

typedef struct {
   char *dbrName;   /* DBR name */
   char *dboName;   /* DBO name */
   int  option;     /* option to perform */
   int  subOption;  /* sub-option on some options */
   char *dborName;  /* DBOR name, if any */
   char *args[10];  /* optional arguments for execute */
} databaseObjControlInp_t;
    
#define databaseObjControlInp_PI "str *dbrName; str *dboName; int option; int subOption; str *dborName; str *arg1; str *arg2; str *arg3; str *arg4; str *arg5; str *arg6; str *arg7; str *arg8; str *arg9; str *arg10;"

typedef struct {
   char *outBuf;
} databaseObjControlOut_t;

#define databaseObjControlOut_PI "str *outBuf;"

#if defined(RODS_SERVER)
#define RS_DATABASE_OBJ_CONTROL rsDatabaseObjControl
/* prototype for the server handler */
int
rsDatabaseObjControl (rsComm_t *rsComm, 
		      databaseObjControlInp_t *databaseObjControlInp, 
		      databaseObjControlOut_t **databaseObjControlOut);
int
_rsDatabaseObjControl (rsComm_t *rsComm,
		       databaseObjControlInp_t *databaseObjControlInp,
		       databaseObjControlOut_t **databaseObjControlOut);
#else
#define RS_DATABASE_OBJ_CONTROL NULL
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* prototype for the client call */
int
rcDatabaseObjControl (rcComm_t *conn, 
		      databaseObjControlInp_t *databaseObjControlInp, 
		      databaseObjControlOut_t **databaseObjControlOut);

#ifdef  __cplusplus
}
#endif

#endif	/* DATABASE_OBJ_CONTROL_H */
