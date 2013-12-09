/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See getHostForPut.h for a description of this API call.*/

#include "getHostForPut.hpp"
#include "rodsLog.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.hpp"
#include "getRemoteZoneResc.hpp"
#include "dataObjCreate.hpp"
#include "objMetaOpr.hpp"
#include "resource.hpp"
#include "collection.hpp"
#include "specColl.hpp"
#include "miscServerFunct.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"


int rsGetHostForPut(
    rsComm_t*     rsComm,
    dataObjInp_t* dataObjInp,
    char **       outHost ) {
    // =-=-=-=-=-=-=-
    // working on the "home zone", determine if we need to redirect to a different
    // server in this zone for this operation.  if there is a RESC_HIER_STR_KW then
    // we know that the redirection decision has already been made
    std::string       hier;
    if ( getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW ) == NULL ) {
        irods::error ret = irods::resolve_resource_hierarchy( irods::CREATE_OPERATION, rsComm,
                           dataObjInp, hier );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " :: failed in irods::resolve_resource_hierarchy for [";
            msg << dataObjInp->objPath << "]";
            irods::log( PASSMSG( msg.str(), ret ) );
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
    irods::error ret = irods::get_loc_for_hier_string( hier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "rsGetHostForPut - failed in get_loc_for_hier_String", ret ) );
        return -1;
    }

    // =-=-=-=-=-=-=-
    // set the out variable
    *outHost = strdup( location.c_str() );
    return 0;

#if 0
    int status = 0;
    rescGrpInfo_t *myRescGrpInfo;
    rescInfo_t *myRescInfo = 0;
    rodsServerHost_t *rodsServerHost;
    rodsHostAddr_t addr;
    specCollCache_t *specCollCache = NULL;
    char *myHost;
    int remoteFlag = 0; // JMC -backport 4746

    *outHost = NULL;
    if ( getValByKey( &dataObjInp->condInput, ALL_KW ) != NULL ||
            getValByKey( &dataObjInp->condInput, FORCE_FLAG_KW ) != NULL ) {
        /* going to ALL copies or overwriting files. not sure which is the
         * best */
        *outHost = strdup( THIS_ADDRESS );
        return 0;
    }

    resolveLinkedPath( rsComm, dataObjInp->objPath, &specCollCache, NULL );
    if ( isLocalZone( dataObjInp->objPath ) == 0 ) {
        resolveLinkedPath( rsComm, dataObjInp->objPath, &specCollCache,
                           &dataObjInp->condInput );
        remoteFlag = getAndConnRcatHost( rsComm, SLAVE_RCAT,
                                         dataObjInp->objPath, &rodsServerHost );
        if ( remoteFlag < 0 ) {
            return ( remoteFlag );
        }
        else if ( remoteFlag == LOCAL_HOST ) {
            *outHost = strdup( THIS_ADDRESS );
            return 0;
        }
        else {
            status = rcGetHostForPut( rodsServerHost->conn, dataObjInp,
                                      outHost );
            if ( status >= 0 && *outHost != NULL &&
                    strcmp( *outHost, THIS_ADDRESS ) == 0 ) {
                free( *outHost );
                *outHost = strdup( rodsServerHost->hostName->name );
            }
            return ( status );
        }
    }

    status = getSpecCollCache( rsComm, dataObjInp->objPath, 0, &specCollCache );
    if ( status >= 0 && NULL != specCollCache ) { // JMC cppcheck - nullptr
        if ( specCollCache->specColl.collClass == MOUNTED_COLL ) {
            /* JMC - legacy resource - status = resolveResc (specCollCache->specColl.resource, &myRescInfo);
               if (status < 0) {
               rodsLog( LOG_ERROR,"rsGetHostForPut: resolveResc error for %s, status = %d",
               specCollCache->specColl.resource, status);
               return status;
               }*/

            myRescInfo = new rescInfo_t;
            irods::error err = irods::get_resc_info( specCollCache->specColl.resource, *myRescInfo );
            if ( !err.ok() ) {
                std::stringstream msg;
                msg << "rsGetHostForPut - failed for [";
                msg << specCollCache->specColl.resource;
                msg << "]";
                irods::log( PASS( false, -1, msg.str(), err ) );
            }


            /* mounted coll will fall through */
        }
        else {
            *outHost = strdup( THIS_ADDRESS );
            return 0;
        }
    }
    else {
        /* normal type */
        status = getRescGrpForCreate( rsComm, dataObjInp, &myRescGrpInfo );
        if ( status < 0 ) {
            return status;
        }

        myRescInfo = myRescGrpInfo->rescInfo;
        freeAllRescGrpInfo( myRescGrpInfo );
    }

    /* get down here when we got a valid myRescInfo */
    bzero( &addr, sizeof( addr ) );
    //rstrcpy (addr.hostAddr, myRescInfo->rescLoc, NAME_LEN);
    status = resolveHost( &addr, &rodsServerHost );
    delete myRescInfo;
    if ( status < 0 ) {
        return status;
    }
    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
        *outHost = strdup( THIS_ADDRESS );
        return 0;
    }

    myHost = getSvrAddr( rodsServerHost );
    if ( myHost != NULL ) {
        *outHost = strdup( myHost );
        return 0;
    }
    else {
        *outHost = NULL;
        return SYS_INVALID_SERVER_HOST;
    }
#endif
}

