/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See sslEnd.h for a description of this API call.*/

#include "sslEnd.h"
#include "rsSslEnd.hpp"

int
rsSslEnd( rsComm_t *rsComm, sslEndInp_t* ) {

    /* if SSL isn't on, just return success */
    if ( !rsComm->ssl_on ) {
        return 0;
    }

    /* let the agent service loop know that it needs to shut
       down SSL after this API call completes */
    rsComm->ssl_do_shutdown = 1;

    return 0;
}
