/* This is script-generated code.  */ 
/* See structFileBundle.h for a description of this API call.*/

#include "structFileBundle.h"

int
rcStructFileBundle (rcComm_t *conn, 
structFileExtAndRegInp_t *structFileBundleInp)
{
    int status;
    status = procApiRequest (conn, STRUCT_FILE_BUNDLE_AN, structFileBundleInp, 
      NULL, (void **) NULL, NULL);

    return (status);
}
