/* This is script-generated code.  */ 
/* See databaseRescClose.h for a description of this API call.*/

#include "databaseRescClose.h"

int
rcDatabaseRescClose (rcComm_t *conn, 
		     databaseRescCloseInp_t *databaseRescCloseInp)
{
    int status;
    status = procApiRequest (conn, DATABASE_RESC_CLOSE_AN, 
			     databaseRescCloseInp, NULL, 
			     (void **) NULL, NULL);
    return (status);
}
