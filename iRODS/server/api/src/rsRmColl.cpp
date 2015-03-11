/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsRmColl.c
 */

#include "rmColl.hpp"
#include "reFuncDefs.hpp"
#include "objMetaOpr.hpp"
#include "specColl.hpp"
#include "icatHighLevelRoutines.hpp"
#include "openCollection.hpp"
#include "readCollection.hpp"
#include "closeCollection.hpp"
#include "dataObjUnlink.hpp"
#include "rsApiHandler.hpp"
#include "fileRmdir.hpp"
#include "collection.hpp"
#include "subStructFileRmdir.hpp"
#include "dataObjRename.hpp"

#include "irods_resource_backport.hpp"

int
rsRmColl( rsComm_t *rsComm, collInp_t *rmCollInp,
          collOprStat_t **collOprStat ) {
    int status;
    ruleExecInfo_t rei;
    collInfo_t collInfo;
    rodsServerHost_t *rodsServerHost = NULL;
    specCollCache_t *specCollCache = NULL;

    resolveLinkedPath( rsComm, rmCollInp->collName, &specCollCache,
                       &rmCollInp->condInput );
    status = getAndConnRcatHost(
                 rsComm,
                 MASTER_RCAT,
                 ( const char* )rmCollInp->collName,
                 &rodsServerHost );

    if ( status < 0 || NULL == rodsServerHost )  { // JMC cppcheck - nullptr
        return status;
    }
    else if ( rodsServerHost->rcatEnabled == REMOTE_ICAT ) {
        int retval;
        retval = _rcRmColl( rodsServerHost->conn, rmCollInp, collOprStat );
        status = svrSendZoneCollOprStat( rsComm, rodsServerHost->conn,
                                         *collOprStat, retval );
        return status;
    }

    initReiWithCollInp( &rei, rsComm, rmCollInp, &collInfo );

    status = applyRule( "acPreprocForRmColl", NULL, &rei, NO_SAVE_REI );

    if ( status < 0 ) {
        if ( rei.status < 0 ) {
            status = rei.status;
        }
        rodsLog( LOG_ERROR,
                 "rsRmColl:acPreprocForRmColl error for %s,stat=%d",
                 rmCollInp->collName, status );
        return status;
    }

    if ( collOprStat != NULL ) {
        *collOprStat = NULL;
    }
    if ( getValByKey( &rmCollInp->condInput, RECURSIVE_OPR__KW ) == NULL ) {
        status = _rsRmColl( rsComm, rmCollInp, collOprStat );
    }
    else {
        if ( isTrashPath( rmCollInp->collName ) == False &&
                getValByKey( &rmCollInp->condInput, FORCE_FLAG_KW ) != NULL ) {
            rodsLog( LOG_NOTICE,
                     "rsRmColl: Recursively removing %s.",
                     rmCollInp->collName );
        }
        status = _rsRmCollRecur( rsComm, rmCollInp, collOprStat );
    }
    rei.status = status;
    rei.status = applyRule( "acPostProcForRmColl", NULL, &rei,
                            NO_SAVE_REI );

    if ( rei.status < 0 ) {
        rodsLog( LOG_ERROR,
                 "rsRmColl:acPostProcForRmColl error for %s,stat=%d",
                 rmCollInp->collName, status );
    }

    return status;
}

int
_rsRmColl( rsComm_t *rsComm, collInp_t *rmCollInp,
           collOprStat_t **collOprStat ) {
    int status;
    dataObjInfo_t *dataObjInfo = NULL;

    if ( getValByKey( &rmCollInp->condInput, UNREG_COLL_KW ) != NULL ) {
        status = svrUnregColl( rsComm, rmCollInp );
    }
    else {
        /* need to check if it is spec coll */
        status = resolvePathInSpecColl( rsComm, rmCollInp->collName,
                                        WRITE_COLL_PERM, 0, &dataObjInfo );
        if ( status < 0 && status != CAT_NO_ROWS_FOUND ) {
            return status;
        }
        else if ( status == COLL_OBJ_T && dataObjInfo->specColl != NULL ) {
            if ( dataObjInfo->specColl->collClass == LINKED_COLL ) {
                rstrcpy( rmCollInp->collName, dataObjInfo->objPath,
                         MAX_NAME_LEN );
                status = svrUnregColl( rsComm, rmCollInp );
            }
            else {
                status = l3Rmdir( rsComm, dataObjInfo );
            }
            freeDataObjInfo( dataObjInfo );
        }
        else {
            status = svrUnregColl( rsComm, rmCollInp );
        }
    }
    if ( status >= 0 && collOprStat != NULL ) {
        *collOprStat = ( collOprStat_t* )malloc( sizeof( collOprStat_t ) );
        memset( *collOprStat, 0, sizeof( collOprStat_t ) );
        ( *collOprStat )->filesCnt = 1;
        ( *collOprStat )->totalFileCnt = 1;
        rstrcpy( ( *collOprStat )->lastObjPath, rmCollInp->collName,
                 MAX_NAME_LEN );
    }
    return status;
}

int
_rsRmCollRecur( rsComm_t *rsComm, collInp_t *rmCollInp,
                collOprStat_t **collOprStat ) {
    int status;
    ruleExecInfo_t rei;
    int trashPolicy;
    dataObjInfo_t *dataObjInfo = NULL;
    /* check for specColl and permission */
    status = resolvePathInSpecColl( rsComm, rmCollInp->collName,
                                    WRITE_COLL_PERM, 0, &dataObjInfo );

    if ( status < 0 && status != CAT_NO_ROWS_FOUND ) {
        return status;
    }
    if ( status == COLL_OBJ_T && dataObjInfo->specColl != NULL &&
            dataObjInfo->specColl->collClass == LINKED_COLL ) {
        rstrcpy( rmCollInp->collName, dataObjInfo->objPath,
                 MAX_NAME_LEN );
        free( dataObjInfo->specColl );
        dataObjInfo->specColl = NULL;
    }
    if ( status != COLL_OBJ_T || dataObjInfo->specColl == NULL ) {
        /* a normal coll */
        if ( rmCollInp->oprType != UNREG_OPR &&
                getValByKey( &rmCollInp->condInput, FORCE_FLAG_KW ) == NULL &&
                getValByKey( &rmCollInp->condInput, RMTRASH_KW ) == NULL &&
                getValByKey( &rmCollInp->condInput, ADMIN_RMTRASH_KW ) == NULL ) {
            initReiWithDataObjInp( &rei, rsComm, NULL );
            status = applyRule( "acTrashPolicy", NULL, &rei, NO_SAVE_REI );
            trashPolicy = rei.status;
            if ( trashPolicy != NO_TRASH_CAN ) {
                status = rsMvCollToTrash( rsComm, rmCollInp );
                if ( status >= 0 && collOprStat != NULL ) {
                    if ( *collOprStat == NULL ) {
                        *collOprStat = ( collOprStat_t* )malloc( sizeof( collOprStat_t ) );
                        memset( *collOprStat, 0, sizeof( collOprStat_t ) );
                    }
                    ( *collOprStat )->filesCnt = 1;
                    ( *collOprStat )->totalFileCnt = 1;
                    rstrcpy( ( *collOprStat )->lastObjPath, rmCollInp->collName,
                             MAX_NAME_LEN );
                }
                return status;
            }
        }
    }
    /* got here. will recursively phy delete the collection */
    status = _rsPhyRmColl( rsComm, rmCollInp, dataObjInfo, collOprStat );

    if ( dataObjInfo != NULL ) {
        freeDataObjInfo( dataObjInfo );
    }
    return status;
}

int
_rsPhyRmColl( rsComm_t *rsComm, collInp_t *rmCollInp,
              dataObjInfo_t *dataObjInfo, collOprStat_t **collOprStat ) {

    char *tmpValue;
    int status;
    collInp_t openCollInp;
    int handleInx;
    dataObjInp_t dataObjInp;
    collInp_t tmpCollInp;
    int rmtrashFlag = 0;
    int savedStatus = 0;
    int fileCntPerStatOut = FILE_CNT_PER_STAT_OUT;
    int entCnt = 0;
    ruleExecInfo_t rei;
    collInfo_t collInfo;

    memset( &openCollInp, 0, sizeof( openCollInp ) );
    rstrcpy( openCollInp.collName, rmCollInp->collName, MAX_NAME_LEN );
    /* cannot query recur because collection is sorted in wrong order */
    openCollInp.flags = 0;
    handleInx = rsOpenCollection( rsComm, &openCollInp );
    if ( handleInx < 0 ) {
        rodsLog( LOG_ERROR,
                 "_rsPhyRmColl: rsOpenCollection of %s error. status = %d",
                 openCollInp.collName, handleInx );
        return handleInx;
    }

    memset( &dataObjInp, 0, sizeof( dataObjInp ) );
    memset( &tmpCollInp, 0, sizeof( tmpCollInp ) );
    /* catch the UNREG_OPR */
    dataObjInp.oprType = tmpCollInp.oprType = rmCollInp->oprType;

    if ( ( tmpValue = getValByKey( &rmCollInp->condInput, AGE_KW ) ) != NULL ) {
        if ( CollHandle[handleInx].rodsObjStat != NULL ) {
            /* when a collection is moved, the modfiyTime of the object in
              * the collectin does not change. So, we'll depend on the
              * modfiyTime of the collection */
            int ageLimit = atoi( tmpValue ) * 60;
            int modifyTime =
                atoi( CollHandle[handleInx].rodsObjStat->modifyTime );
            if ( ( time( 0 ) - modifyTime ) < ageLimit ) {
                rsCloseCollection( rsComm, &handleInx );
                return 0;
            }
        }
        addKeyVal( &dataObjInp.condInput, AGE_KW, tmpValue );
        addKeyVal( &tmpCollInp.condInput, AGE_KW, tmpValue );
    }
    addKeyVal( &dataObjInp.condInput, FORCE_FLAG_KW, "" );
    addKeyVal( &tmpCollInp.condInput, FORCE_FLAG_KW, "" );

    if ( ( tmpValue = getValByKey( &rmCollInp->condInput, AGE_KW ) ) != NULL ) {
        addKeyVal( &dataObjInp.condInput, AGE_KW, tmpValue );
        addKeyVal( &tmpCollInp.condInput, AGE_KW, tmpValue );
    }

    if ( collOprStat != NULL && *collOprStat == NULL ) {
        *collOprStat = ( collOprStat_t* )malloc( sizeof( collOprStat_t ) );
        memset( *collOprStat, 0, sizeof( collOprStat_t ) );
    }



    if ( getValByKey( &rmCollInp->condInput, ADMIN_RMTRASH_KW ) != NULL ) {
        if ( isTrashPath( rmCollInp->collName ) == False ) {
            return SYS_INVALID_FILE_PATH;
        }
        if ( rsComm->clientUser.authInfo.authFlag != LOCAL_PRIV_USER_AUTH ) {
            return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
        }
        addKeyVal( &tmpCollInp.condInput, ADMIN_RMTRASH_KW, "" );
        addKeyVal( &dataObjInp.condInput, ADMIN_RMTRASH_KW, "" );
        rmtrashFlag = 2;
    }
    else if ( getValByKey( &rmCollInp->condInput, RMTRASH_KW ) != NULL ) {
        if ( isTrashPath( rmCollInp->collName ) == False ) {
            return SYS_INVALID_FILE_PATH;
        }
        addKeyVal( &tmpCollInp.condInput, RMTRASH_KW, "" );
        addKeyVal( &dataObjInp.condInput, RMTRASH_KW, "" );
        rmtrashFlag = 1;
    }
    // =-=-=-=-=-=-=-
    // JMC - backport 4552
    if ( getValByKey( &rmCollInp->condInput, EMPTY_BUNDLE_ONLY_KW ) != NULL ) {
        addKeyVal( &tmpCollInp.condInput, EMPTY_BUNDLE_ONLY_KW, "" );
        addKeyVal( &dataObjInp.condInput, EMPTY_BUNDLE_ONLY_KW, "" );
    }
    // =-=-=-=-=-=-=-
    collEnt_t *collEnt = NULL;
    while ( ( status = rsReadCollection( rsComm, &handleInx, &collEnt ) ) >= 0 ) {
        if ( entCnt == 0 ) {
            entCnt ++;
            /* cannot rm non-empty home collection */
            if ( isHomeColl( rmCollInp->collName ) ) {
                free( collEnt );
                return CANT_RM_NON_EMPTY_HOME_COLL;
            }
        }
        if ( collEnt->objType == DATA_OBJ_T ) {
            snprintf( dataObjInp.objPath, MAX_NAME_LEN, "%s/%s",
                      collEnt->collName, collEnt->dataName );

            status = rsDataObjUnlink( rsComm, &dataObjInp );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "_rsPhyRmColl:rsDataObjUnlink failed for %s. stat = %d",
                         dataObjInp.objPath, status );
                /* need to set global error here */
                savedStatus = status;
            }
            else if ( collOprStat != NULL ) {
                ( *collOprStat )->filesCnt ++;
                if ( ( *collOprStat )->filesCnt >= fileCntPerStatOut ) {
                    rstrcpy( ( *collOprStat )->lastObjPath, dataObjInp.objPath,
                             MAX_NAME_LEN );
                    status = svrSendCollOprStat( rsComm, *collOprStat );
                    if ( status < 0 ) {
                        rodsLogError( LOG_ERROR, status,
                                      "_rsPhyRmColl: svrSendCollOprStat failed for %s. status = %d",
                                      rmCollInp->collName, status );
                        *collOprStat = NULL;
                        savedStatus = status;
                        free( collEnt );
                        break;
                    }
                    *collOprStat = ( collOprStat_t* )malloc( sizeof( collOprStat_t ) );
                    memset( *collOprStat, 0, sizeof( collOprStat_t ) );
                }
            }
        }
        else if ( collEnt->objType == COLL_OBJ_T ) {
            if ( strcmp( collEnt->collName, rmCollInp->collName ) == 0 ) {
                free( collEnt );
                collEnt = NULL;
                continue;    /* duplicate */
            }
            rstrcpy( tmpCollInp.collName, collEnt->collName, MAX_NAME_LEN );
            if ( collEnt->specColl.collClass != NO_SPEC_COLL ) {
                if ( strcmp( collEnt->collName, collEnt->specColl.collection )
                        == 0 ) {
                    free( collEnt );
                    collEnt = NULL;
                    continue;    /* no mount point */
                }
            }
            initReiWithCollInp( &rei, rsComm, &tmpCollInp, &collInfo );
            status = applyRule( "acPreprocForRmColl", NULL, &rei, NO_SAVE_REI );
            if ( status < 0 ) {
                if ( rei.status < 0 ) {
                    status = rei.status;
                }
                rodsLog( LOG_ERROR,
                         "_rsPhyRmColl:acPreprocForRmColl error for %s,stat=%d",
                         tmpCollInp.collName, status );
                free( collEnt );
                return status;
            }
            status = _rsRmCollRecur( rsComm, &tmpCollInp, collOprStat );
            rei.status = status;
            rei.status = applyRule( "acPostProcForRmColl", NULL, &rei,
                                    NO_SAVE_REI );
            if ( rei.status < 0 ) {
                rodsLog( LOG_ERROR,
                         "_rsRmColl:acPostProcForRmColl error for %s,stat=%d",
                         tmpCollInp.collName, status );
            }
        }
        if ( status < 0 ) {
            savedStatus = status;
        }
        free( collEnt );
        collEnt = NULL;
    }
    rsCloseCollection( rsComm, &handleInx );

    if ( ( rmtrashFlag > 0 && ( isTrashHome( rmCollInp->collName ) > 0 || // JMC - backport 4561
                                isOrphanPath( rmCollInp->collName ) == is_ORPHAN_HOME ) )   ||
            ( isBundlePath( rmCollInp->collName ) == True                 &&
              getValByKey( &rmCollInp->condInput, EMPTY_BUNDLE_ONLY_KW ) != NULL ) ) {
        /* don't rm user's home trash coll or orphan collection */
        status = 0;
    }
    else {
        if ( dataObjInfo != NULL && dataObjInfo->specColl != NULL ) {
            if ( dataObjInfo->specColl->collClass == LINKED_COLL ) {
                rstrcpy( rmCollInp->collName, dataObjInfo->objPath,
                         MAX_NAME_LEN );
                status = svrUnregColl( rsComm, rmCollInp );
            }
            else {
                status = l3Rmdir( rsComm, dataObjInfo );
            }
        }
        else {
            status = svrUnregColl( rsComm, rmCollInp );
            if ( status < 0 ) {
                savedStatus = status;
            }
        }
    }
    clearKeyVal( &tmpCollInp.condInput );
    clearKeyVal( &dataObjInp.condInput );

    return savedStatus;
}

int
svrUnregColl( rsComm_t *rsComm, collInp_t *rmCollInp ) {
    int status;
#ifdef RODS_CAT
    collInfo_t collInfo;
#endif
    rodsServerHost_t *rodsServerHost = NULL;

    status = getAndConnRcatHost(
                 rsComm,
                 MASTER_RCAT,
                 ( const char* )rmCollInp->collName,
                 &rodsServerHost );
    if ( status < 0 || NULL == rodsServerHost ) { // JMC cppcheck - nullptr
        return status;
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
#ifdef RODS_CAT
        memset( &collInfo, 0, sizeof( collInfo ) );
        rstrcpy( collInfo.collName, rmCollInp->collName, MAX_NAME_LEN );
        if ( getValByKey( &rmCollInp->condInput, ADMIN_RMTRASH_KW )
                != NULL ) {
            status = chlDelCollByAdmin( rsComm, &collInfo );
            if ( status >= 0 ) {
                chlCommit( rsComm );
            }
        }
        else {
            status = chlDelColl( rsComm, &collInfo );
        }

#else
        status = SYS_NO_RCAT_SERVER_ERR;
#endif
    }
    else {
        collOprStat_t *collOprStat = NULL;;
        addKeyVal( &rmCollInp->condInput, UNREG_COLL_KW, "" );
        status = _rcRmColl( rodsServerHost->conn, rmCollInp, &collOprStat );
        if ( collOprStat != NULL ) {
            free( collOprStat );
        }
    }

    return status;
}

int
rsMkTrashPath( rsComm_t *rsComm, char *objPath, char *trashPath ) {
    int status;
    char *tmpStr;
    char startTrashPath[MAX_NAME_LEN];
    char destTrashColl[MAX_NAME_LEN], myFile[MAX_NAME_LEN];
    char *trashPathPtr;

    trashPathPtr = trashPath;
    *trashPathPtr = '/';
    trashPathPtr++;
    tmpStr = objPath + 1;
    /* copy the zone */
    while ( *tmpStr != '\0' ) {
        *trashPathPtr = *tmpStr;
        trashPathPtr ++;
        if ( *tmpStr == '/' ) {
            tmpStr ++;
            break;
        }
        tmpStr ++;
    }

    if ( *tmpStr == '\0' ) {
        rodsLog( LOG_ERROR,
                 "rsMkTrashPath: input path %s too short", objPath );
        return USER_INPUT_PATH_ERR;
    }

    /* skip "home/userName/"  or "home/userName#" */

    if ( strncmp( tmpStr, "home/", 5 ) == 0 ) {
        int nameLen;
        tmpStr += 5;
        nameLen = strlen( rsComm->clientUser.userName );
        if ( strncmp( tmpStr, rsComm->clientUser.userName, nameLen ) == 0 &&
                ( *( tmpStr + nameLen ) == '/' || *( tmpStr + nameLen ) == '#' ) ) {
            /* tmpStr += (nameLen + 1); */
            tmpStr = strchr( tmpStr, '/' ) + 1;
        }
    }


    /* don't want to go back beyond /myZone/trash/home */
    *trashPathPtr = '\0';
    snprintf( startTrashPath, MAX_NAME_LEN, "%strash/home", trashPath );

    /* add home/userName/ */

    if ( rsComm->clientUser.authInfo.authFlag == REMOTE_USER_AUTH ||
            rsComm->clientUser.authInfo.authFlag == REMOTE_PRIV_USER_AUTH ) {
        /* remote user */
        snprintf( trashPathPtr, MAX_NAME_LEN, "trash/home/%s#%s/%s",
                  rsComm->clientUser.userName, rsComm->clientUser.rodsZone, tmpStr );
    }
    else {
        snprintf( trashPathPtr, MAX_NAME_LEN, "trash/home/%s/%s",
                  rsComm->clientUser.userName, tmpStr );
    }

    if ( ( status = splitPathByKey( trashPath, destTrashColl, MAX_NAME_LEN, myFile, MAX_NAME_LEN, '/' ) ) < 0 ) {
        rodsLog( LOG_ERROR,
                 "rsMkTrashPath: splitPathByKey error for %s ", trashPath );
        return USER_INPUT_PATH_ERR;
    }

    status = rsMkCollR( rsComm, startTrashPath, destTrashColl );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "rsMkTrashPath: rsMkCollR error for startPath %s, destPath %s ",
                 startTrashPath, destTrashColl );
    }

    return status;
}

int
l3Rmdir( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo ) {
    fileRmdirInp_t fileRmdirInp;
    int status;

    // =-=-=-=-=-=-=-
    // get the resc location of the hier leaf
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "l3Rmdir - failed in get_loc_for_hier_string", ret ) );
        return -1;
    }

    if ( getStructFileType( dataObjInfo->specColl ) >= 0 ) {
        subFile_t subFile;
        memset( &subFile, 0, sizeof( subFile ) );
        rstrcpy( subFile.subFilePath, dataObjInfo->subPath, MAX_NAME_LEN );
        rstrcpy( subFile.addr.hostAddr, location.c_str(), NAME_LEN );
        subFile.specColl = dataObjInfo->specColl;
        status = rsSubStructFileRmdir( rsComm, &subFile );
    }
    else {
        memset( &fileRmdirInp, 0, sizeof( fileRmdirInp ) );
        rstrcpy( fileRmdirInp.dirName, dataObjInfo->filePath, MAX_NAME_LEN );
        rstrcpy( fileRmdirInp.addr.hostAddr, location.c_str(), NAME_LEN );
        rstrcpy( fileRmdirInp.rescHier, dataObjInfo->rescHier, MAX_NAME_LEN );
        status = rsFileRmdir( rsComm, &fileRmdirInp );
    }
    return status;
}

int
rsMvCollToTrash( rsComm_t *rsComm, collInp_t *rmCollInp ) {
    int status;
    char trashPath[MAX_NAME_LEN];
    dataObjCopyInp_t dataObjRenameInp;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    int continueInx;
    dataObjInfo_t dataObjInfo;

    /* check permission of files */

    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    status = rsQueryDataObjInCollReCur( rsComm, rmCollInp->collName,
                                        &genQueryInp, &genQueryOut, ACCESS_DELETE_OBJECT, 0 );

    memset( &dataObjInfo, 0, sizeof( dataObjInfo ) );
    while ( status >= 0 ) {
        sqlResult_t *subColl, *dataObj, *rescName;
        ruleExecInfo_t rei;

        /* check if allow to delete */

        if ( ( subColl = getSqlResultByInx( genQueryOut, COL_COLL_NAME ) )
                == NULL ) {
            rodsLog( LOG_ERROR,
                     "rsMvCollToTrash: getSqlResultByInx for COL_COLL_NAME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        if ( ( dataObj = getSqlResultByInx( genQueryOut, COL_DATA_NAME ) )
                == NULL ) {
            rodsLog( LOG_ERROR,
                     "rsMvCollToTrash: getSqlResultByInx for COL_DATA_NAME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        if ( ( rescName = getSqlResultByInx( genQueryOut, COL_D_RESC_NAME ) )
                == NULL ) {
            rodsLog( LOG_ERROR,
                     "rsMvCollToTrash: getSqlResultByInx for COL_D_RESC_NAME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        snprintf( dataObjInfo.objPath, MAX_NAME_LEN, "%s/%s",
                  subColl->value, dataObj->value );
        rstrcpy( dataObjInfo.rescName, rescName->value, NAME_LEN );

        initReiWithDataObjInp( &rei, rsComm, NULL );
        rei.doi = &dataObjInfo;

        status = applyRule( "acDataDeletePolicy", NULL, &rei, NO_SAVE_REI );

        if ( status < 0 && status != NO_MORE_RULES_ERR &&
                status != SYS_DELETE_DISALLOWED ) {
            rodsLog( LOG_NOTICE,
                     "rsMvCollToTrash: acDataDeletePolicy error for %s. status = %d",
                     dataObjInfo.objPath, status );
            return status;
        }

        if ( rei.status == SYS_DELETE_DISALLOWED ) {
            rodsLog( LOG_NOTICE,
                     "rsMvCollToTrash:disallowed for %s via DataDeletePolicy,status=%d",
                     dataObjInfo.objPath, rei.status );
            return rei.status;
        }

        continueInx = genQueryOut->continueInx;

        freeGenQueryOut( &genQueryOut );

        if ( continueInx > 0 ) {
            /* More to come */
            genQueryInp.continueInx = continueInx;
            status =  rsGenQuery( rsComm, &genQueryInp, &genQueryOut );
        }
        else {
            break;
        }
    }

    if ( status < 0 && status != CAT_NO_ROWS_FOUND ) {
        rodsLog( LOG_ERROR,
                 "rsMvCollToTrash: rsQueryDataObjInCollReCur error for %s, stat=%d",
                 rmCollInp->collName, status );
        return status;
    }

    status = rsMkTrashPath( rsComm, rmCollInp->collName, trashPath );

    if ( status < 0 ) {
        appendRandomToPath( trashPath );
        status = rsMkTrashPath( rsComm, rmCollInp->collName, trashPath );
        if ( status < 0 ) {
            return status;
        }
    }

    memset( &dataObjRenameInp, 0, sizeof( dataObjRenameInp ) );

    dataObjRenameInp.srcDataObjInp.oprType =
        dataObjRenameInp.destDataObjInp.oprType = RENAME_COLL;

    rstrcpy( dataObjRenameInp.destDataObjInp.objPath, trashPath, MAX_NAME_LEN );
    rstrcpy( dataObjRenameInp.srcDataObjInp.objPath, rmCollInp->collName,
             MAX_NAME_LEN );

    status = rsDataObjRename( rsComm, &dataObjRenameInp );

    while ( status == CAT_NAME_EXISTS_AS_COLLECTION ) {
        appendRandomToPath( dataObjRenameInp.destDataObjInp.objPath );
        status = rsDataObjRename( rsComm, &dataObjRenameInp );
    }

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "mvCollToTrash: rcDataObjRename error for %s, status = %d",
                 dataObjRenameInp.destDataObjInp.objPath, status );
        return status;
    }

    return status;
}
