/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See collCreate.h for a description of this API call.*/

#include "reFuncDefs.hpp"
#include "collCreate.hpp"
#include "rodsConnect.h"
#include "rodsLog.hpp"
#include "regColl.hpp"
#include "icatDefines.hpp"
#include "fileMkdir.hpp"
#include "subStructFileMkdir.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.hpp"
#include "objMetaOpr.hpp"
#include "collection.hpp"
#include "specColl.hpp"
#include "physPath.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_hierarchy_parser.hpp"


// =-=-=-=-=-=-=-
/// @brief function which determines if a collection is created at the root level
irods::error validate_collection_path(
    const std::string& _path ) {
    // =-=-=-=-=-=-=-
    // set up a default error structure
    std::stringstream msg;
    msg << "a valid zone name does not appear at the root of the collection path [";
    msg << _path;
    msg << "]";
    irods::error ret_val = ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );

    // =-=-=-=-=-=-=-
    // loop over the ZoneInfo linked list and see if the path
    // has a root collection which matches any zone
    zoneInfo_t* zone_info = ZoneInfoHead;
    while ( zone_info ) {
        // =-=-=-=-=-=-=-
        // build a root zone name
        std::string zone_name( "/" );
        zone_name += zone_info->zoneName;

        // =-=-=-=-=-=-=-
        // if the zone name appears at the root
        // then this is a good path
        size_t pos = _path.find( zone_name );
        if ( 0 == pos ) {
            ret_val = SUCCESS();
            zone_info = 0;
        }
        else {
            zone_info = zone_info->next;

        }

    } // while

    return ret_val;

} // validate_collection_path


int
rsCollCreate( rsComm_t *rsComm, collInp_t *collCreateInp ) {
    int status;
    rodsServerHost_t *rodsServerHost = NULL;
    ruleExecInfo_t rei;
    collInfo_t collInfo;
    specCollCache_t *specCollCache = NULL;
#ifdef RODS_CAT
    dataObjInfo_t *dataObjInfo = NULL;
#endif

    irods::error ret = validate_collection_path( collCreateInp->collName );
    if ( !ret.ok() ) {
        irods::log( ret );
        return SYS_INVALID_INPUT_PARAM;
    }

    resolveLinkedPath( rsComm, collCreateInp->collName, &specCollCache,
                       &collCreateInp->condInput );
    status = getAndConnRcatHost(
                 rsComm,
                 MASTER_RCAT,
                 ( const char* )collCreateInp->collName,
                 &rodsServerHost );
    if ( status < 0 || rodsServerHost == NULL ) { // JMC cppcheck
        return status;
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
        initReiWithCollInp( &rei, rsComm, collCreateInp, &collInfo );

        status = applyRule( "acPreprocForCollCreate", NULL, &rei, NO_SAVE_REI );

        if ( status < 0 ) {
            if ( rei.status < 0 ) {
                status = rei.status;
            }
            rodsLog( LOG_ERROR,
                     "rsCollCreate:acPreprocForCollCreate error for %s,stat=%d",
                     collCreateInp->collName, status );
            return status;
        }

        if ( getValByKey( &collCreateInp->condInput, RECURSIVE_OPR__KW ) !=
                NULL ) {
            status = rsMkCollR( rsComm, "/", collCreateInp->collName );
            return status;
        }
#ifdef RODS_CAT

        /* for STRUCT_FILE_COLL to make a directory in the structFile, the
         * COLLECTION_TYPE_KW must be set */

        status = resolvePathInSpecColl( rsComm, collCreateInp->collName,
                                        WRITE_COLL_PERM, 0, &dataObjInfo );
        if ( status >= 0 ) {
            freeDataObjInfo( dataObjInfo );
            if ( status == COLL_OBJ_T ) {
                return 0;
            }
            else if ( status == DATA_OBJ_T ) {
                return USER_INPUT_PATH_ERR;
            }
        }
        else if ( status == SYS_SPEC_COLL_OBJ_NOT_EXIST ) {
            /* for STRUCT_FILE_COLL to make a directory in the structFile, the
             * COLLECTION_TYPE_KW must be set */
            if ( dataObjInfo != NULL && dataObjInfo->specColl != NULL &&
                    dataObjInfo->specColl->collClass == LINKED_COLL ) {
                /*  should not be here because if has been translated */
                return SYS_COLL_LINK_PATH_ERR;
            }
            else {
                status = l3Mkdir( rsComm, dataObjInfo );
            }
            freeDataObjInfo( dataObjInfo );
            return status;
        }
        else {
            if ( isColl( rsComm, collCreateInp->collName, NULL ) >= 0 ) {
                return CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME;
            }
            status = _rsRegColl( rsComm, collCreateInp );
        }
        rei.status = status;
        if ( status >= 0 ) {
            rei.status = applyRule( "acPostProcForCollCreate", NULL, &rei,
                                    NO_SAVE_REI );

            if ( rei.status < 0 ) {
                rodsLog( LOG_ERROR,
                         "rsCollCreate:acPostProcForCollCreate error for %s,stat=%d",
                         collCreateInp->collName, status );
            }
        }

#else
        status = SYS_NO_RCAT_SERVER_ERR;
#endif
    }
    else {
        status = rcCollCreate( rodsServerHost->conn, collCreateInp );
    }

    return status;
}

int
l3Mkdir( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo ) {
    //int rescTypeInx;
    fileMkdirInp_t fileMkdirInp;
    int status;

    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "l3Mkdir - failed in get_loc_for_hier_string", ret ) );
        return -1;
    }

    if ( getStructFileType( dataObjInfo->specColl ) >= 0 ) {
        subFile_t subFile;
        memset( &subFile, 0, sizeof( subFile ) );
        rstrcpy( subFile.subFilePath, dataObjInfo->subPath, MAX_NAME_LEN );
        subFile.mode = getDefDirMode();
        //rstrcpy (subFile.addr.hostAddr, dataObjInfo->rescInfo->rescLoc, NAME_LEN );
        rstrcpy( subFile.addr.hostAddr, location.c_str(), NAME_LEN );
        subFile.specColl = dataObjInfo->specColl;
        status = rsSubStructFileMkdir( rsComm, &subFile );
    }
    else {
        memset( &fileMkdirInp, 0, sizeof( fileMkdirInp ) );
        rstrcpy( fileMkdirInp.dirName, dataObjInfo->filePath, MAX_NAME_LEN );
        rstrcpy( fileMkdirInp.rescHier, dataObjInfo->rescHier, MAX_NAME_LEN );
        rstrcpy( fileMkdirInp.addr.hostAddr, location.c_str(), NAME_LEN );
        fileMkdirInp.mode = getDefDirMode();
        status = rsFileMkdir( rsComm, &fileMkdirInp );
    }
    return status;
}
