/* This is script-generated code.  */ 
/* See phyBundleColl.h for a description of this API call.*/

#include "phyBundleColl.h"

int
rcPhyBundleColl (rcComm_t *conn, 
structFileExtAndRegInp_t *phyBundleCollInp)
{
    int status;
    status = procApiRequest (conn, PHY_BUNDLE_COLL_AN, phyBundleCollInp, NULL, 
        (void **) NULL, NULL);

    return (status);
}
