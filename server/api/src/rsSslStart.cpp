/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See sslStart.h for a description of this API call.*/

#include "sslStart.h"
#include "rsSslStart.hpp"

int
rsSslStart( rsComm_t *rsComm, sslStartInp_t* ) {
    /* if SSL is on already, just return success */
    if ( rsComm->ssl_on ) {
        return 0;
    }

    /* Let the agent service loop know that it needs to
       setup SSL before the next API call */
    rsComm->ssl_do_accept = 1;

    return 0;
}
