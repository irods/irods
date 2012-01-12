/* This is script-generated code.  */ 
/* See databaseRescOpen.h for a description of this API call.*/

#include "databaseRescOpen.h"

int
rcDatabaseRescOpen (rcComm_t *conn, 
		      databaseRescOpenInp_t *databaseRescOpenInp)
{
    int status;
    status = procApiRequest (conn, DATABASE_RESC_OPEN_AN,  databaseRescOpenInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
