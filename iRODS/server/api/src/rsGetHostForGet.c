/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See getHostForGet.h for a description of this API call.*/

#include "getHostForGet.h"
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
#include "openCollection.h"
#include "readCollection.h"
#include "closeCollection.h"
#include "dataObjOpr.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_resource_backport.h"
#include "eirods_resource_redirect.h"

int rsGetHostForGet(
        rsComm_t*     rsComm, 
        dataObjInp_t* dataObjInp,
        char**        outHost ) {
    // =-=-=-=-=-=-=-
    // working on the "home zone", determine if we need to redirect to a different
    // server in this zone for this operation.  if there is a RESC_HIER_STR_KW then
    // we know that the redirection decision has already been made
    std::string       hier;
    int               local = LOCAL_HOST;
    rodsServerHost_t* host  =  0;
    if( getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW ) == NULL ) {
        eirods::error ret = eirods::resource_redirect( eirods::EIRODS_OPEN_OPERATION, rsComm, 
                                                       dataObjInp, hier, host, local );
        if( !ret.ok() ) { 
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " :: failed in eirods::resource_redirect for [";
            msg << dataObjInp->objPath << "]";
            eirods::log( PASSMSG( msg.str(), ret ) );
            return ret.code();
        }
        // =-=-=-=-=-=-=-
        // we resolved the redirect and have a host, set the hier str for subsequent
        // api calls, etc.
        addKeyVal( &dataObjInp->condInput, RESC_HIER_STR_KW, hier.c_str() );

    } // if keyword

    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    eirods::error ret = eirods::get_loc_for_hier_string( hier, location );
    if( !ret.ok() ) {
        eirods::log( PASSMSG( "rsGetHostForGet - failed in get_loc_for_hier_String", ret ) );
        return -1;
    }

    // =-=-=-=-=-=-=-
    // set the out variable
    *outHost = strdup( location.c_str() );
    return 0;




#if 0
    int status;
    rescInfo_t *myRescInfo;
    char *myResc;
    rodsServerHost_t *rodsServerHost;
    rodsHostAddr_t addr;
    specCollCache_t *specCollCache = NULL;
    char *myHost;
    int remoteFlag = 0; // JMC - backport 4746

    *outHost = NULL;
#if 0 // JMC - backport 4746
    if (isLocalZone (dataObjInp->objPath) == 0) {
	/* it is a remote zone. better connect to this host */
	*outHost = strdup (THIS_ADDRESS);
	return 0;
    }
#endif // JMC - backport 4746

    resolveLinkedPath (rsComm, dataObjInp->objPath, &specCollCache, NULL);
    if (isLocalZone (dataObjInp->objPath) == 0) {
#if 0 // JMC - backport 4746
		/* it is a remote zone. better connect to this host */
		*outHost = strdup (THIS_ADDRESS);
		return 0;
#else // JMC - backport 4746
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
            status = rcGetHostForGet (rodsServerHost->conn, dataObjInp, 
             outHost);
           if (status >= 0 && *outHost != NULL && 
             strcmp (*outHost, THIS_ADDRESS) == 0) {
               free (*outHost);
               *outHost = strdup (rodsServerHost->hostName->name);
           }
            return (status);
       }
#endif     // JMC - backport 4746
	}

    eirods::resource_ptr resc;
    status = getSpecCollCache (rsComm, dataObjInp->objPath, 0, &specCollCache);
    if (status >= 0 && NULL != specCollCache ) { // JMC cppcheck - nullptr
	    if (specCollCache->specColl.collClass == MOUNTED_COLL) {
            //status = resolveResc (specCollCache->specColl.resource, &myRescInfo);
            /*if (status < 0) {
                rodsLog (LOG_ERROR,
                  "rsGetHostForGet: resolveResc error for %s, status = %d",
                 specCollCache->specColl.resource, status);
                return status;
            }*/
            eirods::error err = eirods::get_resc_info( specCollCache->specColl.resource, *myRescInfo );
            if( !err.ok() ) {
                std::stringstream msg;
                msg << "rsGetHostForGet - failed to get resc info for [";
                msg << specCollCache->specColl.resource;
                msg << "]";
                eirods::log( PASS( false, -1, msg.str(), err ) );
            }

            /* mounted coll will fall through with myRescInfo */
        } else {
            *outHost = strdup (THIS_ADDRESS);
            return 0;
        }
    } else if( ( myResc = getValByKey( &dataObjInp->condInput, RESC_NAME_KW ) )  != NULL && 
                eirods::get_resc_info( myResc, *myRescInfo ).ok() ) {
	    /* user specified a resource. myRescInfo set and fall through */
    } else {
	    /* normal type */
        status = getBestRescForGet (rsComm, dataObjInp, &myRescInfo);
        if (myRescInfo == NULL) {
            *outHost = strdup (THIS_ADDRESS);
                return status;
        }
    }

    /* get down here when we got a valid myRescInfo */
#if 0 // JMC - legacy resource
    if (getRescClass (myRescInfo) == COMPOUND_CL) {
        *outHost = strdup (THIS_ADDRESS);
        return 0;
    }
#endif // JMC - legacy resource

    bzero (&addr, sizeof (addr));
    //rstrcpy (addr.hostAddr, myRescInfo->rescLoc, NAME_LEN);
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
#endif
}

int 
getBestRescForGet (rsComm_t *rsComm, dataObjInp_t *dataObjInp,  
rescInfo_t **outRescInfo)
{
    collInp_t collInp;
    hostSearchStat_t hostSearchStat;
    int status, i;
    int maxInx = -1;

    *outRescInfo = NULL;

    if (dataObjInp == NULL || outRescInfo == NULL) return USER__NULL_INPUT_ERR;
    bzero (&hostSearchStat, sizeof (hostSearchStat));
    bzero (&collInp, sizeof (collInp));
    rstrcpy (collInp.collName, dataObjInp->objPath, MAX_NAME_LEN);
    /* assume it is a collection */
    status = getRescForGetInColl (rsComm, &collInp, &hostSearchStat);
    if (status < 0) {
	/* try dataObj */
	status = getRescForGetInDataObj (rsComm, dataObjInp, &hostSearchStat);
    }

    for (i = 0; i < hostSearchStat.numHost; i++) {
	if (maxInx < 0 ||
	   hostSearchStat.count[i] > hostSearchStat.count[maxInx]) {
	    maxInx = i;
	}
    }
    if (maxInx >= 0) *outRescInfo = hostSearchStat.rescInfo[maxInx];

    return status;
}

int
getRescForGetInColl (rsComm_t *rsComm, collInp_t *collInp,
hostSearchStat_t *hostSearchStat)
{
    collEnt_t *collEnt;
    int handleInx;
    int status;

    if (collInp == NULL || hostSearchStat == NULL) return USER__NULL_INPUT_ERR;

    handleInx = rsOpenCollection (rsComm, collInp);
    if (handleInx < 0) return handleInx;

    while ((status = rsReadCollection (rsComm, &handleInx, &collEnt)) >= 0) {
        if (collEnt->objType == DATA_OBJ_T) {
	    dataObjInp_t dataObjInp;
	    bzero (&dataObjInp, sizeof (dataObjInp));
	    snprintf (dataObjInp.objPath, MAX_NAME_LEN, "%s/%s", 
	      collEnt->collName, collEnt->dataName);
	    status = getRescForGetInDataObj (rsComm, &dataObjInp, 
	      hostSearchStat);
	    if (status < 0) {
                rodsLog (LOG_NOTICE,
                 "getRescForGetInColl: getRescForGetInDataObj %s err, stat=%d",
                 dataObjInp.objPath, status);
	    }
	} else if (collEnt->objType == COLL_OBJ_T) {
	    collInp_t myCollInp;
	    bzero (&myCollInp, sizeof (myCollInp));
            rstrcpy (myCollInp.collName, collEnt->collName, MAX_NAME_LEN);
	    status = getRescForGetInColl (rsComm, &myCollInp, hostSearchStat);
            if (status < 0) {
                rodsLog (LOG_NOTICE,
                 "getRescForGetInColl: getRescForGetInColl %s err, stat=%d",
                 collEnt->collName, status);
            }
	}
	free (collEnt);     /* just free collEnt but not content */
	if (hostSearchStat->totalCount >= MAX_HOST_TO_SEARCH) {
	    /* done */
	    rsCloseCollection (rsComm, &handleInx);
            return 0;
        }
    }
    rsCloseCollection (rsComm, &handleInx);
    return 0;
}

int
getRescForGetInDataObj (rsComm_t *rsComm, dataObjInp_t *dataObjInp, 
hostSearchStat_t *hostSearchStat)
{
    int status, i;
    dataObjInfo_t *dataObjInfoHead = NULL;

    if (dataObjInp == NULL || hostSearchStat == NULL) 
      return USER__NULL_INPUT_ERR;

    status = getDataObjInfoIncSpecColl (rsComm, dataObjInp, &dataObjInfoHead);
    if (status < 0) return status;

    sortObjInfoForOpen (rsComm, &dataObjInfoHead, &dataObjInp->condInput, 0);
    if (dataObjInfoHead != NULL && dataObjInfoHead->rescInfo != NULL) {
	if (hostSearchStat->numHost >= MAX_HOST_TO_SEARCH ||
	  hostSearchStat->totalCount >= MAX_HOST_TO_SEARCH) {
	    freeAllDataObjInfo (dataObjInfoHead);
            return 0;
	}
	for (i = 0; i < hostSearchStat->numHost; i++) {
	    if (dataObjInfoHead->rescInfo == hostSearchStat->rescInfo[i]) {
		hostSearchStat->count[i]++;
		hostSearchStat->totalCount++;
                freeAllDataObjInfo (dataObjInfoHead);
                return 0;
	    }
	}
	/* no match. add one */
	hostSearchStat->rescInfo[hostSearchStat->numHost] =
         dataObjInfoHead->rescInfo;
	hostSearchStat->count[hostSearchStat->numHost] = 1;
	hostSearchStat->numHost++;
	hostSearchStat->totalCount++;
    }
    freeAllDataObjInfo (dataObjInfoHead);
    return 0;
}
