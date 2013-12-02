/* This is script-generated code.  */ 
/* See generalRowInsert.h for a description of this API call.*/

#include "generalRowInsert.h"

int
rcGeneralRowInsert (rcComm_t *conn, generalRowInsertInp_t *generalRowInsertInp)
{
    int status;
    status = procApiRequest (conn, GENERAL_ROW_INSERT_AN,  generalRowInsertInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
