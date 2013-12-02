/* This is script-generated code.  */ 
/* See structFileExtract.h for a description of this API call.*/

#include "structFileExtract.h"

int
rcStructFileExtract (rcComm_t *conn, structFileOprInp_t *structFileOprInp)
{
    int status;
    status = procApiRequest (conn, STRUCT_FILE_EXTRACT_AN,  
      structFileOprInp, NULL, (void **) NULL, NULL);

    return (status);
}
