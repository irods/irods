/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See phyPathReg.h for a description of this API call.*/

#include "reFuncDefs.hpp"
#include "fileStat.hpp"
#include "phyPathReg.hpp"
#include "rodsLog.hpp"
#include "icatDefines.hpp"
#include "objMetaOpr.hpp"
#include "dataObjOpr.hpp"
#include "collection.hpp"
#include "specColl.hpp"
#include "resource.hpp"
#include "physPath.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.hpp"
#include "reGlobalsExtern.hpp"
#include "miscServerFunct.hpp"
#include "apiHeaderAll.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_hierarchy_parser.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>

/* holds a struct that describes pathname match patterns
   to exclude from registration. Needs to be global due
   to the recursive dirPathReg() calls. */
static pathnamePatterns_t *ExcludePatterns = NULL;

/* function to read pattern file from a data server */
pathnamePatterns_t *
readPathnamePatternsFromFile( rsComm_t *rsComm, char *filename, char* );


/* phyPathRegNoChkPerm - Wrapper internal function to allow phyPathReg with
 * no checking for path Perm.
 */
int
phyPathRegNoChkPerm( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp ) {
    int status;

    addKeyVal( &phyPathRegInp->condInput, NO_CHK_FILE_PERM_KW, "" );

    status = irsPhyPathReg( rsComm, phyPathRegInp );
    return status;
}

int
rsPhyPathReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp ) {
    int status;

    if ( getValByKey( &phyPathRegInp->condInput, NO_CHK_FILE_PERM_KW ) != NULL &&
            rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return SYS_NO_API_PRIV;
    }

    status = irsPhyPathReg( rsComm, phyPathRegInp );
    return status;
}

int
irsPhyPathReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp ) {
    int status;
    rodsServerHost_t *rodsServerHost = NULL;
    int remoteFlag;
    rodsHostAddr_t addr;


    // =-=-=-=-=-=-=-
    // NOTE:: resource_redirect can wipe out the specColl due to a call to getDataObjIncSpecColl
    //        which nullifies the specColl in the case of certain special collections ( LINKED ).
    //        this block of code needs to be called before redirect to handle that case as it doesnt
    //        need the resource hierarchy anyway.  this is the sort of thing i'd like to avoid in
    //        the future

    char* coll_type = getValByKey( &phyPathRegInp->condInput, COLLECTION_TYPE_KW );

    if ( coll_type && strcmp( coll_type, UNMOUNT_STR ) == 0 ) {
        status = unmountFileDir( rsComm, phyPathRegInp );
        return status;

    }
    else  if ( coll_type != NULL && strcmp( coll_type, LINK_POINT_STR ) == 0 ) {
        status = linkCollReg( rsComm, phyPathRegInp );
        return status;

    }

    // =-=-=-=-=-=-=-
    // determine if a hierarchy has been passed by kvp, if so use it.
    // otherwise determine a resource hierarchy given dst hier or default resource
    std::string hier;
    char*       tmp_hier = getValByKey( &phyPathRegInp->condInput, RESC_HIER_STR_KW );
    if ( NULL == tmp_hier ) {
        // =-=-=-=-=-=-=-
        // if no hier is provided, determine if a resource was specified
        char* dst_resc = getValByKey( &phyPathRegInp->condInput, DEST_RESC_NAME_KW );
        if ( dst_resc ) {
            // =-=-=-=-=-=-=-
            // if we do have a dest resc, see if it has a parent or a child
            irods::resource_ptr resc;
            irods::error ret = resc_mgr.resolve( dst_resc, resc );
            if ( !ret.ok() ) {
                irods::log( PASS( ret ) );
                return ret.code();
            }

            // =-=-=-=-=-=-=-
            // get parent
            irods::resource_ptr parent_resc;
            ret = resc->get_parent( parent_resc );
            bool has_parent = ret.ok();

            // =-=-=-=-=-=-=-
            // get child
            bool has_child = ( resc->num_children() > 0 );

            // =-=-=-=-=-=-=-
            // if the resc is mid-tier this is a Bad Thing
            if ( has_parent && has_child ) {
                return HIERARCHY_ERROR;
            }
            // =-=-=-=-=-=-=-
            // this is a leaf node situation
            else if ( has_parent && !has_child ) {
                // =-=-=-=-=-=-=-
                // get the path from our parent resource
                // to this given leaf resource - this our hier
                getHierarchyForRescOut_t* get_hier_out = 0;
                getHierarchyForRescInp_t  get_hier_inp;
                snprintf( get_hier_inp.resc_name_, sizeof( get_hier_inp.resc_name_ ), "%s", dst_resc );
                status = rsGetHierarchyForResc(
                             rsComm,
                             &get_hier_inp,
                             &get_hier_out );
                if ( status < 0 ) {
                    irods::log( ERROR( status, "failed to get resc hier" ) );
                    free( get_hier_out );
                    return status;
                }

                hier = get_hier_out->resc_hier_;
                addKeyVal(
                    &phyPathRegInp->condInput,
                    RESC_HIER_STR_KW,
                    hier.c_str() );

                // =-=-=-=-=-=-=-
                // get the root resc of the hier, this is the
                // new resource name
                std::string root_resc;
                irods::hierarchy_parser parser;
                parser.set_string( get_hier_out->resc_hier_ );
                parser.first_resc( root_resc );
                addKeyVal(
                    &phyPathRegInp->condInput,
                    DEST_RESC_NAME_KW,
                    root_resc.c_str() );
                free( get_hier_out );

            }
            // =-=-=-=-=-=-=-
            // this is a solo node situation
            else if ( !has_parent && !has_child ) {
                hier = dst_resc;
                addKeyVal(
                    &phyPathRegInp->condInput,
                    RESC_HIER_STR_KW,
                    hier.c_str() );
            }
            // =-=-=-=-=-=-=-
            // root node and pathological situation
            else {
                irods::error ret = irods::resolve_resource_hierarchy(
                                       irods::CREATE_OPERATION,
                                       rsComm,
                                       phyPathRegInp,
                                       hier );
                if ( !ret.ok() ) {
                    std::string msg( "failed for [" );
                    msg += phyPathRegInp->objPath;
                    msg += "]";
                    irods::log( PASSMSG( msg, ret ) );
                    return ret.code();
                }

                addKeyVal(
                    &phyPathRegInp->condInput,
                    RESC_HIER_STR_KW,
                    hier.c_str() );
            }

        } // if dst_resc
        else {
            // =-=-=-=-=-=-=-
            // no resc is specificied, request a hierarchy given the default resource
            irods::error ret = irods::resolve_resource_hierarchy(
                                   irods::CREATE_OPERATION,
                                   rsComm,
                                   phyPathRegInp,
                                   hier );
            if ( !ret.ok() ) {
                std::string msg( "failed for [" );
                msg += phyPathRegInp->objPath;
                msg += "]";
                irods::log( PASSMSG( msg, ret ) );
                return ret.code();
            }

            // =-=-=-=-=-=-=-
            // we resolved the redirect and have a host, set the hier str for subsequent
            // api calls, etc.
            addKeyVal( &phyPathRegInp->condInput, RESC_HIER_STR_KW, hier.c_str() );

        } // else

    } // if tmp_hier
    // =-=-=-=-=-=-=-
    // we have been handed a hierarchy, pass it on
    else {
        hier = tmp_hier;
    }

    // =-=-=-=-=-=-=-
    // coll registration requires the resource hierarchy
    if ( coll_type && ( strcmp( coll_type, HAAW_STRUCT_FILE_STR ) == 0 ||
                        strcmp( coll_type, TAR_STRUCT_FILE_STR ) == 0 ||
                        strcmp( coll_type, MSSO_STRUCT_FILE_STR ) == 0 ) ) {
        status = structFileReg( rsComm, phyPathRegInp );
        return status;
    }

    // =-=-=-=-=-=-=-
    irods::hierarchy_parser parser;
    parser.set_string( hier );
    std::string resc_name;
    parser.first_resc( resc_name );

    std::string last_resc;
    parser.last_resc( last_resc );

    std::string location;
    irods::error ret = irods::get_resource_property< std::string >(
                           last_resc,
                           irods::RESOURCE_LOCATION,
                           location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "failed in get_resource_property", ret ) );
        return -1;
    }

    memset( &addr, 0, sizeof( addr ) );
    rstrcpy( addr.hostAddr, location.c_str(), LONG_NAME_LEN );
    remoteFlag = resolveHost( &addr, &rodsServerHost );

    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsPhyPathReg( rsComm, phyPathRegInp, resc_name.c_str(), rodsServerHost );

    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remotePhyPathReg( rsComm, phyPathRegInp, rodsServerHost );

    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_ERROR,
                     "rsPhyPathReg: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    return status;
}

int
remotePhyPathReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp,
                  rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_ERROR,
                 "remotePhyPathReg: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    status = rcPhyPathReg( rodsServerHost->conn, phyPathRegInp );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "remotePhyPathReg: rcPhyPathReg failed for %s",
                 phyPathRegInp->objPath );
    }

    return status;
}

int
_rsPhyPathReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp,
               const char *_resc_name, rodsServerHost_t *rodsServerHost ) {
    int status = 0;
    fileOpenInp_t chkNVPathPermInp;
    char *tmpFilePath = 0;
    char filePath[MAX_NAME_LEN];
    dataObjInfo_t dataObjInfo;
    char *tmpStr = NULL;
    int chkType = 0; // JMC - backport 4774
    char *excludePatternFile = 0;

    if ( ( tmpFilePath = getValByKey( &phyPathRegInp->condInput, FILE_PATH_KW ) ) == NULL ) {
        rodsLog( LOG_ERROR, "_rsPhyPathReg: No filePath input for %s",
                 phyPathRegInp->objPath );
        return SYS_INVALID_FILE_PATH;
    }
    else {
        /* have to do this since it will be over written later */
        rstrcpy( filePath, tmpFilePath, MAX_NAME_LEN );
    }

    /* check if we need to chk permission */
    memset( &dataObjInfo, 0, sizeof( dataObjInfo ) );
    rstrcpy( dataObjInfo.objPath, phyPathRegInp->objPath, MAX_NAME_LEN );
    rstrcpy( dataObjInfo.filePath, filePath, MAX_NAME_LEN );
    rstrcpy( dataObjInfo.rescName, _resc_name, NAME_LEN );

    char* resc_hier = getValByKey( &phyPathRegInp->condInput, RESC_HIER_STR_KW );
    if ( resc_hier ) {
        rstrcpy( dataObjInfo.rescHier, resc_hier, MAX_NAME_LEN );
    }
    else {
        rodsLog( LOG_NOTICE, "_rsPhyPathReg :: RESC_HIER_STR_KW is NULL" );
        return -1;
    }


    if ( getValByKey( &phyPathRegInp->condInput, NO_CHK_FILE_PERM_KW ) == NULL &&
            ( chkType = getchkPathPerm( rsComm, phyPathRegInp, &dataObjInfo ) ) != NO_CHK_PATH_PERM ) { // JMC - backport 4774

        memset( &chkNVPathPermInp, 0, sizeof( chkNVPathPermInp ) );

        rstrcpy( chkNVPathPermInp.fileName, filePath, MAX_NAME_LEN );

        // =-=-=-=-=-=-=-
        // extract the host location from the resource hierarchy
        std::string location;
        irods::error ret = irods::get_loc_for_hier_string( resc_hier, location );
        if ( !ret.ok() ) {
            irods::log( PASSMSG( "_rsPhyPathReg - failed in get_loc_for_hier_string", ret ) );
            return -1;
        }

        rstrcpy( chkNVPathPermInp.addr.hostAddr, location.c_str(), NAME_LEN );
        status = chkFilePathPerm( rsComm, &chkNVPathPermInp, rodsServerHost, chkType ); // JMC - backport 4774

        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "_rsPhyPathReg: chkFilePathPerm error for %s",
                     phyPathRegInp->objPath );
            return status;
        }
    }
    else {
        status = 0;
    }

    if ( getValByKey( &phyPathRegInp->condInput, COLLECTION_KW ) != NULL ) {
        excludePatternFile = getValByKey( &phyPathRegInp->condInput, EXCLUDE_FILE_KW );
        if ( excludePatternFile != NULL ) {
            ExcludePatterns = readPathnamePatternsFromFile( rsComm,
                              excludePatternFile,
                              resc_hier );
        }

        status = dirPathReg( rsComm, phyPathRegInp, filePath, _resc_name );
        if ( excludePatternFile != NULL ) {
            freePathnamePatterns( ExcludePatterns );
            ExcludePatterns = NULL;
        }

    }
    else if ( ( tmpStr = getValByKey( &phyPathRegInp->condInput, COLLECTION_TYPE_KW ) ) != NULL && strcmp( tmpStr, MOUNT_POINT_STR ) == 0 ) {

        // Get resource path
        std::string resc_vault_path;
        irods::error ret = irods::get_resource_property< std::string >( _resc_name, irods::RESOURCE_PATH, resc_vault_path );
        if ( !ret.ok() ) {
            irods::log PASSMSG( "_rsPhyPathReg - failed in get_resource_property", ret );
            return ret.code();
        }

        status = mountFileDir( rsComm, phyPathRegInp, filePath, resc_vault_path.c_str() );

    }
    else {
        if ( getValByKey( &phyPathRegInp->condInput, REG_REPL_KW ) != NULL ) {
            status = filePathRegRepl( rsComm, phyPathRegInp, filePath, _resc_name );

        }
        else {
            status = filePathReg( rsComm, phyPathRegInp, _resc_name );
        }
    }

    return status;
}

int
filePathRegRepl( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp, char *filePath,
                 const char *_resc_name ) {
    dataObjInfo_t destDataObjInfo, *dataObjInfoHead = NULL;
    regReplica_t regReplicaInp;
    int status;

    status = getDataObjInfo( rsComm, phyPathRegInp, &dataObjInfoHead,
                             ACCESS_READ_OBJECT, 0 );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "filePathRegRepl: getDataObjInfo for %s", phyPathRegInp->objPath );
        return status;
    }

    status = sortObjInfoForOpen( &dataObjInfoHead, &phyPathRegInp->condInput, 0 );
    if ( status < 0 ) {
        // =-=-=-=-=-=-=-
        // we perhaps did not match the hier string but
        // we can still continue as we have a good copy
        // for a read
        if ( NULL == dataObjInfoHead ) {
            return status; // JMC cppcheck - nullptr
        }
    }
    destDataObjInfo = *dataObjInfoHead;
    rstrcpy( destDataObjInfo.filePath, filePath, MAX_NAME_LEN );
    rstrcpy( destDataObjInfo.rescName, _resc_name, NAME_LEN );

    memset( &regReplicaInp, 0, sizeof( regReplicaInp ) );
    regReplicaInp.srcDataObjInfo = dataObjInfoHead;
    regReplicaInp.destDataObjInfo = &destDataObjInfo;
    if ( getValByKey( &phyPathRegInp->condInput, SU_CLIENT_USER_KW ) != NULL ) {
        addKeyVal( &regReplicaInp.condInput, SU_CLIENT_USER_KW, "" );
        addKeyVal( &regReplicaInp.condInput, ADMIN_KW, "" );
    }
    else if ( getValByKey( &phyPathRegInp->condInput,
                           ADMIN_KW ) != NULL ) {
        addKeyVal( &regReplicaInp.condInput, ADMIN_KW, "" );
    }
    status = rsRegReplica( rsComm, &regReplicaInp );
    clearKeyVal( &regReplicaInp.condInput );
    freeAllDataObjInfo( dataObjInfoHead );

    return status;
}

int
filePathReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp, const char *_resc_name ) {
    dataObjInfo_t dataObjInfo;
    memset( &dataObjInfo, 0, sizeof( dataObjInfo ) );

    int status;
    char *chksum = NULL;
    initDataObjInfoWithInp( &dataObjInfo, phyPathRegInp );

    dataObjInfo.replStatus = NEWLY_CREATED_COPY;
    rstrcpy( dataObjInfo.rescName, _resc_name, NAME_LEN );

    char* resc_hier = getValByKey( &phyPathRegInp->condInput, RESC_HIER_STR_KW );
    if ( !resc_hier ) {
        rodsLog( LOG_NOTICE, "filePathReg - RESC_HIER_STR_KW is NULL" );
        return -1;
    }

    rstrcpy( dataObjInfo.rescHier, resc_hier, MAX_NAME_LEN );

    if ( dataObjInfo.dataSize <= 0 &&
            ( dataObjInfo.dataSize = getFileMetadataFromVault( rsComm, &dataObjInfo ) ) < 0 &&
            dataObjInfo.dataSize != UNKNOWN_FILE_SZ ) {
        status = ( int ) dataObjInfo.dataSize;
        rodsLog( LOG_ERROR,
                 "filePathReg: getFileMetadataFromVault for %s failed, status = %d",
                 dataObjInfo.objPath, status );
        return status;
    }

    if ( ( chksum = getValByKey( &phyPathRegInp->condInput,
                                 REG_CHKSUM_KW ) ) != NULL ) {
        rstrcpy( dataObjInfo.chksum, chksum, NAME_LEN );
    }
    else if ( ( chksum = getValByKey( &phyPathRegInp->condInput,
                                      VERIFY_CHKSUM_KW ) ) != NULL ) {
        chksum = 0;
        status = _dataObjChksum( rsComm, &dataObjInfo, &chksum );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "filePathReg: _dataObjChksum for %s failed, status = %d",
                     dataObjInfo.objPath, status );
            return status;
        }
        rstrcpy( dataObjInfo.chksum, chksum, NAME_LEN );
    }

    status = svrRegDataObj( rsComm, &dataObjInfo );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "filePathReg: svrRegDataObj for %s failed, status = %d",
                 dataObjInfo.objPath, status );
    }
    else {
        ruleExecInfo_t rei;
        initReiWithDataObjInp( &rei, rsComm, phyPathRegInp );
        rei.doi = &dataObjInfo;
        rei.status = status;
        rei.status = applyRule( "acPostProcForFilePathReg", NULL, &rei,
                                NO_SAVE_REI );
    }

    return status;
}

int
dirPathReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp, char *filePath,
            const char *_resc_name ) {
    fileStatInp_t fileStatInp;
    collInp_t collCreateInp;
    fileOpendirInp_t fileOpendirInp;
    fileClosedirInp_t fileClosedirInp;
    int status;
    int dirFd;
    dataObjInp_t subPhyPathRegInp;
    fileReaddirInp_t fileReaddirInp;
    rodsDirent_t *rodsDirent = NULL;
    rodsObjStat_t *rodsObjStatOut = NULL;
    int forceFlag;

    char* resc_hier = getValByKey( &phyPathRegInp->condInput, RESC_HIER_STR_KW );
    if ( !resc_hier ) {
        rodsLog( LOG_NOTICE, "dirPathReg - RESC_HIER_STR_KW is NULL" );
        return -1;
    }

    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( resc_hier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "dirPathReg - failed in get_loc_for_hier_string", ret ) );
        return -1;
    }

    status = collStat( rsComm, phyPathRegInp, &rodsObjStatOut );
    if ( status < 0 || NULL == rodsObjStatOut ) { // JMC cppcheck - nullptr
        freeRodsObjStat( rodsObjStatOut );
        rodsObjStatOut = NULL;
        memset( &collCreateInp, 0, sizeof( collCreateInp ) );
        rstrcpy( collCreateInp.collName, phyPathRegInp->objPath,
                 MAX_NAME_LEN );

        /* no need to resolve sym link */ // JMC - backport 4845
        addKeyVal( &collCreateInp.condInput, TRANSLATED_PATH_KW, "" ); // JMC - backport 4845

        /* stat the source directory to track the         */
        /* original directory meta-data                   */
        memset( &fileStatInp, 0, sizeof( fileStatInp ) );
        rstrcpy( fileStatInp.fileName, filePath, MAX_NAME_LEN );

        // Get resource location
        std::string location;
        irods::error ret = irods::get_resource_property< std::string >( _resc_name, irods::RESOURCE_LOCATION, location );
        if ( !ret.ok() ) {
            irods::log PASSMSG( "dirPathReg - failed in get_resource_property", ret );
            return ret.code();
        }

        snprintf( fileStatInp.addr.hostAddr, NAME_LEN, "%s", location.c_str() );


        rstrcpy( fileStatInp.rescHier, resc_hier, MAX_NAME_LEN );
        rstrcpy( fileStatInp.objPath, phyPathRegInp->objPath, MAX_NAME_LEN );

        /* create the coll just in case it does not exist */
        status = rsCollCreate( rsComm, &collCreateInp );
        clearKeyVal( &collCreateInp.condInput ); // JMC - backport 4835
        if ( status < 0 ) {
            return status;
        }
    }
    else if ( rodsObjStatOut->specColl != NULL ) {
        freeRodsObjStat( rodsObjStatOut );
        rodsLog( LOG_ERROR,
                 "mountFileDir: %s already mounted", phyPathRegInp->objPath );
        return SYS_MOUNT_MOUNTED_COLL_ERR;
    }
    freeRodsObjStat( rodsObjStatOut );
    rodsObjStatOut = NULL;

    memset( &fileOpendirInp, 0, sizeof( fileOpendirInp ) );

    rstrcpy( fileOpendirInp.dirName, filePath, MAX_NAME_LEN );
    rstrcpy( fileOpendirInp.addr.hostAddr, location.c_str(), NAME_LEN );
    rstrcpy( fileOpendirInp.objPath,    phyPathRegInp->objPath, MAX_NAME_LEN );
    rstrcpy( fileOpendirInp.resc_hier_, resc_hier,              MAX_NAME_LEN );

    dirFd = rsFileOpendir( rsComm, &fileOpendirInp );
    if ( dirFd < 0 ) {
        rodsLog( LOG_ERROR,
                 "dirPathReg: rsFileOpendir for %s error, status = %d",
                 filePath, dirFd );
        return dirFd;
    }

    fileReaddirInp.fileInx = dirFd;

    if ( getValByKey( &phyPathRegInp->condInput, FORCE_FLAG_KW ) != NULL ) {
        forceFlag = 1;
    }
    else {
        forceFlag = 0;
    }

    while ( ( status = rsFileReaddir( rsComm, &fileReaddirInp, &rodsDirent ) ) >= 0 ) {

        fileStatInp_t fileStatInp;
        rodsStat_t *myStat = NULL;

        if ( strlen( rodsDirent->d_name ) == 0 ) {
            break;
        }

        if ( strcmp( rodsDirent->d_name, "." ) == 0 ||
                strcmp( rodsDirent->d_name, ".." ) == 0 ) {
            free( rodsDirent ); // JMC - backport 4835
            continue;
        }

        subPhyPathRegInp = *phyPathRegInp;
        snprintf( subPhyPathRegInp.objPath, MAX_NAME_LEN, "%s/%s",
                  phyPathRegInp->objPath, rodsDirent->d_name );

        if ( matchPathname( ExcludePatterns, rodsDirent->d_name, filePath ) ) {
            continue;
        }

        memset( &fileStatInp, 0, sizeof( fileStatInp ) );

        snprintf( fileStatInp.fileName, MAX_NAME_LEN, "%s/%s", filePath, rodsDirent->d_name );
        rstrcpy( fileStatInp.objPath, subPhyPathRegInp.objPath, MAX_NAME_LEN );
        fileStatInp.addr = fileOpendirInp.addr;
        rstrcpy( fileStatInp.rescHier, resc_hier, MAX_NAME_LEN );


        status = rsFileStat( rsComm, &fileStatInp, &myStat );

        if ( status != 0 ) {
            rodsLog( LOG_ERROR,
                     "dirPathReg: rsFileStat failed for %s, status = %d",
                     fileStatInp.fileName, status );
            free( rodsDirent ); // JMC - backport 4835
            return status;
        }

        if ( ( myStat->st_mode & S_IFREG ) != 0 ) { /* a file */
            if ( forceFlag > 0 ) {
                /* check if it already exists */
                if ( isData( rsComm, subPhyPathRegInp.objPath, NULL ) >= 0 ) {
                    free( myStat );
                    free( rodsDirent ); // JMC - backport 4835
                    continue;
                }
            }
            subPhyPathRegInp.dataSize = myStat->st_size;
            if ( getValByKey( &phyPathRegInp->condInput, REG_REPL_KW ) != NULL ) {
                status = filePathRegRepl( rsComm, &subPhyPathRegInp,
                                          fileStatInp.fileName, _resc_name );
            }
            else {
                addKeyVal( &subPhyPathRegInp.condInput, FILE_PATH_KW,
                           fileStatInp.fileName );
                status = filePathReg( rsComm, &subPhyPathRegInp, _resc_name );
            }
        }
        else if ( ( myStat->st_mode & S_IFDIR ) != 0 ) {    /* a directory */
            status = dirPathReg( rsComm, &subPhyPathRegInp,
                                 fileStatInp.fileName, _resc_name );
        }
        free( myStat );
        free( rodsDirent ); // JMC - backport 4835
    }

    if ( status == -1 ) {       /* just EOF */
        status = 0;
    }

    fileClosedirInp.fileInx = dirFd;
    rsFileClosedir( rsComm, &fileClosedirInp );

    return status;
}

int mountFileDir( rsComm_t*     rsComm,
                  dataObjInp_t* phyPathRegInp,
                  char*         filePath,
                  const char *rescVaultPath ) {
    collInp_t collCreateInp;
    int status;
    fileStatInp_t fileStatInp;
    rodsStat_t *myStat = NULL;
    rodsObjStat_t *rodsObjStatOut = NULL;

    char* resc_hier = getValByKey( &phyPathRegInp->condInput, RESC_HIER_STR_KW );
    if ( !resc_hier ) {
        rodsLog( LOG_NOTICE, "mountFileDir - RESC_HIER_STR_KW is NULL" );
        return -1;
    }

    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( resc_hier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "mountFileDir - failed in get_loc_for_hier_string", ret ) );
        return -1;
    }

    if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) { // JMC - backport 4832
        rodsLog( LOG_NOTICE, "mountFileDir - insufficient privilege" );
        return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
    }

    status = collStat( rsComm, phyPathRegInp, &rodsObjStatOut );
    if ( status < 0 || NULL == rodsObjStatOut ) {
        freeRodsObjStat( rodsObjStatOut );
        rodsLog( LOG_NOTICE, "mountFileDir collstat failed." );
        return status; // JMC cppcheck - nullptr
    }

    if ( rodsObjStatOut->specColl != NULL ) {
        freeRodsObjStat( rodsObjStatOut );
        rodsLog( LOG_ERROR,
                 "mountFileDir: %s already mounted", phyPathRegInp->objPath );
        return SYS_MOUNT_MOUNTED_COLL_ERR;
    }
    freeRodsObjStat( rodsObjStatOut );
    rodsObjStatOut = NULL;

    if ( isCollEmpty( rsComm, phyPathRegInp->objPath ) == False ) {
        rodsLog( LOG_ERROR,
                 "mountFileDir: collection %s not empty", phyPathRegInp->objPath );
        return SYS_COLLECTION_NOT_EMPTY;
    }

    memset( &fileStatInp, 0, sizeof( fileStatInp ) );

    rstrcpy( fileStatInp.fileName, filePath, MAX_NAME_LEN );
    rstrcpy( fileStatInp.objPath, phyPathRegInp->objPath, MAX_NAME_LEN );
    rstrcpy( fileStatInp.addr.hostAddr,  location.c_str(), NAME_LEN );
    rstrcpy( fileStatInp.rescHier, resc_hier, MAX_NAME_LEN );


    status = rsFileStat( rsComm, &fileStatInp, &myStat );

    if ( status < 0 ) {
        fileMkdirInp_t fileMkdirInp;

        rodsLog( LOG_NOTICE,
                 "mountFileDir: rsFileStat failed for %s, status = %d, create it",
                 fileStatInp.fileName, status );
        memset( &fileMkdirInp, 0, sizeof( fileMkdirInp ) );
        rstrcpy( fileMkdirInp.dirName, filePath, MAX_NAME_LEN );
        rstrcpy( fileMkdirInp.rescHier, resc_hier, MAX_NAME_LEN );
        fileMkdirInp.mode = getDefDirMode();
        rstrcpy( fileMkdirInp.addr.hostAddr,  location.c_str(), NAME_LEN );
        status = rsFileMkdir( rsComm, &fileMkdirInp );
        if ( status < 0 ) {
            return status;
        }
    }
    else if ( ( myStat->st_mode & S_IFDIR ) == 0 ) {
        rodsLog( LOG_ERROR,
                 "mountFileDir: phyPath %s is not a directory",
                 fileStatInp.fileName );
        free( myStat );
        return USER_FILE_DOES_NOT_EXIST;
    }

    free( myStat );
    /* mk the collection */

    memset( &collCreateInp, 0, sizeof( collCreateInp ) );
    rstrcpy( collCreateInp.collName, phyPathRegInp->objPath, MAX_NAME_LEN );
    addKeyVal( &collCreateInp.condInput, COLLECTION_TYPE_KW, MOUNT_POINT_STR );

    addKeyVal( &collCreateInp.condInput, COLLECTION_INFO1_KW, filePath );
    addKeyVal( &collCreateInp.condInput, COLLECTION_INFO2_KW, resc_hier );

    /* try to mod the coll first */
    status = rsModColl( rsComm, &collCreateInp );

    if ( status < 0 ) {	/* try to create it */
        rodsLog( LOG_NOTICE, "mountFileDir rsModColl < 0." );
        status = rsRegColl( rsComm, &collCreateInp );
    }

    if ( status >= 0 ) {
        rodsLog( LOG_NOTICE, "mountFileDir rsModColl > 0." );

        char outLogPath[MAX_NAME_LEN];
        int status1;
        /* see if the phyPath is mapped into a real collection */
        if ( getLogPathFromPhyPath( filePath, rescVaultPath, outLogPath ) >= 0 &&
                strcmp( outLogPath, phyPathRegInp->objPath ) != 0 ) {
            /* log path not the same as input objPath */
            if ( isColl( rsComm, outLogPath, NULL ) >= 0 ) {
                modAccessControlInp_t modAccessControl;
                /* it is a real collection. better set the collection
                 * to read-only mode because any modification to files
                 * through this mounted collection can be trouble */
                bzero( &modAccessControl, sizeof( modAccessControl ) );
                modAccessControl.accessLevel = "read";
                modAccessControl.userName = rsComm->clientUser.userName;
                modAccessControl.zone = rsComm->clientUser.rodsZone;
                modAccessControl.path = phyPathRegInp->objPath;
                status1 = rsModAccessControl( rsComm, &modAccessControl );
                if ( status1 < 0 ) {
                    rodsLog( LOG_NOTICE,
                             "mountFileDir: rsModAccessControl err for %s, stat = %d",
                             phyPathRegInp->objPath, status1 );
                }
            }
        }
    }

    rodsLog( LOG_NOTICE, "mountFileDir return status." );
    return status;
}

int
unmountFileDir( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp ) {
    int status;
    collInp_t modCollInp;
    rodsObjStat_t *rodsObjStatOut = NULL;

    status = collStat( rsComm, phyPathRegInp, &rodsObjStatOut );
    if ( status < 0 || NULL == rodsObjStatOut ) { // JMC cppcheck - nullptr
        free( rodsObjStatOut );
        return status;
    }
    else if ( rodsObjStatOut->specColl == NULL ) {
        freeRodsObjStat( rodsObjStatOut );
        rodsLog( LOG_ERROR,
                 "unmountFileDir: %s not mounted", phyPathRegInp->objPath );
        return SYS_COLL_NOT_MOUNTED_ERR;
    }

    if ( getStructFileType( rodsObjStatOut->specColl ) >= 0 ) {
        status = _rsSyncMountedColl( rsComm, rodsObjStatOut->specColl,
                                     PURGE_STRUCT_FILE_CACHE );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR, "unmountFileDir -  failed in _rsSyncMountedColl with status %d", status );
        }
    }

    freeRodsObjStat( rodsObjStatOut );
    rodsObjStatOut = NULL;

    memset( &modCollInp, 0, sizeof( modCollInp ) );
    rstrcpy( modCollInp.collName, phyPathRegInp->objPath, MAX_NAME_LEN );
    addKeyVal( &modCollInp.condInput, COLLECTION_TYPE_KW,
               "NULL_SPECIAL_VALUE" );
    addKeyVal( &modCollInp.condInput, COLLECTION_INFO1_KW, "NULL_SPECIAL_VALUE" );
    addKeyVal( &modCollInp.condInput, COLLECTION_INFO2_KW, "NULL_SPECIAL_VALUE" );

    status = rsModColl( rsComm, &modCollInp );

    return status;
}

int structFileReg(
    rsComm_t*     rsComm,
    dataObjInp_t* phyPathRegInp ) {
    // =-=-=-=-=-=-=-
    //
    dataObjInp_t     dataObjInp;
    collInp_t        collCreateInp;
    int              status         = 0;
    int              len            = 0;
    char*            collType       = NULL;
    char*            structFilePath = NULL;
    dataObjInfo_t*   dataObjInfo    = NULL;
    rodsObjStat_t*   rodsObjStatOut = NULL;
    specCollCache_t* specCollCache  = NULL;

    if ( ( structFilePath = getValByKey( &phyPathRegInp->condInput, FILE_PATH_KW ) )
            == NULL ) {
        rodsLog( LOG_ERROR,
                 "structFileReg: No structFilePath input for %s",
                 phyPathRegInp->objPath );
        return SYS_INVALID_FILE_PATH;
    }

    collType = getValByKey( &phyPathRegInp->condInput, COLLECTION_TYPE_KW );
    if ( collType == NULL ) {
        rodsLog( LOG_ERROR,
                 "structFileReg: Bad COLLECTION_TYPE_KW for structFilePath %s",
                 dataObjInp.objPath );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    len = strlen( phyPathRegInp->objPath );
    if ( strncmp( structFilePath, phyPathRegInp->objPath, len ) == 0 &&
            ( structFilePath[len] == '\0' || structFilePath[len] == '/' ) ) {
        rodsLog( LOG_ERROR,
                 "structFileReg: structFilePath %s inside collection %s",
                 structFilePath, phyPathRegInp->objPath );
        return SYS_STRUCT_FILE_INMOUNTED_COLL;
    }

    /* see if the struct file is in spec coll */

    if ( getSpecCollCache( rsComm, structFilePath, 0,  &specCollCache ) >= 0 ) {
        rodsLog( LOG_ERROR,
                 "structFileReg: structFilePath %s is in a mounted path",
                 structFilePath );
        return SYS_STRUCT_FILE_INMOUNTED_COLL;
    }

    status = collStat( rsComm, phyPathRegInp, &rodsObjStatOut );
    if ( status < 0 || NULL == rodsObjStatOut ) {
        free( rodsObjStatOut );
        return status;    // JMC cppcheck - nullptr
    }

    if ( rodsObjStatOut->specColl != NULL ) {
        freeRodsObjStat( rodsObjStatOut );
        rodsLog( LOG_ERROR,
                 "structFileReg: %s already mounted", phyPathRegInp->objPath );
        return SYS_MOUNT_MOUNTED_COLL_ERR;
    }

    freeRodsObjStat( rodsObjStatOut );
    rodsObjStatOut = NULL;

    if ( isCollEmpty( rsComm, phyPathRegInp->objPath ) == False ) {
        rodsLog( LOG_ERROR,
                 "structFileReg: collection %s not empty", phyPathRegInp->objPath );
        return SYS_COLLECTION_NOT_EMPTY;
    }

    memset( &dataObjInp, 0, sizeof( dataObjInp ) );
    rstrcpy( dataObjInp.objPath, structFilePath, sizeof( dataObjInp.objPath ) );
    /* user need to have write permission */
    dataObjInp.openFlags = O_WRONLY;
    status = getDataObjInfoIncSpecColl( rsComm, &dataObjInp, &dataObjInfo );
    if ( status < 0 || NULL == dataObjInfo ) { // JMC cppcheck - nullptr
        int myStatus;
        /* try to make one */
        dataObjInp.condInput = phyPathRegInp->condInput;
        /* have to remove FILE_PATH_KW because getFullPathName will use it */
        rmKeyVal( &dataObjInp.condInput, FILE_PATH_KW );
        myStatus = rsDataObjCreate( rsComm, &dataObjInp );
        if ( myStatus < 0 ) {
            rodsLog( LOG_ERROR,
                     "structFileReg: Problem with open/create structFilePath %s, status = %d",
                     dataObjInp.objPath, status );
            return status;
        }
        else {
            openedDataObjInp_t dataObjCloseInp;
            bzero( &dataObjCloseInp, sizeof( dataObjCloseInp ) );
            dataObjCloseInp.l1descInx = myStatus;
            rsDataObjClose( rsComm, &dataObjCloseInp );
        }
    }

    char* tmp_hier = getValByKey( &phyPathRegInp->condInput, RESC_HIER_STR_KW );
    if ( !tmp_hier ) {
        rodsLog( LOG_ERROR, "structFileReg - RESC_HIER_STR_KW is NULL" );
        return -1;
    }
    irods::hierarchy_parser parser;
    parser.set_string( std::string( tmp_hier ) );
    std::string resc_name;
    parser.last_resc( resc_name );

    /* mk the collection */

    memset( &collCreateInp, 0, sizeof( collCreateInp ) );
    rstrcpy( collCreateInp.collName, phyPathRegInp->objPath, MAX_NAME_LEN );
    addKeyVal( &collCreateInp.condInput, COLLECTION_TYPE_KW, collType );
    /* have to use dataObjInp.objPath because structFile path was removed */
    addKeyVal( &collCreateInp.condInput, COLLECTION_INFO1_KW, dataObjInp.objPath );

    /* try to mod the coll first */
    status = rsModColl( rsComm, &collCreateInp );

    if ( status < 0 ) { /* try to create it */
        status = rsRegColl( rsComm, &collCreateInp );
    }

    return status;
}

int
structFileSupport( rsComm_t *rsComm, char *collection, char *collType,
                   char* resc_hier ) {
    int status;
    subFile_t subFile;
    specColl_t specColl;

    if ( rsComm == NULL || collection == NULL || collType == NULL ||
            resc_hier == NULL ) {
        return 0;
    }

    memset( &subFile, 0, sizeof( subFile ) );
    memset( &specColl, 0, sizeof( specColl ) );
    /* put in some fake path */
    subFile.specColl = &specColl;
    rstrcpy( specColl.collection, collection, MAX_NAME_LEN );
    specColl.collClass = STRUCT_FILE_COLL;
    if ( strcmp( collType, HAAW_STRUCT_FILE_STR ) == 0 ) {
        specColl.type = HAAW_STRUCT_FILE_T;
    }
    else if ( strcmp( collType, TAR_STRUCT_FILE_STR ) == 0 ) {
        specColl.type = TAR_STRUCT_FILE_T;
    }
    else if ( strcmp( collType, MSSO_STRUCT_FILE_STR ) == 0 ) {
        specColl.type = MSSO_STRUCT_FILE_T;
    }
    else {
        return 0;
    }

    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( resc_hier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "structFileSupport - failed in get_loc_for_hier_string", ret ) );
        return -1;
    }

    irods::hierarchy_parser parser;
    parser.set_string( resc_hier );
    std::string first_resc;
    parser.first_resc( first_resc );

    snprintf( specColl.objPath, MAX_NAME_LEN, "%s/myFakeFile", collection );
    rstrcpy( specColl.resource, first_resc.c_str(), NAME_LEN );
    rstrcpy( specColl.rescHier, resc_hier, MAX_NAME_LEN );
    rstrcpy( specColl.phyPath, "/fakeDir1/fakeDir2/myFakeStructFile", MAX_NAME_LEN );
    rstrcpy( subFile.subFilePath, "/fakeDir1/fakeDir2/myFakeFile", MAX_NAME_LEN );
    rstrcpy( subFile.addr.hostAddr, location.c_str(), NAME_LEN );

    rodsStat_t *myStat = NULL;
    status = rsSubStructFileStat( rsComm, &subFile, &myStat );
    free( myStat );
    return status != SYS_NOT_SUPPORTED;
}

int
linkCollReg( rsComm_t *rsComm, dataObjInp_t *phyPathRegInp ) {
    collInp_t collCreateInp;
    int status;
    char *linkPath = NULL;
    char *collType;
    int len;
    rodsObjStat_t *rodsObjStatOut = NULL;
    specCollCache_t *specCollCache = NULL;

    if ( ( linkPath = getValByKey( &phyPathRegInp->condInput, FILE_PATH_KW ) )
            == NULL ) {
        rodsLog( LOG_ERROR,
                 "linkCollReg: No linkPath input for %s",
                 phyPathRegInp->objPath );
        return SYS_INVALID_FILE_PATH;
    }

    collType = getValByKey( &phyPathRegInp->condInput, COLLECTION_TYPE_KW );
    if ( collType == NULL || strcmp( collType, LINK_POINT_STR ) != 0 ) {
        rodsLog( LOG_ERROR,
                 "linkCollReg: Bad COLLECTION_TYPE_KW for linkPath %s",
                 phyPathRegInp->objPath );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( phyPathRegInp->objPath[0] != '/' || linkPath[0] != '/' ) {
        rodsLog( LOG_ERROR,
                 "linkCollReg: linkPath %s or collection %s not absolute path",
                 linkPath, phyPathRegInp->objPath );
        return SYS_COLL_LINK_PATH_ERR;
    }

    len = strlen( phyPathRegInp->objPath );
    if ( strncmp( linkPath, phyPathRegInp->objPath, len ) == 0 &&
            linkPath[len] == '/' ) {
        rodsLog( LOG_ERROR,
                 "linkCollReg: linkPath %s inside collection %s",
                 linkPath, phyPathRegInp->objPath );
        return SYS_COLL_LINK_PATH_ERR;
    }

    len = strlen( linkPath );
    if ( strncmp( phyPathRegInp->objPath, linkPath, len ) == 0 &&
            phyPathRegInp->objPath[len] == '/' ) {
        rodsLog( LOG_ERROR,
                 "linkCollReg: collection %s inside linkPath %s",
                 linkPath, phyPathRegInp->objPath );
        return SYS_COLL_LINK_PATH_ERR;
    }

    if ( getSpecCollCache( rsComm, linkPath, 0,  &specCollCache ) >= 0 &&
            specCollCache->specColl.collClass != LINKED_COLL ) {
        rodsLog( LOG_ERROR,
                 "linkCollReg: linkPath %s is in a spec coll path",
                 linkPath );
        return SYS_COLL_LINK_PATH_ERR;
    }

    status = collStat( rsComm, phyPathRegInp, &rodsObjStatOut );
    if ( status < 0 ) {
        freeRodsObjStat( rodsObjStatOut );
        rodsObjStatOut = NULL;
        /* does not exist. make one */
        collInp_t collCreateInp;
        memset( &collCreateInp, 0, sizeof( collCreateInp ) );
        rstrcpy( collCreateInp.collName, phyPathRegInp->objPath, MAX_NAME_LEN );
        status = rsRegColl( rsComm, &collCreateInp );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "linkCollReg: rsRegColl error for  %s, status = %d",
                     collCreateInp.collName, status );
            return status;
        }
        status = collStat( rsComm, phyPathRegInp, &rodsObjStatOut );
        if ( status < 0 ) {
            freeRodsObjStat( rodsObjStatOut );
            return status;
        }

    }

    if ( rodsObjStatOut && // JMC cppcheck - nullptr
            rodsObjStatOut->specColl != NULL &&
            rodsObjStatOut->specColl->collClass != LINKED_COLL ) {
        freeRodsObjStat( rodsObjStatOut );
        rodsLog( LOG_ERROR,
                 "linkCollReg: link collection %s in a spec coll path",
                 phyPathRegInp->objPath );
        return SYS_COLL_LINK_PATH_ERR;
    }

    freeRodsObjStat( rodsObjStatOut );
    rodsObjStatOut = NULL;

    if ( isCollEmpty( rsComm, phyPathRegInp->objPath ) == False ) {
        rodsLog( LOG_ERROR,
                 "linkCollReg: collection %s not empty", phyPathRegInp->objPath );
        return SYS_COLLECTION_NOT_EMPTY;
    }

    /* mk the collection */

    memset( &collCreateInp, 0, sizeof( collCreateInp ) );
    rstrcpy( collCreateInp.collName, phyPathRegInp->objPath, MAX_NAME_LEN );
    addKeyVal( &collCreateInp.condInput, COLLECTION_TYPE_KW, collType );

    /* have to use dataObjInp.objPath because structFile path was removed */
    addKeyVal( &collCreateInp.condInput, COLLECTION_INFO1_KW,
               linkPath );

    /* try to mod the coll first */
    status = rsModColl( rsComm, &collCreateInp );

    if ( status < 0 ) { /* try to create it */
        status = rsRegColl( rsComm, &collCreateInp );
    }

    return status;
}

pathnamePatterns_t *
readPathnamePatternsFromFile( rsComm_t *rsComm, char *filename, char* resc_hier ) {
    int status;
    rodsStat_t *stbuf;
    fileStatInp_t fileStatInp;
    bytesBuf_t fileReadBuf;
    fileOpenInp_t fileOpenInp;
    fileReadInp_t fileReadInp;
    fileCloseInp_t fileCloseInp;
    int buf_len, fd;
    pathnamePatterns_t *pp;

    if ( rsComm == NULL || filename == NULL || resc_hier == NULL ) {
        return NULL;
    }


    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( resc_hier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "readPathnamePatternsFromFile - failed in get_loc_for_hier_string", ret ) );
        return NULL;
    }

    memset( &fileStatInp, 0, sizeof( fileStatInp ) );
    rstrcpy( fileStatInp.fileName, filename, MAX_NAME_LEN );
    rstrcpy( fileStatInp.addr.hostAddr, location.c_str(), NAME_LEN );
    status = rsFileStat( rsComm, &fileStatInp, &stbuf );
    if ( status != 0 ) {
        if ( status != UNIX_FILE_STAT_ERR - ENOENT ) {
            rodsLog( LOG_DEBUG, "readPathnamePatternsFromFile: can't stat %s. status = %d",
                     fileStatInp.fileName, status );
        }
        return NULL;
    }
    buf_len = stbuf->st_size;
    free( stbuf );

    memset( &fileOpenInp, 0, sizeof( fileOpenInp ) );
    rstrcpy( fileOpenInp.fileName, filename, MAX_NAME_LEN );
    rstrcpy( fileOpenInp.addr.hostAddr, location.c_str(), NAME_LEN );
    fileOpenInp.flags = O_RDONLY;
    fd = rsFileOpen( rsComm, &fileOpenInp );
    if ( fd < 0 ) {
        rodsLog( LOG_NOTICE,
                 "readPathnamePatternsFromFile: can't open %s for reading. status = %d",
                 fileOpenInp.fileName, fd );
        return NULL;
    }

    memset( &fileReadBuf, 0, sizeof( fileReadBuf ) );
    fileReadBuf.buf = malloc( buf_len );
    if ( fileReadBuf.buf == NULL ) {
        rodsLog( LOG_NOTICE, "readPathnamePatternsFromFile: could not malloc buffer" );
        return NULL;
    }

    memset( &fileReadInp, 0, sizeof( fileReadInp ) );
    fileReadInp.fileInx = fd;
    fileReadInp.len = buf_len;
    status = rsFileRead( rsComm, &fileReadInp, &fileReadBuf );

    memset( &fileCloseInp, 0, sizeof( fileCloseInp ) );
    fileCloseInp.fileInx = fd;
    rsFileClose( rsComm, &fileCloseInp );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE, "readPathnamePatternsFromFile: could not read %s. status = %d",
                 fileOpenInp.fileName, status );
        free( fileReadBuf.buf );
        return NULL;
    }

    pp = readPathnamePatterns( ( char* )fileReadBuf.buf, buf_len );

    return pp;
}


