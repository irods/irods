/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See chkNVPathPerm.h for a description of this API call.*/

#include "chkNVPathPerm.h"

int
rcChkNVPathPerm (rcComm_t *conn, fileOpenInp_t *chkNVPathPermInp)
{
    int status;
    status = procApiRequest (conn, CHK_N_V_PATH_PERM_AN, 
      chkNVPathPermInp, NULL, (void **) NULL, NULL);

    return (status);
}
