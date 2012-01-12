/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See unbunAndRegPhyBunfle.h for a description of this API call.*/

#include "unbunAndRegPhyBunfile.h"

int
rcUnbunAndRegPhyBunfile (rcComm_t *conn, dataObjInp_t *dataObjInp)
{
    int status;
    status = procApiRequest (conn, UNBUN_AND_REG_PHY_BUNFILE_AN,  dataObjInp, 
      NULL, (void **) NULL, NULL);

    return (status);
}

