/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See getHostForPut.h for a description of this API call.*/

#include "getHostForPut.h"
#include "rodsLog.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "getRemoteZoneResc.h"
#include "dataObjCreate.h"
#include "objMetaOpr.h"
#include "resource.h"
#include "collection.h"
#include "specColl.h"
#include "miscServerFunct.h"

// =-=-=-=-=-=-=-
// eirods resource includes
#include "eirods_resource_backport.h"


int
rsGetHostForPut (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
char **outHost)
{
    int status;
    rescGrpInfo_t *myRescGrpInfo;
    rescInfo_t *myRescInfo = new rescInfo_t;
    rodsServerHost_t *rodsServerHost;
    rodsHostAddr_t addr;
    specCollCache_t *specCollCache = NULL;
    char *myHost;
    int remoteFlag = 0; // JMC -backport 4746

    *outHost = NULL;
#if 0 // JMC -backport 4746
    if (isLocalZone (dataObjInp->objPath) == 0) {
	/* it is a remote zone. better connect to this host */
	*outHost = strdup (THIS_ADDRESS);
	return 0;
    }
#endif
    if (getValByKey (&dataObjInp->condInput, ALL_KW) != NULL ||
      getValByKey (&dataObjInp->condInput, FORCE_FLAG_KW) != NULL) {
	/* going to ALL copies or overwriting files. not sure which is the 
         * best */ 
        *outHost = strdup (THIS_ADDRESS);
        return 0;
    }

    resolveLinkedPath (rsComm, dataObjInp->objPath, &specCollCache, NULL);
    if (isLocalZone (dataObjInp->objPath) == 0) {
#if 0 // JMC -backport 4746
        /* it is a remote zone. better connect to this host */
        *outHost = strdup (THIS_ADDRESS);
        return 0;
#else // JMC -backport 4746
        resolveLinkedPath (rsComm, dataObjInp->objPath, &specCollCache,
          &dataObjInp->condInput);
        remoteFlag = getAndConnRcatHost (rsComm, SLAVE_RCAT,
          dataObjInp->objPath, &rodsServerHost);
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else if (remoteFlag == LOCAL_HOST) {
            *outHost = strdup (THIS_ADDRESS);
            return 0;
        } else {
            status = rcGetHostForPut (rodsServerHost->conn, dataObjInp,
              outHost);
            if (status >= 0 && *outHost != NULL &&
              strcmp (*outHost, THIS_ADDRESS) == 0) {
                free (*outHost);
                *outHost = strdup (rodsServerHost->hostName->name);
            }
            return (status);
        }
#endif // JMC -backport 4746
    }

    status = getSpecCollCache (rsComm, dataObjInp->objPath, 0, &specCollCache);
    if (status >= 0 && NULL != specCollCache) { // JMC cppcheck - nullptr
        if (specCollCache->specColl.collClass == MOUNTED_COLL) {
            /* JMC - legacy resource - status = resolveResc (specCollCache->specColl.resource, &myRescInfo);
            if (status < 0) {
                rodsLog( LOG_ERROR,"rsGetHostForPut: resolveResc error for %s, status = %d",
                         specCollCache->specColl.resource, status);
                return status;
            }*/

            eirods::error err = eirods::get_resc_info( specCollCache->specColl.resource, *myRescInfo );
            if( !err.ok() ) {
                std::stringstream msg;
                msg << "rsGetHostForPut - failed for [";
                msg << specCollCache->specColl.resource;
                msg << "]";
                eirods::log( PASS( false, -1, msg.str(), err ) );
            }


            /* mounted coll will fall through */
        } else {
            *outHost = strdup (THIS_ADDRESS);
            return 0;
        }
    } else {
	/* normal type */
        status = getRescGrpForCreate (rsComm, dataObjInp, &myRescGrpInfo);
        if (status < 0) 
            return status;

        myRescInfo = myRescGrpInfo->rescInfo;
	    freeAllRescGrpInfo (myRescGrpInfo);
#if 0 // JMC - legacy resource
        /* status == 1 means random sorting scheme */
        if( ( status == 1 && getRescCnt (myRescGrpInfo) > 1 ) || 
            getRescClass (myRescInfo) == COMPOUND_CL) {
            *outHost = strdup (THIS_ADDRESS);
	        return 0;
	    }
#endif // JMC - legacy resource
    }
    /* get down here when we got a valid myRescInfo */
    bzero (&addr, sizeof (addr));
    rstrcpy (addr.hostAddr, myRescInfo->rescLoc, NAME_LEN);
    status = resolveHost (&addr, &rodsServerHost);
    if (status < 0) return status;
    if (rodsServerHost->localFlag == LOCAL_HOST) {
        *outHost = strdup (THIS_ADDRESS);
        return 0;
    }

    myHost = getSvrAddr (rodsServerHost);
    if (myHost != NULL) {
	*outHost = strdup (myHost);
        return 0;
    } else {
        *outHost = NULL;
	return SYS_INVALID_SERVER_HOST;
    }
}

