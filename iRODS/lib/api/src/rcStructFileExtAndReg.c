/* This is script-generated code.  */ 
/* See structFileExtAndReg.h for a description of this API call.*/

#include "apiHeaderAll.h"

int
rcStructFileExtAndReg (rcComm_t *conn, 
structFileExtAndRegInp_t *structFileExtAndRegInp)
{
    int status;
    status = procApiRequest (conn, STRUCT_FILE_EXT_AND_REG_AN, 
     structFileExtAndRegInp, NULL, (void **) NULL, NULL);

    return (status);
}
