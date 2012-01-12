/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsGetMiscSvrInfo.c - server routine that handles the the GetMiscSvrInfo
 * API
 */

/* script generated code */
#include "getMiscSvrInfo.h"

int
rsGetMiscSvrInfo (rsComm_t *rsComm, miscSvrInfo_t **outSvrInfo)
{
    miscSvrInfo_t *myOutSvrInfo;
    char *tmpStr;

    myOutSvrInfo = *outSvrInfo = (miscSvrInfo_t*)malloc (sizeof (miscSvrInfo_t));

    memset (myOutSvrInfo, 0, sizeof (miscSvrInfo_t));

/* user writtened code */
#ifdef RODS_CAT
    myOutSvrInfo->serverType = RCAT_ENABLED;
#else
    myOutSvrInfo->serverType = RCAT_NOT_ENABLED;
#endif
    rstrcpy (myOutSvrInfo->relVersion, RODS_REL_VERSION, NAME_LEN);
    rstrcpy (myOutSvrInfo->apiVersion, RODS_API_VERSION, NAME_LEN);

    rstrcpy (myOutSvrInfo->rodsZone, rsComm->myEnv.rodsZone, NAME_LEN);
    if ((tmpStr = getenv (SERVER_BOOT_TIME)) != NULL) 
	myOutSvrInfo->serverBootTime = atoi (tmpStr);

    return (0);
}

