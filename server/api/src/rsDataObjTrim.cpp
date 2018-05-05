/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* See dataObjTrim.h for a description of this API call.*/

#include "dataObjTrim.h"
#include "dataObjUnlink.h"
#include "dataObjOpr.hpp"
#include "rodsLog.h"
#include "objMetaOpr.hpp"
#include "specColl.hpp"
#include "rsDataObjTrim.hpp"
#include "icatDefines.h"
#include "getRemoteZoneResc.h"
#include "rsDataObjUnlink.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_redirect.hpp"
#include "irods_hierarchy_parser.hpp"

/* rsDataObjTrim - The Api handler of the rcDataObjTrim call - trim down
 * the number of replicas of a data object
 * Input -
 *    rsComm_t *rsComm
 *    dataObjInp_t *dataObjInp - The trim input
 *
 *  Returned val - return 1 if a copy is trimmed. 0 if nothing trimmed.
 */

int
rsDataObjTrim( rsComm_t *rsComm, dataObjInp_t *dataObjInp ) {
    if (getValByKey(&dataObjInp->condInput, RESC_NAME_KW) && // -S
        getValByKey(&dataObjInp->condInput, REPL_NUM_KW))    // -n
    {
        return USER_INCOMPATIBLE_PARAMS;
    }

    int status;
    dataObjInfo_t *dataObjInfoHead = NULL;
    dataObjInfo_t *tmpDataObjInfo;
    char *accessPerm;
    int retVal = 0;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    specCollCache_t *specCollCache = NULL;
    int myTime = 0;
    char *tmpStr;
    int myAge;

    resolveLinkedPath( rsComm, dataObjInp->objPath, &specCollCache,
                       &dataObjInp->condInput );
    remoteFlag = getAndConnRemoteZone( rsComm, dataObjInp, &rodsServerHost,
                                       REMOTE_OPEN );

    if ( remoteFlag < 0 ) {
        return remoteFlag;
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = rcDataObjTrim( rodsServerHost->conn, dataObjInp );
        return status;
    }

    // =-=-=-=-=-=-=-
    // working on the "home zone", determine if we need to redirect to a different
    // server in this zone for this operation.  if there is a RESC_HIER_STR_KW then
    // we know that the redirection decision has already been made
    std::string       hier;
    int               local = LOCAL_HOST;
    rodsServerHost_t* host  =  0;
    char* hier_char = getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW );
    if ( hier_char == NULL ) {
        // set a repl keyword here so resources can respond accordingly
        addKeyVal( &dataObjInp->condInput, IN_REPL_KW, "" );
        irods::error ret = irods::resource_redirect( irods::UNLINK_OPERATION, rsComm,
                           dataObjInp, hier, host, local );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " :: failed in irods::resource_redirect for [";
            msg << dataObjInp->objPath << "]";
            irods::log( PASSMSG( msg.str(), ret ) );
            return ret.code();
        }
        // =-=-=-=-=-=-=-
        // we resolved the redirect and have a host, set the hier str for subsequent
        // api calls, etc.
        addKeyVal( &dataObjInp->condInput, RESC_HIER_STR_KW, hier.c_str() );

    } // if keyword

    if ( getValByKey( &dataObjInp->condInput, ADMIN_KW ) != NULL ) {
        if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
        }
        accessPerm = NULL;
    }
    else {
        accessPerm = ACCESS_DELETE_OBJECT;
    }

    status = getDataObjInfo( rsComm, dataObjInp, &dataObjInfoHead,
                             accessPerm, 1 );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "rsDataObjTrim: getDataObjInfo for %s", dataObjInp->objPath );
        return status;
    }
    status = resolveInfoForTrim( &dataObjInfoHead, &dataObjInp->condInput );

    if ( status < 0 ) {
        return status;
    }

    if ( ( tmpStr = getValByKey( &dataObjInp->condInput, AGE_KW ) ) != NULL ) {
        myAge = atoi( tmpStr );
        /* age value is in minutes */
        if ( myAge > 0 ) {
            myTime = time( 0 ) - myAge * 60;
        }
    }

    tmpDataObjInfo = dataObjInfoHead;
    while ( tmpDataObjInfo != NULL ) {
        if ( myTime == 0 || atoi( tmpDataObjInfo->dataModify ) <= myTime ) {
            if ( getValByKey( &dataObjInp->condInput, DRYRUN_KW ) == NULL ) {
                status = dataObjUnlinkS( rsComm, dataObjInp, tmpDataObjInfo );
                if ( status < 0 ) {
                    if ( retVal == 0 ) {
                        retVal = status;
                    }
                }
                else {
                    retVal = 1;
                }
            }
            else {
                retVal = 1;
            }
        }
        tmpDataObjInfo = tmpDataObjInfo->next;
    }

    freeAllDataObjInfo( dataObjInfoHead );

    return retVal;
}


int trimDataObjInfo(
    rsComm_t*      rsComm,
    dataObjInfo_t* dataObjInfo ) {

    dataObjInp_t dataObjInp;
    char tmpStr[NAME_LEN];
    int status;

    // =-=-=-=-=-=-=-
    // add the hier to a parser to get the leaf
    irods::hierarchy_parser parser;
    parser.set_string( dataObjInfo->rescHier );
    std::string cache_resc;
    parser.last_resc( cache_resc );

    bzero( &dataObjInp, sizeof( dataObjInp ) );
    rstrcpy( dataObjInp.objPath,  dataObjInfo->objPath, MAX_NAME_LEN );
    snprintf( tmpStr, NAME_LEN, "1" );
    addKeyVal( &dataObjInp.condInput, COPIES_KW, tmpStr );

    // =-=-=-=-=-=-=-
    // specify the cache repl num to trim just the cache
    std::stringstream str;
    str << dataObjInfo->replNum;
    addKeyVal( &dataObjInp.condInput, REPL_NUM_KW, str.str().c_str() );
    addKeyVal( &dataObjInp.condInput, RESC_HIER_STR_KW, dataObjInfo->rescHier );

    status = rsDataObjTrim( rsComm, &dataObjInp );
    clearKeyVal( &dataObjInp.condInput );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "trimDataObjInfo: rsDataObjTrim of %s error", dataObjInfo->objPath );
    }
    return status;
}
