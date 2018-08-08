/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See collCreate.h for a description of this API call.*/

#include "rcMisc.h"
#include "collCreate.h"
#include "rodsConnect.h"
#include "rodsLog.h"
#include "regColl.h"
#include "icatDefines.h"
#include "fileMkdir.h"
#include "subStructFileMkdir.h"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "objMetaOpr.hpp"
#include "collection.hpp"
#include "specColl.hpp"
#include "physPath.hpp"
#include "dataObjOpr.hpp"
#include "miscServerFunct.hpp"
#include "rsCollCreate.hpp"
#include "rsRegColl.hpp"
#include "rsSubStructFileMkdir.hpp"
#include "rsFileMkdir.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_configuration_keywords.hpp"

int
rsCollCreate( rsComm_t *rsComm, collInp_t *collCreateInp ) {
    int status;
    rodsServerHost_t *rodsServerHost = NULL;
    ruleExecInfo_t rei;
    collInfo_t collInfo;
    specCollCache_t *specCollCache = NULL;
    dataObjInfo_t *dataObjInfo = NULL;

    irods::error ret = validate_logical_path( collCreateInp->collName );
    if ( !ret.ok() ) {
        if ( rsComm->rError.len < MAX_ERROR_MESSAGES ) {
            char error_msg[ERR_MSG_LEN];
            snprintf(error_msg, ERR_MSG_LEN, "%s", ret.user_result().c_str());
            addRErrorMsg( &rsComm->rError, ret.code(), error_msg );
        }
        irods::log( ret );
        return SYS_INVALID_INPUT_PARAM;
    }

    // Issue 3913: retain status in case string too long
    status = resolveLinkedPath( rsComm, collCreateInp->collName, &specCollCache,
                       &collCreateInp->condInput );

    // Issue 3913: retain status in case string too long
    if (status == USER_STRLEN_TOOLONG) {
        return USER_STRLEN_TOOLONG;
    }
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
        std::string svc_role;
        irods::error ret = get_catalog_service_role(svc_role);
        if(!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }

        if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
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
        } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
            status = SYS_NO_RCAT_SERVER_ERR;
        } else {
            rodsLog(
                LOG_ERROR,
                "role not supported [%s]",
                svc_role.c_str() );
            status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
        }
    }
    else {
        status = rcCollCreate( rodsServerHost->conn, collCreateInp );
    }
    return status;
}

int
l3Mkdir( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo ) {
    if ( NULL == dataObjInfo ) {
        return SYS_NULL_INPUT;
    }

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
