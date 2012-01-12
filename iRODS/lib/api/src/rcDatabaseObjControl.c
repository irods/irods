/* This is script-generated code.  */ 
/* See databaseObjControl.h for a description of this API call.*/

#include "databaseObjControl.h"

int
rcDatabaseObjControl (rcComm_t *conn, 
		      databaseObjControlInp_t *databaseObjControlInp, 
		      databaseObjControlOut_t **databaseObjControlOut)
{
    int status;
    status = procApiRequest (conn, DATABASE_OBJ_CONTROL_AN,
			     databaseObjControlInp, NULL, 
			     (void **) databaseObjControlOut, NULL);
    return (status);
}
