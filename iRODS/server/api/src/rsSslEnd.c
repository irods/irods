/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See sslEnd.h for a description of this API call.*/

#include "sslEnd.h"

int
rsSslEnd(rsComm_t *rsComm, sslEndInp_t *sslEndInp)
{
#if defined(USE_SSL)

    /* if SSL isn't on, just return success */
    if (!rsComm->ssl_on) {
        return 0;
    }

    /* let the agent service loop know that it needs to shut 
       down SSL after this API call completes */
    rsComm->ssl_do_shutdown = 1;

    return 0;
#else
    return SSL_NOT_BUILT_INTO_SERVER;
#endif
}
    
