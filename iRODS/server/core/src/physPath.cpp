/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* physPath.c - routines for physical path operations
 */

#include <unistd.h> // JMC - backport 4598
#include <fcntl.h> // JMC - backport 4598
#include "rodsDef.h"
#include "rodsConnect.h"
#include "physPath.hpp"
#include "dataObjOpr.hpp"
#include "rodsDef.h"
#include "rsGlobalExtern.hpp"
#include "fileChksum.hpp"
#include "modDataObjMeta.hpp"
#include "objMetaOpr.hpp"
#include "collection.hpp"
#include "resource.hpp"
#include "dataObjClose.hpp"
#include "rcGlobalExtern.hpp"
#include "reGlobalsExtern.hpp"
#include "reDefines.hpp"
#include "reSysDataObjOpr.hpp"
#include "genQuery.hpp"
#include "rodsClient.hpp"
#include "readServerConfig.hpp"
#include "reFuncDefs.hpp"

#include <iostream>

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_stacktrace.hpp"
#include "irods_server_properties.hpp"

int getLeafRescPathName( const std::string& _resc_hier, std::string& _ret_string );

int
getFileMode( dataObjInp_t *dataObjInp ) {
    int createMode;
    int defFileMode;

    defFileMode = getDefFileMode();
    if ( dataObjInp != NULL &&
            ( dataObjInp->createMode & 0110 ) != 0 ) {
        if ( ( defFileMode & 0070 ) != 0 ) {
            createMode = defFileMode | 0110;
        }
        else {
            createMode = defFileMode | 0100;
        }
    }
    else {
        createMode = defFileMode;
    }

    return createMode;
}

int
getFileFlags( int l1descInx ) {
    int flags;

    dataObjInp_t *dataObjInp = L1desc[l1descInx].dataObjInp;

    if ( dataObjInp != NULL ) {
        flags = dataObjInp->openFlags;
    }
    else {
        flags = O_RDONLY;
    }

    return flags;
}


int
getFilePathName( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo,
                 dataObjInp_t *dataObjInp ) {
    char *filePath;
    vaultPathPolicy_t vaultPathPolicy;
    int status;

    if ( !dataObjInfo ) {
        rodsLog( LOG_ERROR, "getFilePathName: input dataObjInfo is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( dataObjInp != NULL &&
            ( filePath = getValByKey( &dataObjInp->condInput, FILE_PATH_KW ) ) != NULL
            && strlen( filePath ) > 0 ) {
        rstrcpy( dataObjInfo->filePath, filePath, MAX_NAME_LEN );
        return 0;
    }
    else {
    }

    /* Make up a physical path */
    if ( dataObjInfo->rescName[0] == '\0' ) {
        rodsLog( LOG_ERROR,
                 "getFilePathName: rescName for %s not resolved",
                 dataObjInp->objPath );
        return SYS_INVALID_RESC_INPUT;
    }

    int chk_path = 0;
    irods::error err = irods::get_resource_property< int >(
                           dataObjInfo->rescName,
                           irods::RESOURCE_CHECK_PATH_PERM, chk_path );
    if ( !err.ok() ) {
        irods::log( PASS( err ) );
    }

    if ( NO_CREATE_PATH == chk_path ) {
        *dataObjInfo->filePath = '\0';
        return 0;
    }

    std::string vault_path;
    status = getLeafRescPathName( dataObjInfo->rescHier, vault_path );
    if ( status != 0 ) {
        return status;
    }

    status = getVaultPathPolicy( rsComm, dataObjInfo, &vaultPathPolicy );
    if ( status < 0 ) {
        return status;
    }

    if ( vaultPathPolicy.scheme == GRAFT_PATH_S ) {
        status = setPathForGraftPathScheme( dataObjInp->objPath,
                                            vault_path.c_str(), vaultPathPolicy.addUserName,
                                            rsComm->clientUser.userName, vaultPathPolicy.trimDirCnt,
                                            dataObjInfo->filePath );
    }
    else {
        status = setPathForRandomScheme( dataObjInp->objPath,
                                         vault_path.c_str(), rsComm->clientUser.userName,
                                         dataObjInfo->filePath );
    }

    return status;
}


int
getVaultPathPolicy( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo,
                    vaultPathPolicy_t *outVaultPathPolicy ) {
    ruleExecInfo_t rei;
    msParam_t *msParam;
    int status;

    if ( outVaultPathPolicy == NULL || dataObjInfo == NULL || rsComm == NULL ) {
        rodsLog( LOG_ERROR,
                 "getVaultPathPolicy: NULL input" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
    initReiWithDataObjInp( &rei, rsComm, NULL );

    rei.doi = dataObjInfo;
    status = applyRule( "acSetVaultPathPolicy", NULL, &rei, NO_SAVE_REI );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "getVaultPathPolicy: rule acSetVaultPathPolicy error, status = %d",
                 status );
        return status;
    }

    if ( ( msParam = getMsParamByLabel( &rei.inOutMsParamArray,
                                        VAULT_PATH_POLICY ) ) == NULL ) {
        /* use the default */
        outVaultPathPolicy->scheme = DEF_VAULT_PATH_SCHEME;
        outVaultPathPolicy->addUserName = DEF_ADD_USER_FLAG;
        outVaultPathPolicy->trimDirCnt = DEF_TRIM_DIR_CNT;
    }
    else {
        *outVaultPathPolicy = *( ( vaultPathPolicy_t * ) msParam->inOutStruct );
        clearMsParamArray( &rei.inOutMsParamArray, 1 );
    }
    /* make sure trimDirCnt is <= 1 */
    if ( outVaultPathPolicy->trimDirCnt > DEF_TRIM_DIR_CNT ) {
        outVaultPathPolicy->trimDirCnt = DEF_TRIM_DIR_CNT;
    }

    return 0;
}

int
setPathForRandomScheme( char *objPath, const char *vaultPath, char *userName,
                        char *outPath ) {
    int myRandom;
    int dir1, dir2;
    char logicalCollName[MAX_NAME_LEN];
    char logicalFileName[MAX_NAME_LEN];
    int status;

    myRandom = random();
    dir1 = myRandom & 0xf;
    dir2 = ( myRandom >> 4 ) & 0xf;

    status = splitPathByKey( objPath,
                             logicalCollName, MAX_NAME_LEN, logicalFileName, MAX_NAME_LEN, '/' );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "setPathForRandomScheme: splitPathByKey error for %s, status = %d",
                 outPath, status );
        return status;
    }

    snprintf( outPath, MAX_NAME_LEN,
              "%s/%s/%d/%d/%s.%d", vaultPath, userName, dir1, dir2,
              logicalFileName, ( uint ) time( NULL ) );
    return 0;
}

int
setPathForGraftPathScheme( char *objPath, const char *vaultPath, int addUserName,
                           char *userName, int trimDirCnt, char *outPath ) {
    int i;
    char *objPathPtr, *tmpPtr;
    int len;

    objPathPtr = objPath + 1;

    for ( i = 0; i < trimDirCnt; i++ ) {
        tmpPtr = strchr( objPathPtr, '/' );
        if ( tmpPtr == NULL ) {
            rodsLog( LOG_ERROR,
                     "setPathForGraftPathScheme: objPath %s too short", objPath );
            break;      /* just use the shorten one */
        }
        else {
            /* skip over '/' */
            objPathPtr = tmpPtr + 1;
            /* don't skip over the trash path */
            if ( i == 0 && strncmp( objPathPtr, "trash/", 6 ) == 0 ) {
                break;
            }
        }
    }

    if ( addUserName > 0 && userName != NULL ) {
        len = snprintf( outPath, MAX_NAME_LEN,
                        "%s/%s/%s", vaultPath, userName, objPathPtr );
    }
    else {
        len = snprintf( outPath, MAX_NAME_LEN,
                        "%s/%s", vaultPath, objPathPtr );
    }

    if ( len >= MAX_NAME_LEN ) {
        rodsLog( LOG_ERROR,
                 "setPathForGraftPathScheme: filePath %s too long", objPath );
        return USER_STRLEN_TOOLONG;
    }
    else {
        return 0;
    }
}

/* resolveDupFilePath - try to resolve deplicate file path in the same
 * resource.
 */

int
resolveDupFilePath( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo,
                    dataObjInp_t *dataObjInp ) {
    char tmpStr[NAME_LEN];
    char *filePath;

    if ( getSizeInVault( rsComm, dataObjInfo ) == SYS_PATH_IS_NOT_A_FILE ) {
        /* a dir */
        return SYS_PATH_IS_NOT_A_FILE;
    }
    if ( chkAndHandleOrphanFile( rsComm, dataObjInfo->objPath, dataObjInfo->rescHier, dataObjInfo->filePath,
                                 dataObjInfo->rescName, dataObjInfo->replStatus ) >= 0 ) {
        /* this is an orphan file or has been renamed */
        return 0;
    }

    if ( dataObjInp != NULL ) {
        filePath = getValByKey( &dataObjInp->condInput, FILE_PATH_KW );
        if ( filePath != NULL && strlen( filePath ) > 0 ) {
            return -1;
        }
    }

    if ( strlen( dataObjInfo->filePath ) >= MAX_NAME_LEN - 3 ) {
        return -1;
    }

    snprintf( tmpStr, NAME_LEN, ".%d", dataObjInfo->replNum );
    strcat( dataObjInfo->filePath, tmpStr );

    return 0;
}

int
getchkPathPerm( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                dataObjInfo_t *dataObjInfo ) {
    int chkPathPerm;
    char *filePath;
    ruleExecInfo_t rei;

    if ( rsComm->clientUser.authInfo.authFlag == LOCAL_PRIV_USER_AUTH ) {
        return NO_CHK_PATH_PERM;
    }

    if ( dataObjInp == NULL || dataObjInfo == NULL ) {
        return NO_CHK_PATH_PERM;
    }

    if ( ( filePath = getValByKey( &dataObjInp->condInput, FILE_PATH_KW ) ) != NULL
            && strlen( filePath ) > 0 ) {
        /* the user input a path */
        if ( !strlen( dataObjInfo->rescName ) ) {
            chkPathPerm = NO_CHK_PATH_PERM;
        }
        else {
            initReiWithDataObjInp( &rei, rsComm, dataObjInp );
            rei.doi = dataObjInfo;
            // =-=-=-=-=-=-=-
            // JMC - backport 4774
            rei.status = DISALLOW_PATH_REG;             /* default */
            applyRule( "acSetChkFilePathPerm", NULL, &rei, NO_SAVE_REI );

            int chk_path = 0;
            irods::error err = irods::get_resource_property< int >( dataObjInfo->rescName,
                               irods::RESOURCE_CHECK_PATH_PERM, chk_path );
            if ( !err.ok() ) {
                irods::log( PASS( err ) );
            }

            if ( err.ok() && ( rei.status == NO_CHK_PATH_PERM || NO_CHK_PATH_PERM == chk_path ) ) {
                chkPathPerm = NO_CHK_PATH_PERM;
            }
            else {
                chkPathPerm = rei.status;
                // =-=-=-=-=-=-=-
            }
        }
    }
    else {
        chkPathPerm = NO_CHK_PATH_PERM;
    }
    return chkPathPerm;
}

int
getCopiesFromCond( keyValPair_t *condInput ) {
    char *myValue;

    myValue = getValByKey( condInput, COPIES_KW );

    if ( myValue == NULL ) {
        return 1;
    }
    else if ( strcmp( myValue, "all" ) == 0 ) {
        return ALL_COPIES;
    }
    else {
        return atoi( myValue );
    }
}

int
getWriteFlag( int openFlag ) {
    if ( openFlag & O_WRONLY || openFlag & O_RDWR ) {
        return 1;
    }
    else {
        return 0;
    }
}

rodsLong_t
getSizeInVault( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo ) {
    rodsStat_t *myStat = NULL;
    int status;
    rodsLong_t mysize;

    status = l3Stat( rsComm, dataObjInfo, &myStat );

    if ( status < 0 || NULL == myStat ) { // JMC cppcheck - nullptr
        rodsLog( LOG_DEBUG,
                 "getSizeInVault: l3Stat error for %s. status = %d",
                 dataObjInfo->filePath, status );
        return status;
    }
    else {
        if ( myStat->st_mode & S_IFDIR ) {
            free( myStat );
            return ( rodsLong_t ) SYS_PATH_IS_NOT_A_FILE;
        }
        mysize = myStat->st_size;
        free( myStat );
        return mysize;
    }
}

int
_dataObjChksum(
    rsComm_t*      rsComm,
    dataObjInfo_t* dataObjInfo,
    char**         chksumStr ) { // JMC - backport 4527
    fileChksumInp_t fileChksumInp;
    int status = 0;

    // =-=-=-=-=-=-=-
    int category = FILE_CAT; // only supporting file resource, not DB right now
    char* orig_chksum = getValByKey( &dataObjInfo->condInput, ORIG_CHKSUM_KW );

    std::string location;
    irods::error ret;
    // JMC - legacy resource - switch ( RescTypeDef[rescTypeInx].rescCat) {
    switch ( category ) {
    case FILE_CAT:
        // =-=-=-=-=-=-=-
        // get the resource location for the hier string leaf
        ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
        if ( !ret.ok() ) {
            irods::log( PASSMSG( "_dataObjChksum - failed in get_loc_for_hier_string", ret ) );
            return -1;
        }

        memset( &fileChksumInp, 0, sizeof( fileChksumInp ) );
        rstrcpy( fileChksumInp.addr.hostAddr, location.c_str(), NAME_LEN );
        rstrcpy( fileChksumInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN );
        rstrcpy( fileChksumInp.rescHier, dataObjInfo->rescHier, MAX_NAME_LEN );
        rstrcpy( fileChksumInp.objPath,  dataObjInfo->objPath, MAX_NAME_LEN );
        rstrcpy( fileChksumInp.in_pdmo,  dataObjInfo->in_pdmo, MAX_NAME_LEN );

        if ( orig_chksum ) {
            rstrcpy( fileChksumInp.orig_chksum, orig_chksum, CHKSUM_LEN );
        }

        status = rsFileChksum( rsComm, &fileChksumInp, chksumStr );

        if ( status == DIRECT_ARCHIVE_ACCESS ) {
            std::stringstream msg;
            msg << "Data object: \"";
            msg << dataObjInfo->filePath;
            msg << "\" is located in an archive resource. Ignoring its checksum.";
            irods::log( LOG_DEBUG, msg.str() );

        }
        break;
    default:
        rodsLog( LOG_NOTICE,
                 "_dataObjChksum: rescCat type %d is not recognized", category );
        status = SYS_INVALID_RESC_TYPE;
        break;
    }

    return status;
}

int
dataObjChksumAndReg( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo,
                     char **chksumStr ) {
    keyValPair_t regParam;
    modDataObjMeta_t modDataObjMetaInp;
    int status;

    status = _dataObjChksum( rsComm, dataObjInfo, chksumStr );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "dataObjChksumAndReg: _dataObjChksum error for %s, status = %d",
                 dataObjInfo->objPath, status );
        return status;
    }

    /* register it */
    memset( &regParam, 0, sizeof( regParam ) );
    addKeyVal( &regParam, CHKSUM_KW, *chksumStr );

    // always set pdmo flag here... total hack - harry
    addKeyVal( &regParam, IN_PDMO_KW, dataObjInfo->rescHier );

    modDataObjMetaInp.dataObjInfo = dataObjInfo;
    modDataObjMetaInp.regParam = &regParam;

    status = rsModDataObjMeta( rsComm, &modDataObjMetaInp );

    clearKeyVal( &regParam );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "dataObjChksumAndReg: rsModDataObjMeta error for %s, status = %d",
                 dataObjInfo->objPath, status );
        /* don't return error because it is not fatal */
    }

    return 0;
}

/* chkAndHandleOrphanFile - Check whether the file is an orphan file.
 * If it is, rename it.
 * If it belongs to an old copy, move the old path and register it.
 *
 * return 0 - the filePath is NOT an orphan file.
 *        1 - the filePath is an orphan file and has been renamed.
 *        < 0 - error
 */

int
chkAndHandleOrphanFile( rsComm_t *rsComm, char* objPath, char* rescHier, char *filePath, const char *_resc_name,
                        int replStatus ) {

    fileRenameInp_t fileRenameInp;
    int status;
    dataObjInfo_t myDataObjInfo;

    if ( strlen( filePath ) + 17 >= MAX_NAME_LEN ) {
        /* the new path name will be too long to add "/orphan + random" */
        return -1;
    }

    /* check if the input filePath is assocated with a dataObj */

    memset( &myDataObjInfo, 0, sizeof( myDataObjInfo ) );
    memset( &fileRenameInp, 0, sizeof( fileRenameInp ) );
    if ( ( status = chkOrphanFile( rsComm, filePath, _resc_name, &myDataObjInfo ) ) == 0 ) {

        rstrcpy( fileRenameInp.oldFileName, filePath, MAX_NAME_LEN );
        rstrcpy( fileRenameInp.rescHier, rescHier, MAX_NAME_LEN );
        rstrcpy( fileRenameInp.objPath, objPath, MAX_NAME_LEN );

        /* not an orphan file */
        if ( replStatus > OLD_COPY || isTrashPath( myDataObjInfo.objPath ) ) {
            modDataObjMeta_t modDataObjMetaInp;
            keyValPair_t regParam;

            /* a new copy or the current path is in trash.
             * rename and reg the path of the old one */
            char new_fn[ MAX_NAME_LEN ];
            status = renameFilePathToNewDir( rsComm, REPL_DIR, &fileRenameInp,
                                             1, new_fn );
            if ( status < 0 ) {
                char* sys_error;
                char* rods_error = rodsErrorName( status, &sys_error );
                rodsLog( LOG_ERROR, "%s:%d renameFilePathToNewDir failed for file: %s - status = %d %s %s",
                         __FUNCTION__, __LINE__, filePath, status, rods_error, sys_error );
                return status;
            }
            /* register the change */
            memset( &regParam, 0, sizeof( regParam ) );
            addKeyVal( &regParam, FILE_PATH_KW, new_fn );//fileRenameInp.newFileName );
            modDataObjMetaInp.dataObjInfo = &myDataObjInfo;
            modDataObjMetaInp.regParam = &regParam;
            status = rsModDataObjMeta( rsComm, &modDataObjMetaInp );
            clearKeyVal( &regParam );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "%s:%d rsModDataObjMeta of %s error. stat = %d",
                         __FUNCTION__, __LINE__, fileRenameInp.newFileName, status );
                /* need to rollback the change in path */
                rstrcpy( fileRenameInp.oldFileName, fileRenameInp.newFileName,
                         MAX_NAME_LEN );
                rstrcpy( fileRenameInp.newFileName, filePath, MAX_NAME_LEN );
                rstrcpy( fileRenameInp.rescHier, myDataObjInfo.rescHier, MAX_NAME_LEN );
                fileRenameOut_t* ren_out = 0;
                status = rsFileRename( rsComm, &fileRenameInp, &ren_out );
                free( ren_out );

                if ( status < 0 ) {
                    rodsLog( LOG_ERROR,
                             "%s:%d rsFileRename %s failed, status = %d",
                             __FUNCTION__, __LINE__, fileRenameInp.oldFileName, status );
                    return status;
                }
                /* this thing still failed */
                return -1;
            }
            else {
                return 0;
            }
        }
        else {

            /* this is an old copy. change the path but don't
             * actually rename it */
            rstrcpy( fileRenameInp.oldFileName, filePath, MAX_NAME_LEN );
            char new_fn[ MAX_NAME_LEN ];
            status = renameFilePathToNewDir( rsComm, REPL_DIR, &fileRenameInp,
                                             0, new_fn );
            if ( status >= 0 ) {
                //rstrcpy( filePath, fileRenameInp.newFileName, MAX_NAME_LEN );
                rstrcpy( filePath, new_fn, MAX_NAME_LEN );
                return 0;
            }
            else {
                char* sys_error;
                char* rods_error = rodsErrorName( status, &sys_error );
                rodsLog( LOG_ERROR, "%s:%d renameFilePathToNewDir failed for file: %s - status = %d %s %s",
                         __FUNCTION__, __LINE__, filePath, status, rods_error, sys_error );
                return status;
            }
        }

    }
    else if ( status > 0 ) {
        /* this is an orphan file. need to rename it */
        rstrcpy( fileRenameInp.oldFileName, filePath,           MAX_NAME_LEN );
        rstrcpy( fileRenameInp.rescHier,    rescHier,           MAX_NAME_LEN );
        rstrcpy( fileRenameInp.objPath,     objPath,            MAX_NAME_LEN );
        char new_fn[ MAX_NAME_LEN ];
        status = renameFilePathToNewDir( rsComm, ORPHAN_DIR, &fileRenameInp,
                                         1, new_fn );
        if ( status >= 0 ) {
            return 1;
        }
        else {
            char* sys_error;
            char* rods_error = rodsErrorName( status, &sys_error );
            rodsLog( LOG_ERROR, "%s:%d renameFilePathToNewDir failed for file: %s - status = %d %s %s",
                     __FUNCTION__, __LINE__, filePath, status, rods_error, sys_error );
            return status;
        }
    }
    else {
        /* error */
        rodsLog( LOG_ERROR, "%s:%d chkOrphanFile failed for file: %s",
                 __FUNCTION__, __LINE__, filePath );
        return status;
    }
}

int
renameFilePathToNewDir( rsComm_t *rsComm, char *newDir,
                        fileRenameInp_t *fileRenameInp,
                        int renameFlag, char* new_fn ) {
    int len, status;
    char *oldPtr, *newPtr;
    char *filePath = fileRenameInp->oldFileName;
    // =-=-=-=-=-=-=-
    // get the resource location for the hier string leaf
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( fileRenameInp->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "renameFilePathToNewDir - failed in get_loc_for_hier_string", ret ) );
        return -1;
    }

    rstrcpy( fileRenameInp->addr.hostAddr, location.c_str(), NAME_LEN );

    std::string vault_path;
    ret = irods::get_vault_path_for_hier_string( fileRenameInp->rescHier, vault_path );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Unable to determine vault path from resource hierarch: \"";
        msg << fileRenameInp->rescHier;
        msg << "\"";
        irods::error result = PASSMSG( msg.str(), ret );
        irods::log( result );
        return result.code();
    }

    len = vault_path.size();

    if ( vault_path.empty() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Vault path is empty.";
        irods::error result = ERROR( RESCVAULTPATH_EMPTY_IN_STRUCT_ERR, msg.str() );
        irods::log( result );
        return result.code();
    }

    std::string file_path = filePath;
    if ( file_path.find( vault_path ) != 0 ) {
        /* not in rescVaultPath */
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - File: \"";
        msg << filePath;
        msg << "\" is not in vault: \"";
        msg << vault_path;
        msg << "\"";
        irods::error result = ERROR( FILE_NOT_IN_VAULT, msg.str() );
        irods::log( result );
        return result.code();
    }

    rstrcpy( fileRenameInp->newFileName, vault_path.c_str(), MAX_NAME_LEN );
    oldPtr = filePath + len;
    newPtr = fileRenameInp->newFileName + len;

    snprintf( newPtr, MAX_NAME_LEN - len, "/%s%s.%-d", newDir, oldPtr,
              ( uint ) random() );


    if ( renameFlag > 0 ) {
        fileRenameOut_t* ren_out = 0;
        status = rsFileRename( rsComm, fileRenameInp, &ren_out );
        strncpy( new_fn, ren_out->file_name, MAX_NAME_LEN );
        free( ren_out );
        if ( status < 0 ) {
            rodsLog( LOG_NOTICE,
                     "renameFilePathToNewDir:rsFileRename from %s to %s failed,stat=%d",
                     filePath, fileRenameInp->newFileName, status );
            return status;
        }
        else {
            return 0;
        }
    }
    else {
        return 0;
    }
}

/* syncDataObjPhyPath - sync the path of the phy path with the path of
 * the data ovject. This is unsed by rename to sync the path of the
 * phy path with the new path.
 */

int
syncDataObjPhyPath( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                    dataObjInfo_t *dataObjInfoHead, char *acLCollection ) {
    dataObjInfo_t *tmpDataObjInfo;
    int status;
    int savedStatus = 0;

    tmpDataObjInfo = dataObjInfoHead;
    while ( tmpDataObjInfo != NULL ) {
        status = syncDataObjPhyPathS( rsComm, dataObjInp, tmpDataObjInfo, acLCollection );

        if ( status < 0 ) {
            savedStatus = status;
        }

        tmpDataObjInfo = tmpDataObjInfo->next;
    }

    return savedStatus;
}

int
syncDataObjPhyPathS( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                     dataObjInfo_t *dataObjInfo, char *acLCollection ) {
    int status, status1;
    fileRenameInp_t fileRenameInp;
    modDataObjMeta_t modDataObjMetaInp;
    keyValPair_t regParam;
    vaultPathPolicy_t vaultPathPolicy;
    irods::error err;

    if ( strcmp( dataObjInfo->rescName, BUNDLE_RESC ) == 0 ) {
        return 0;
    }

    int create_path = 0;
    err = irods::get_resource_property< int >(
              dataObjInfo->rescName,
              irods::RESOURCE_CREATE_PATH, create_path );
    if ( !err.ok() ) {
        irods::log( PASS( err ) );
    }

    if ( NO_CREATE_PATH == create_path ) {
        /* no need to sync for path created by resource */
        return 0;
    }

    status = getVaultPathPolicy( rsComm, dataObjInfo, &vaultPathPolicy );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "syncDataObjPhyPathS: getVaultPathPolicy error for %s, status = %d",
                 dataObjInfo->objPath, status );
    }
    else {
        if ( vaultPathPolicy.scheme != GRAFT_PATH_S ) {
            /* no need to sync */
            return 0;
        }
    }

    if ( isInVault( dataObjInfo ) == 0 ) {
        /* not in vault. */
        return 0;
    }

    err = irods::is_resc_live( dataObjInfo->rescName );
    if ( !err.ok() ) {
        irods::log( PASSMSG( "syncDataObjPhyPathS - failed in is_resc_live", err ) );
        return err.code();
    }

    // =-=-=-=-=-=-=-
    // get the resource location for the hier string leaf
    std::string location;
    err = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if ( !err.ok() ) {
        irods::log( PASSMSG( "syncDataObjPhyPathS - failed in get_loc_for_hier_string", err ) );
        return err.code();
    }

    /* Save the current objPath */
    memset( &fileRenameInp, 0, sizeof( fileRenameInp ) );
    rstrcpy( fileRenameInp.oldFileName, dataObjInfo->filePath, MAX_NAME_LEN );
    rstrcpy( fileRenameInp.rescHier, dataObjInfo->rescHier, MAX_NAME_LEN );
    rstrcpy( fileRenameInp.objPath, dataObjInfo->objPath, MAX_NAME_LEN );
    if ( dataObjInp == NULL ) {
        dataObjInp_t myDdataObjInp;
        memset( &myDdataObjInp, 0, sizeof( myDdataObjInp ) );
        rstrcpy( myDdataObjInp.objPath, dataObjInfo->objPath, MAX_NAME_LEN );
        status = getFilePathName( rsComm, dataObjInfo, &myDdataObjInp );
    }
    else {
        status = getFilePathName( rsComm, dataObjInfo, dataObjInp );
    }

    if ( strcmp( fileRenameInp.oldFileName, dataObjInfo->filePath ) == 0 ) {
        return 0;
    }

    /* see if the new file exist */
    if ( getSizeInVault( rsComm, dataObjInfo ) >= 0 ) {
        if ( ( status = chkAndHandleOrphanFile( rsComm, dataObjInfo->objPath, dataObjInfo->rescHier,
                                                dataObjInfo->filePath, dataObjInfo->rescName, OLD_COPY ) ) <= 0 ) {
            rodsLog( LOG_ERROR,
                     "%s: newFileName %s already in use. Status = %d",
                     __FUNCTION__, dataObjInfo->filePath, status );
            return SYS_PHY_PATH_INUSE;
        }
    }

    /* rename it */
    rstrcpy( fileRenameInp.addr.hostAddr, location.c_str(), NAME_LEN );
    rstrcpy( fileRenameInp.newFileName, dataObjInfo->filePath,
             MAX_NAME_LEN );

    fileRenameOut_t* ren_out = 0;
    status = rsFileRename( rsComm, &fileRenameInp, &ren_out );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "syncDataObjPhyPath:rsFileRename from %s to %s failed,status=%d",
                 fileRenameInp.oldFileName, fileRenameInp.newFileName, status );
        free( ren_out );
        return status;
    }

    // =-=-=-=-=-=-=-
    // update phy path with possible new path from the resource
    strncpy( dataObjInfo->filePath, ren_out->file_name, MAX_NAME_LEN );

    /* register the change */
    memset( &regParam, 0, sizeof( regParam ) );
    addKeyVal( &regParam, FILE_PATH_KW, ren_out->file_name );
    free( ren_out );
    if ( acLCollection != NULL ) {
        addKeyVal( &regParam, ACL_COLLECTION_KW, acLCollection );
    }
    modDataObjMetaInp.dataObjInfo = dataObjInfo;
    modDataObjMetaInp.regParam = &regParam;
    status = rsModDataObjMeta( rsComm, &modDataObjMetaInp );
    clearKeyVal( &regParam );
    if ( status < 0 ) {
        char tmpPath[MAX_NAME_LEN];
        rodsLog( LOG_ERROR,
                 "syncDataObjPhyPath: rsModDataObjMeta of %s error. stat = %d",
                 fileRenameInp.newFileName, status );
        /* need to rollback the change in path */
        rstrcpy( tmpPath, fileRenameInp.oldFileName, MAX_NAME_LEN );
        rstrcpy( fileRenameInp.oldFileName, fileRenameInp.newFileName,
                 MAX_NAME_LEN );
        rstrcpy( fileRenameInp.newFileName, tmpPath, MAX_NAME_LEN );
        fileRenameOut_t* ren_out = 0;
        status1 = rsFileRename( rsComm, &fileRenameInp, &ren_out );
        free( ren_out );

        if ( status1 < 0 ) {
            rodsLog( LOG_ERROR,
                     "syncDataObjPhyPath: rollback rename %s failed, status = %d",
                     fileRenameInp.oldFileName, status1 );
        }
        return status;
    }
    return 0;
}

/* syncCollPhyPath - sync the path of the phy path with the path of
 * the data ovject in the new collection. This is unsed by rename to sync
 * the path of the phy path with the new path.
 */

int
syncCollPhyPath( rsComm_t *rsComm, char *collection ) {
    int status, i;
    int savedStatus = 0;
    genQueryOut_t *genQueryOut = NULL;
    genQueryInp_t genQueryInp;
    int continueInx;

    status = rsQueryDataObjInCollReCur( rsComm, collection,
                                        &genQueryInp, &genQueryOut, NULL, 0 );

    if ( status < 0 && status != CAT_NO_ROWS_FOUND ) {
        savedStatus = status; /* return the error code */
    }

    while ( status >= 0 ) {
        sqlResult_t *dataIdRes, *subCollRes, *dataNameRes, *replNumRes,
                    *rescNameRes, *filePathRes, *rescHierRes;
        char *tmpDataId, *tmpDataName, *tmpSubColl, *tmpReplNum,
             *tmpRescName, *tmpFilePath, *tmpRescHier;
        dataObjInfo_t dataObjInfo;

        memset( &dataObjInfo, 0, sizeof( dataObjInfo ) );

        if ( ( dataIdRes = getSqlResultByInx( genQueryOut, COL_D_DATA_ID ) )
                == NULL ) {
            rodsLog( LOG_ERROR,
                     "syncCollPhyPath: getSqlResultByInx for COL_D_DATA_ID failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        if ( ( subCollRes = getSqlResultByInx( genQueryOut, COL_COLL_NAME ) )
                == NULL ) {
            rodsLog( LOG_ERROR,
                     "syncCollPhyPath: getSqlResultByInx for COL_COLL_NAME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        if ( ( dataNameRes = getSqlResultByInx( genQueryOut, COL_DATA_NAME ) )
                == NULL ) {
            rodsLog( LOG_ERROR,
                     "syncCollPhyPath: getSqlResultByInx for COL_DATA_NAME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        if ( ( replNumRes = getSqlResultByInx( genQueryOut, COL_DATA_REPL_NUM ) )
                == NULL ) {
            rodsLog( LOG_ERROR,
                     "syncCollPhyPath:getSqlResultByIn for COL_DATA_REPL_NUM failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        if ( ( rescNameRes = getSqlResultByInx( genQueryOut, COL_D_RESC_NAME ) )
                == NULL ) {
            rodsLog( LOG_ERROR,
                     "syncCollPhyPath: getSqlResultByInx for COL_D_RESC_NAME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        if ( ( filePathRes = getSqlResultByInx( genQueryOut, COL_D_DATA_PATH ) )
                == NULL ) {
            rodsLog( LOG_ERROR,
                     "syncCollPhyPath: getSqlResultByInx for COL_D_DATA_PATH failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        if ( ( rescHierRes = getSqlResultByInx( genQueryOut, COL_D_RESC_HIER ) )
                == NULL ) {
            rodsLog( LOG_ERROR,
                     "syncCollPhyPath: getSqlResultByInx for COL_D_RESC_HIER failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
            tmpDataId = &dataIdRes->value[dataIdRes->len * i];
            tmpDataName = &dataNameRes->value[dataNameRes->len * i];
            tmpSubColl = &subCollRes->value[subCollRes->len * i];
            tmpReplNum = &replNumRes->value[replNumRes->len * i];
            tmpRescName = &rescNameRes->value[rescNameRes->len * i];
            tmpFilePath = &filePathRes->value[filePathRes->len * i];
            tmpRescHier = &rescHierRes->value[rescHierRes->len * i];

            dataObjInfo.dataId = strtoll( tmpDataId, 0, 0 );
            snprintf( dataObjInfo.objPath, MAX_NAME_LEN, "%s/%s",
                      tmpSubColl, tmpDataName );
            dataObjInfo.replNum = atoi( tmpReplNum );
            rstrcpy( dataObjInfo.rescName, tmpRescName, NAME_LEN );
            rstrcpy( dataObjInfo.rescHier, tmpRescHier, MAX_NAME_LEN );

            rstrcpy( dataObjInfo.filePath, tmpFilePath, MAX_NAME_LEN );

            status = syncDataObjPhyPathS( rsComm, NULL, &dataObjInfo,
                                          collection );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "syncCollPhyPath: syncDataObjPhyPathS error for %s,stat=%d",
                         dataObjInfo.filePath, status );
                savedStatus = status;
            }

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
    clearGenQueryInp( &genQueryInp );

    return savedStatus;
}

int
isInVault( dataObjInfo_t *dataObjInfo ) {
    int len;

    if ( !dataObjInfo ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    std::string vault_path;
    irods::error ret = irods::get_vault_path_for_hier_string( dataObjInfo->rescHier, vault_path );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Failed to get the vault path for the hierarchy: \"" << dataObjInfo->rescHier << "\"";
        ret = PASSMSG( msg.str(), ret );
        irods::log( ret );
        return ret.code();
    }
    len = vault_path.size();

    if ( strncmp( vault_path.c_str(),
                  dataObjInfo->filePath, len ) == 0 ) {
        return 1;
    }
    else {
        return 0;
    }
}

int
getDefFileMode() {
    int defFileMode = DEFAULT_FILE_MODE;

    irods::error ret = irods::server_properties::getInstance().get_property<int>( DEF_FILE_MODE_KW, defFileMode );

    return defFileMode;
}

int
getDefDirMode() {
    int defDirMode = DEFAULT_DIR_MODE;

    irods::error ret = irods::server_properties::getInstance().get_property<int>( DEF_DIR_MODE_KW, defDirMode );

    return defDirMode;
}

int
getLogPathFromPhyPath( char *phyPath, const char *rescVaultPath, char *outLogPath ) {
    int len;
    char *tmpPtr;
    zoneInfo_t *tmpZoneInfo = NULL;
    int status;

    if ( !phyPath || !rescVaultPath || !outLogPath ) {
        return USER__NULL_INPUT_ERR;
    }

    len = strlen( rescVaultPath );
    if ( strncmp( rescVaultPath, phyPath, len ) != 0 ) {
        return -1;
    }
    tmpPtr = phyPath + len;

    if ( *tmpPtr != '/' ) {
        return -1;
    }

    tmpPtr ++;
    status = getLocalZoneInfo( &tmpZoneInfo );
    if ( status < 0 || NULL == tmpZoneInfo ) {
        return status;    // JMC cppcheck - nullptr
    }

    len = strlen( tmpZoneInfo->zoneName );
    if ( strncmp( tmpZoneInfo->zoneName, tmpPtr, len ) == 0 &&
            *( tmpPtr + len ) == '/' ) {
        /* start with zoneName */
        tmpPtr += ( len + 1 );
    }

    snprintf( outLogPath, MAX_NAME_LEN, "/%s/%s", tmpZoneInfo->zoneName,
              tmpPtr );

    return 0;
}

/* rsMkOrphanPath - given objPath, compose the orphan path which is
 * /myZone/trash/orphan/userName#userZone/filename.random
 * Also make the required directory.
 */
int
rsMkOrphanPath( rsComm_t *rsComm, char *objPath, char *orphanPath ) {
    int status;
    char *tmpStr;
    char *orphanPathPtr;
    int len = 0;
    char parentColl[MAX_NAME_LEN], childName[MAX_NAME_LEN];
    collInp_t collCreateInp;

    status = splitPathByKey( objPath, parentColl, MAX_NAME_LEN, childName, MAX_NAME_LEN, '/' );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "rsMkOrphanPath: splitPathByKey error for %s, status = %d",
                 objPath, status );
        return status;
    }
    orphanPathPtr = orphanPath;
    *orphanPathPtr = '/';
    orphanPathPtr++;
    tmpStr = objPath + 1;
    /* copy the zone */
    while ( *tmpStr != '\0' ) {
        *orphanPathPtr = *tmpStr;
        orphanPathPtr ++;
        len ++;
        if ( *tmpStr == '/' ) {
            tmpStr ++;
            break;
        }
        tmpStr ++;
    }
    len = strlen( rsComm->clientUser.userName ) +
          strlen( rsComm->clientUser.rodsZone );
    snprintf( orphanPathPtr, len + 20, "trash/orphan/%s#%s",
              rsComm->clientUser.userName, rsComm->clientUser.rodsZone );

    memset( &collCreateInp, 0, sizeof( collCreateInp ) );
    rstrcpy( collCreateInp.collName, orphanPath, MAX_NAME_LEN );
    addKeyVal( &collCreateInp.condInput, RECURSIVE_OPR__KW, "" );	// in case there is no trash/orphan collection

    status = rsCollCreate( rsComm, &collCreateInp );

    if ( status < 0 && status != CAT_NAME_EXISTS_AS_COLLECTION ) {
        rodsLogError( LOG_ERROR, status,
                      "rsMkOrphanPath: rsCollCreate error for %s",
                      orphanPath );
    }
    orphanPathPtr = orphanPath + strlen( orphanPath );

    snprintf( orphanPathPtr, strlen( childName ) + 20, "/%s.%-d",
              childName, ( uint ) random() );

    return 0;
}

// =-=-=-=-=-=-=-
// JMC - backport 4598
int
getDataObjLockPath( char *objPath, char **outLockPath ) {
    int i;
    char *objPathPtr, *tmpPtr;
    char tmpPath[MAX_NAME_LEN];
    int c;
    int len;

    if ( objPath == NULL || outLockPath == NULL ) {
        return USER__NULL_INPUT_ERR;
    }
    objPathPtr = objPath; // JMC - backport 4604

    /* skip over the first 3 '/' */
    for ( i = 0; i < 3; i++ ) {
        tmpPtr = strchr( objPathPtr, '/' );
        if ( tmpPtr == NULL ) {
            break;      /* just use the shorten one */
        }
        else {
            /* skip over '/' */
            objPathPtr = tmpPtr + 1;
        }
    }
    rstrcpy( tmpPath, objPathPtr, MAX_NAME_LEN );
    /* replace all '/' with '.' */
    objPathPtr = tmpPath;

    while ( ( c = *objPathPtr ) != '\0' ) {
        if ( c == '/' ) {
            *objPathPtr = '.';
        }
        objPathPtr++;
    }

    len = strlen( getConfigDir() ) + strlen( LOCK_FILE_DIR ) +
          strlen( tmpPath ) + strlen( LOCK_FILE_TRAILER ) + 10; // JMC - backport 6404
    *outLockPath = ( char * ) malloc( len );

    snprintf( *outLockPath, len, "%-s/%-s/%-s.%-s", getConfigDir(),  // JMC - backport 6404
              LOCK_FILE_DIR, tmpPath, LOCK_FILE_TRAILER );

    return 0;
}


int
executeFilesystemLockCommand( int cmd, int type, int fd, struct flock * lock ) {
#ifndef _WIN32
    bzero( lock, sizeof( *lock ) );
    lock->l_type = type;
    lock->l_whence = SEEK_SET;
    int status = fcntl( fd, cmd, lock );
    if ( status < 0 ) {
        /* this is not necessary an error. cmd F_SETLK or F_GETLK can return
         * -1. */
        status = SYS_FS_LOCK_ERR - errno;
        close( fd );
        return status;
    }
    return 0;
#endif
}

/* fsDataObjLock - lock the data object using the local file system
 * Input:
 *    char *objPath - The full Object path
 *    int cmd - the fcntl cmd - valid values are F_SETLK, F_SETLKW and F_GETLK
 *    int type - the fcntl type - valid values are F_UNLCK, F_WRLCK, F_RDLCK
 */
int
fsDataObjLock( char *objPath, int cmd, int type ) {
    int status;
    int fd;

    char *path = NULL;
    if ( ( status = getDataObjLockPath( objPath, &path ) ) < 0 ) {
        rodsLogError( LOG_ERROR, status,
                      "fsDataObjLock: getDataObjLockPath error for %s", objPath );
        free( path );
        return status;
    }
    fd = open( path, O_RDWR | O_CREAT, 0644 );
    free( path );
    if ( fd < 0 ) {
        status = FILE_OPEN_ERR - errno;
        rodsLogError( LOG_ERROR, status,
                      "fsDataObjLock: open error for %s", objPath );
        return status;
    }
    struct flock lock;
    status = executeFilesystemLockCommand( cmd, type, fd, &lock );
    if ( status < 0 ) {
        rodsLogError( LOG_DEBUG, status, "fsDataObjLock: fcntl error for %s, cmd = %d, type = %d",
                      objPath, cmd, type ); // JMC - backport 4604
        return status;
    }
    if ( cmd == F_GETLK ) {
        /* get the status of locking the file. return F_UNLCK if no conflict */
        close( fd );
        return lock.l_type;
    }
    return fd;
}

int
fsDataObjUnlock( int cmd, int type, int fd ) {
    struct flock lock;
    int status = executeFilesystemLockCommand( cmd, type, fd, &lock );
    if ( status < 0 ) {
        rodsLogError( LOG_DEBUG, status, "fsDataObjLock: fcntl error on unlock, status = %d",
                      status ); // JMC - backport 4604
        return status;
    }
    close( fd );
    return 0;
}

rodsLong_t
getFileMetadataFromVault( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo )

{
    static char fname[] = "getFileMetadataFromVault";
    rodsStat_t *myStat = NULL;
    int status;
    rodsLong_t mysize;
    char name_buf[NAME_LEN];
    char sstr_buf[SHORT_STR_LEN];
    char time_buf[TIME_LEN];

    status = l3Stat( rsComm, dataObjInfo, &myStat );

    if ( status < 0 ) {
        rodsLog( LOG_DEBUG,
                 "getFileMetaFromVault: l3Stat error for %s. status = %d",
                 dataObjInfo->filePath, status );
        return status;
    }

    if ( myStat->st_mode & S_IFDIR ) {
        free( myStat );
        return ( rodsLong_t ) SYS_PATH_IS_NOT_A_FILE;
    }

    status = getUnixUsername( myStat->st_uid, name_buf, NAME_LEN );
    if ( status ) {
        rodsLog( LOG_ERROR, "%s: could not retrieve username for uid %d",
                 fname, myStat->st_uid );
        free( myStat );
        return status;
    }
    addKeyVal( &dataObjInfo->condInput, FILE_OWNER_KW, name_buf );

    status = getUnixGroupname( myStat->st_gid, name_buf, NAME_LEN );
    if ( status ) {
        rodsLog( LOG_ERROR, "%s: could not retrieve groupname for gid %d",
                 fname, myStat->st_gid );
        free( myStat );
        return status;
    }
    addKeyVal( &dataObjInfo->condInput, FILE_GROUP_KW, name_buf );

    snprintf( sstr_buf, SHORT_STR_LEN, "%u", myStat->st_uid );
    addKeyVal( &dataObjInfo->condInput, FILE_UID_KW, sstr_buf );

    snprintf( sstr_buf, SHORT_STR_LEN, "%u", myStat->st_gid );
    addKeyVal( &dataObjInfo->condInput, FILE_GID_KW, sstr_buf );

    snprintf( sstr_buf, SHORT_STR_LEN, "%u", myStat->st_mode );
    addKeyVal( &dataObjInfo->condInput, FILE_MODE_KW, sstr_buf );

    snprintf( time_buf, TIME_LEN, "%u", myStat->st_ctim );
    addKeyVal( &dataObjInfo->condInput, FILE_CTIME_KW, time_buf );

    snprintf( time_buf, TIME_LEN, "%u", myStat->st_mtim );
    addKeyVal( &dataObjInfo->condInput, FILE_MTIME_KW, time_buf );
    mysize = myStat->st_size;
    free( myStat );
    return mysize;
}

int
getLeafRescPathName(
    const std::string& _resc_hier,
    std::string& _ret_string ) {
    int result = 0;
    irods::hierarchy_parser hp;
    irods::error ret;
    ret = hp.set_string( _resc_hier );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << "Unable to parse hierarchy string: \"" << _resc_hier << "\"";
        irods::log( LOG_ERROR, msg.str() );
        result = ret.code();
    }
    else {
        std::string leaf;
        ret = hp.last_resc( leaf );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << "Unable to retrieve last resource from hierarchy string: \"" << _resc_hier << "\"";
            irods::log( LOG_ERROR, msg.str() );
            result = ret.code();
        }
        else {
            ret = irods::get_resource_property<std::string>( leaf, irods::RESOURCE_PATH, _ret_string );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << "Unable to get vault path from resource: \"" << leaf << "\"";
                irods::log( LOG_ERROR, msg.str() );
                result = ret.code();
            }
        }
    }
    return result;
}

// =-=-=-=-=-=-=-

