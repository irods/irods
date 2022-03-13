#include "irods/dataObjRename.h"
#include "irods/objMetaOpr.hpp"
#include "irods/dataObjOpr.hpp"
#include "irods/collection.hpp"
#include "irods/rcMisc.h"
#include "irods/resource.hpp"
#include "irods/rodsErrorTable.h"
#include "irods/specColl.hpp"
#include "irods/physPath.hpp"
#include "irods/subStructFileRename.h"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/dataObjUnlink.h"
#include "irods/dataObjClose.h"
#include "irods/regDataObj.h"
#include "irods/fileOpendir.h"
#include "irods/fileReaddir.h"
#include "irods/fileClosedir.h"
#include "irods/rmColl.h"
#include "irods/miscServerFunct.hpp"
#include "irods/rsDataObjRename.hpp"
#include "irods/rsDataObjUnlink.hpp"
#include "irods/rsSubStructFileRename.hpp"
#include "irods/rsFileRename.hpp"
#include "irods/rsRegDataObj.hpp"
#include "irods/rsQuerySpecColl.hpp"
#include "irods/rsFileReaddir.hpp"
#include "irods/rsDataObjClose.hpp"
#include "irods/rsRmColl.hpp"
#include "irods/rsObjStat.hpp"
#include "irods/irods_resource_backport.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/irods_resource_redirect.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/irods_logger.hpp"
#include "irods/key_value_proxy.hpp"
#include "irods/scoped_privileged_client.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "irods/filesystem.hpp"

#include "irods/logical_locking.hpp"

#include <fmt/format.h>

#include <cstring>
#include <chrono>

using logger = irods::experimental::log;

namespace
{
    namespace fs = irods::experimental::filesystem;
    namespace ill = irods::logical_locking;

    int _rsDataObjRename(
            rsComm_t *rsComm,
            dataObjCopyInp_t *dataObjRenameInp) {
        std::string svc_role{};
        irods::error ret = get_catalog_service_role(svc_role);
        if(!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }

        if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
            return SYS_NO_ICAT_SERVER_ERR;
        }
        else if( irods::CFG_SERVICE_ROLE_PROVIDER != svc_role ) {
            rodsLog(LOG_ERROR, "%s: role not supported [%s]",
                    __FUNCTION__, svc_role.c_str() );
            return SYS_SERVICE_ROLE_NOT_SUPPORTED;
        }

        if (!rsComm) {
            rodsLog( LOG_ERROR, "%s: rsComm is null", __FUNCTION__ );
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }
        int status;
        char srcColl[MAX_NAME_LEN], srcObj[MAX_NAME_LEN];
        char destColl[MAX_NAME_LEN], destObj[MAX_NAME_LEN];
        dataObjInp_t *srcDataObjInp, *destDataObjInp;
        dataObjInfo_t *dataObjInfoHead = NULL;
        rodsLong_t srcId, destId;
        int acPreProcFromRenameFlag = 0;

        const char *args[MAX_NUM_OF_ARGS_IN_ACTION];
        int i, argc;
        ruleExecInfo_t rei2{};
        rei2.rsComm = rsComm;
        rei2.uoic = &rsComm->clientUser;
        rei2.uoip = &rsComm->proxyUser;
        rei2.doinp = &dataObjRenameInp->srcDataObjInp;

        srcDataObjInp = &dataObjRenameInp->srcDataObjInp;
        destDataObjInp = &dataObjRenameInp->destDataObjInp;
        auto source_cond_input = irods::experimental::make_key_value_proxy(srcDataObjInp->condInput);
        auto destination_cond_input = irods::experimental::make_key_value_proxy(destDataObjInp->condInput);

        // Separate source collection/object names and destination collection/object names
        if ( ( status = splitPathByKey(
                        srcDataObjInp->objPath, srcColl, MAX_NAME_LEN, srcObj, MAX_NAME_LEN, '/' ) ) < 0 ) {
            rodsLog( LOG_ERROR, "%s: splitPathByKey for %s error, status = %d",
                    __FUNCTION__, srcDataObjInp->objPath, status );
            return status;
        }
        if ( ( status = splitPathByKey(
                        destDataObjInp->objPath, destColl, MAX_NAME_LEN, destObj, MAX_NAME_LEN, '/' ) ) < 0 ) {
            rodsLog( LOG_ERROR, "%s: splitPathByKey for %s error, status = %d",
                    __FUNCTION__, destDataObjInp->objPath, status );
            return status;
        }

        if ( srcDataObjInp->oprType == RENAME_DATA_OBJ ) {
            status = getDataObjInfo( rsComm, srcDataObjInp, &dataObjInfoHead, ACCESS_DELETE_OBJECT, 0 );
            if ( status >= 0 || NULL != dataObjInfoHead ) {
                srcId = dataObjInfoHead->dataId;
            }
            else {
                rodsLog( LOG_ERROR,
                        "%s: src data %s does not exist, status = %d",
                        __FUNCTION__, srcDataObjInp->objPath, status );
                return status;
            }
        }
        else if ( srcDataObjInp->oprType == RENAME_COLL ) {
            status = isColl( rsComm, srcDataObjInp->objPath, &srcId );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                        "%s: src coll %s does not exist, status = %d",
                        __FUNCTION__, srcDataObjInp->objPath, status );
                return status;
            }
        }
        else {
            if ( ( status = isData( rsComm, srcDataObjInp->objPath, &srcId ) ) >= 0 ) {
                srcDataObjInp->oprType = destDataObjInp->oprType = RENAME_DATA_OBJ;
                status = getDataObjInfo( rsComm, srcDataObjInp, &dataObjInfoHead, ACCESS_DELETE_OBJECT, 0 );
                if ( status < 0 ) {
                    rodsLog( LOG_ERROR,
                            "%s: src data %s does not exist, status = %d",
                            __FUNCTION__, srcDataObjInp->objPath, status );
                    return status;
                }

                if (const auto ret = ill::try_lock(*dataObjInfoHead, ill::lock_type::write); ret < 0) {
                    const auto msg = fmt::format(
                        "rename not allowed because source data object is locked "
                        "[error code=[{}], source path=[{}]",
                        ret, srcDataObjInp->objPath);

                    irods::log(LOG_NOTICE, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, msg));

                    return ret;
                }

                const auto force_flag_provided = source_cond_input.contains(FORCE_FLAG_KW) || destination_cond_input.contains(FORCE_FLAG_KW);
                if (isData(rsComm, destDataObjInp->objPath, &destId) >= 0 && force_flag_provided) {
                    // The force flag ensures that unlink will not send the object to the trash.
                    destination_cond_input[FORCE_FLAG_KW] = "";

                    if (const auto ec = rsDataObjUnlink(rsComm, destDataObjInp); ec < 0) {
                        const auto msg = fmt::format(
                            "failed to unlink destination data object for overwrite via rename "
                            "[error code=[{}], source path=[{}], destination path=[{}]]",
                            ec, srcDataObjInp->objPath, destDataObjInp->objPath);

                        irods::log(LOG_NOTICE, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, msg));

                        return ec;
                    }
                }
            }
            else if ( ( status = isColl( rsComm, srcDataObjInp->objPath, &srcId ) ) >= 0 ) {
                srcDataObjInp->oprType = destDataObjInp->oprType = RENAME_COLL;
            }
            else {
                rodsLog( LOG_ERROR,
                        "%s: src obj %s does not exist, status = %d",
                        __FUNCTION__, srcDataObjInp->objPath, status );
                return status;
            }
        }

        if (srcDataObjInp->oprType == RENAME_DATA_OBJ &&
            strstr(dataObjInfoHead->dataType, BUNDLE_STR)) { // JMC - backport 4658
            rodsLog(LOG_ERROR, "%s: cannot rename tar bundle type obj %s",
                    __FUNCTION__, srcDataObjInp->objPath );
            return CANT_RM_MV_BUNDLE_TYPE;
        }

        // If srcObj is different from destObj, this is a data obj rename...
        if ( strcmp( srcObj, destObj ) != 0 ) {
            if ( srcId < 0 ) {
                status = srcId;
                return status;
            }

            /**  June 1 2009 for pre-post processing rule hooks **/
            argc    = 2;
            args[0] = srcDataObjInp->objPath;
            args[1] = destDataObjInp->objPath;
            acPreProcFromRenameFlag = 1;
            i =  applyRuleArg( "acPreProcForObjRename", args, argc, &rei2, NO_SAVE_REI );
            if ( i < 0 ) {
                if ( rei2.status < 0 ) {
                    i = rei2.status;
                }
                rodsLog( LOG_ERROR,
                        "%s: acPreProcForObjRename error for source %s and destination %s,stat=%d",
                        __FUNCTION__, args[0], args[1], i );
                return i;
            }
            /**  June 1 2009 for pre-post processing rule hooks **/

            status = chlRenameObject( rsComm, srcId, destObj );
        }

        if ( status < 0 ) {
            return status;
        }

        // If srcColl is different from destColl, this is a collection rename...
        if ( strcmp( srcColl, destColl ) != 0 ) {
            status = isColl( rsComm, destColl, &destId );
            if (status < 0) {
                logger::api::error("%s: Destination collection does not exist [collection = {}, error_code = {}]",
                                   __FUNCTION__, destColl, status);
                return CAT_UNKNOWN_COLLECTION;
            }

            /**  June 1 2009 for pre-post processing rule hooks **/
            if ( acPreProcFromRenameFlag == 0 ) {
                args[0] = srcDataObjInp->objPath;
                args[1] = destDataObjInp->objPath;
                argc = 2;
                i =  applyRuleArg( "acPreProcForObjRename", args, argc, &rei2, NO_SAVE_REI );
                if ( i < 0 ) {
                    if ( rei2.status < 0 ) {
                        i = rei2.status;
                    }
                    rodsLog( LOG_ERROR,
                            "rsDataObjRename: acPreProcForObjRename error for source %s and destination %s,stat=%d",
                            args[0], args[1], i );
                    return i;
                }
            }
            /**  June 1 2009 for pre-post processing rule hooks **/

            status = chlMoveObject( rsComm, srcId, destId );
        }

        if ( status >= 0 ) {
            /* enforce physPath consistency */
            if ( srcDataObjInp->oprType == RENAME_DATA_OBJ ) {
                /* update src dataObjInfoHead with dest objPath */
                dataObjInfo_t* tmpDataObjInfo = dataObjInfoHead;
                while ( tmpDataObjInfo ) {
                    rstrcpy( tmpDataObjInfo->objPath, destDataObjInp->objPath, MAX_NAME_LEN );
                    tmpDataObjInfo = tmpDataObjInfo->next;
                }
                status = syncDataObjPhyPath( rsComm, destDataObjInp, dataObjInfoHead, NULL );
                freeAllDataObjInfo( dataObjInfoHead );
            }
            else {
                status = syncCollPhyPath( rsComm, destDataObjInp->objPath );
            }

            if ( status >= 0 ) {
                status = chlCommit( rsComm );
            }
            else {
                chlRollback( rsComm );
            }
            if ( status >= 0 ) {
                args[0] = srcDataObjInp->objPath;
                args[1] = destDataObjInp->objPath;
                argc = 2;
                status =  applyRuleArg( "acPostProcForObjRename", args, argc,
                        &rei2, NO_SAVE_REI );
                if ( status < 0 ) {
                    if ( rei2.status < 0 ) {
                        status = rei2.status;
                    }
                    rodsLog( LOG_ERROR,
                            "rsDataObjRename: acPostProc err for src %s dest %s,stat=%d",
                            args[0], args[1], status );
                }
            }
        }
        else {
            chlRollback( rsComm );
        }
        return status;
    }

    int rsDataObjRename_impl(rsComm_t *rsComm, dataObjCopyInp_t *dataObjRenameInp)
    {
        int status;
        dataObjInp_t *srcDataObjInp, *destDataObjInp;
        rodsServerHost_t *rodsServerHost = NULL;
        dataObjInfo_t *srcDataObjInfo = NULL;
        dataObjInfo_t *destDataObjInfo = NULL;
        specCollCache_t *specCollCache = NULL;
        int srcType, destType;

        srcDataObjInp = &dataObjRenameInp->srcDataObjInp;
        destDataObjInp = &dataObjRenameInp->destDataObjInp;

        /* don't translate the link pt. treat it as a normal collection */
        addKeyVal( &srcDataObjInp->condInput, NO_TRANSLATE_LINKPT_KW, "" );
        resolveLinkedPath( rsComm, srcDataObjInp->objPath, &specCollCache,
                           &srcDataObjInp->condInput );
        rmKeyVal( &srcDataObjInp->condInput, NO_TRANSLATE_LINKPT_KW );

        resolveLinkedPath( rsComm, destDataObjInp->objPath, &specCollCache,
                           &destDataObjInp->condInput );

        if ( strcmp( srcDataObjInp->objPath, destDataObjInp->objPath ) == 0 ) {
            return SAME_SRC_DEST_PATHS_ERR;
        }

        /* connect to rcat for cross zone */
        status = getAndConnRcatHost(
                     rsComm,
                     MASTER_RCAT,
                     ( const char* )srcDataObjInp->objPath,
                     &rodsServerHost );
        if ( status < 0 || NULL == rodsServerHost ) {
            return status;
        }
        else if ( rodsServerHost->rcatEnabled == REMOTE_ICAT ) {
            status = rcDataObjRename( rodsServerHost->conn, dataObjRenameInp );
            return status;
        }

        srcType = resolvePathInSpecColl( rsComm, srcDataObjInp->objPath,
                                         WRITE_COLL_PERM, 0, &srcDataObjInfo );

        destType = resolvePathInSpecColl( rsComm, destDataObjInp->objPath,
                                          WRITE_COLL_PERM, 0, &destDataObjInfo );

        if ( srcDataObjInfo           != NULL &&
                srcDataObjInfo->specColl != NULL &&
                strcmp( srcDataObjInfo->specColl->collection,
                        srcDataObjInp->objPath ) == 0 ) {
            /* this must be the link pt or mount pt. treat it as normal coll */
            freeDataObjInfo( srcDataObjInfo );
            srcDataObjInfo = NULL;
            srcType = SYS_SPEC_COLL_NOT_IN_CACHE;
        }

        if ( !isSameZone( srcDataObjInp->objPath, destDataObjInp->objPath ) ) {
            return SYS_CROSS_ZONE_MV_NOT_SUPPORTED;
        }

        if ( destType >= 0 ) {
            rodsLog( LOG_ERROR,
                     "rsDataObjRename: dest specColl objPath %s exists",
                     destDataObjInp->objPath );
            freeDataObjInfo( srcDataObjInfo );
            freeDataObjInfo( destDataObjInfo );
            return SYS_DEST_SPEC_COLL_SUB_EXIST;
        }

        if ( srcType >= 0 ) {
            /* src is a specColl of some sort. dest must also be a specColl in
             * order to rename, except in the special case where src is in a
             * mounted collection. Otherwise, bail out with specColl conflict error. */
            if ( NULL != srcDataObjInfo && SYS_SPEC_COLL_OBJ_NOT_EXIST == destType ) {
                status = specCollObjRename( rsComm, srcDataObjInfo, destDataObjInfo );
            }
            else if ( NULL != srcDataObjInfo && NULL != srcDataObjInfo->specColl &&
                      MOUNTED_COLL == srcDataObjInfo->specColl->collClass ) {
                status = moveMountedCollObj( rsComm, srcDataObjInfo, srcType, destDataObjInp );
            }
            else {
                rodsLog( LOG_ERROR,
                         "rsDataObjRename: src %s is in spec coll but dest %s is not",
                         srcDataObjInp->objPath, destDataObjInp->objPath );
                status = SYS_SRC_DEST_SPEC_COLL_CONFLICT;
            }
            freeDataObjInfo( srcDataObjInfo );
            freeDataObjInfo( destDataObjInfo );
            return status;
        }
        else if ( srcType == SYS_SPEC_COLL_OBJ_NOT_EXIST ) {
            return SYS_SPEC_COLL_OBJ_NOT_EXIST;
        }
        else if ( destType == SYS_SPEC_COLL_OBJ_NOT_EXIST ) {
            /* source is normal object but dest is not */
            rodsLog( LOG_ERROR,
                     "rsDataObjRename: src %s is not in spec coll but dest %s is",
                     srcDataObjInp->objPath, destDataObjInp->objPath );
            return SYS_SRC_DEST_SPEC_COLL_CONFLICT;
        }

        status = getAndConnRcatHost(
                     rsComm,
                     MASTER_RCAT,
                     ( const char* )dataObjRenameInp->srcDataObjInp.objPath,
                     &rodsServerHost );
        if ( status < 0 ) {
            return status;
        }
        if ( rodsServerHost->localFlag == LOCAL_HOST ) {
            std::string svc_role;
            irods::error ret = get_catalog_service_role(svc_role);
            if(!ret.ok()) {
                irods::log(PASS(ret));
                return ret.code();
            }

            if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
                status = _rsDataObjRename( rsComm, dataObjRenameInp );
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
            status = rcDataObjRename( rodsServerHost->conn, dataObjRenameInp );
        }

        return status;
    }
} // anonymous namespace

int rsDataObjRename(rsComm_t *rsComm, dataObjCopyInp_t *dataObjRenameInp)
{
    const auto ec = rsDataObjRename_impl(rsComm, dataObjRenameInp);

    // Update the mtime of the parent collections.
    if (ec == 0) {
        const auto src_parent_path = fs::path{dataObjRenameInp->srcDataObjInp.objPath}.parent_path();
        const auto dst_parent_path = fs::path{dataObjRenameInp->destDataObjInp.objPath}.parent_path();

        using std::chrono::system_clock;
        using std::chrono::time_point_cast;

        const auto mtime = time_point_cast<fs::object_time_type::duration>(system_clock::now());

        try {
            irods::experimental::scoped_privileged_client spc{*rsComm};

            if (fs::server::is_collection_registered(*rsComm, src_parent_path)) {
                fs::server::last_write_time(*rsComm, src_parent_path, mtime);
            }

            if (src_parent_path != dst_parent_path) {
                if (fs::server::is_collection_registered(*rsComm, dst_parent_path)) {
                    fs::server::last_write_time(*rsComm, dst_parent_path, mtime);
                }
            }
        }
        catch (const fs::filesystem_error& e) {
            logger::api::error(e.what());
            return e.code().value();
        }
    }

    return ec;
}

int
specCollObjRename( rsComm_t *rsComm, dataObjInfo_t *srcDataObjInfo,
                   dataObjInfo_t *destDataObjInfo ) {
    int status;
    char *newPath;

    if ( getStructFileType( srcDataObjInfo->specColl ) >= 0 ) {
        /* structFile type, use the logical subPath */
        newPath = destDataObjInfo->subPath;
    }
    else {
        /* mounted fs, use phy path */
        newPath = destDataObjInfo->filePath;
    }

    status = l3Rename( rsComm, srcDataObjInfo, newPath );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "specCollObjRename: l3Rename error from %s to %s, status = %d",
                 srcDataObjInfo->subPath, newPath, status );
        return status;
    }
    return status;
}

int
l3Rename( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo, char *newFileName ) {
    fileRenameInp_t fileRenameInp;
    int status;

    irods::error resc_err = irods::is_hier_live( dataObjInfo->rescHier );
    if ( !resc_err.ok() ) {
        return resc_err.code();
    }

    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "l3Rename - failed in get_loc_for_hier_string", ret ) );
        return -1;
    }

    if ( getStructFileType( dataObjInfo->specColl ) >= 0 ) {
        subStructFileRenameInp_t subStructFileRenameInp;
        memset( &subStructFileRenameInp, 0, sizeof( subStructFileRenameInp ) );
        rstrcpy( subStructFileRenameInp.subFile.subFilePath, dataObjInfo->subPath,
                 MAX_NAME_LEN );
        rstrcpy( subStructFileRenameInp.newSubFilePath, newFileName, MAX_NAME_LEN );
        rstrcpy( subStructFileRenameInp.subFile.addr.hostAddr, location.c_str(), NAME_LEN );
        rstrcpy( subStructFileRenameInp.resc_hier, dataObjInfo->rescHier, MAX_NAME_LEN );
        subStructFileRenameInp.subFile.specColl = dataObjInfo->specColl;
        status = rsSubStructFileRename( rsComm, &subStructFileRenameInp );
    }
    else {
        memset( &fileRenameInp, 0, sizeof( fileRenameInp ) );
        rstrcpy( fileRenameInp.oldFileName,   dataObjInfo->filePath,  MAX_NAME_LEN );
        rstrcpy( fileRenameInp.newFileName,   newFileName,            MAX_NAME_LEN );
        rstrcpy( fileRenameInp.rescHier,      dataObjInfo->rescHier,  MAX_NAME_LEN );
        rstrcpy( fileRenameInp.objPath,       dataObjInfo->objPath,   MAX_NAME_LEN );
        rstrcpy( fileRenameInp.addr.hostAddr, location.c_str(),       NAME_LEN );
        fileRenameOut_t* ren_out = 0;
        status = rsFileRename( rsComm, &fileRenameInp, &ren_out );
        free( ren_out );
    }
    return status;
}

/* moveMountedCollObj - move a mounted collection obj to a normal obj */
int
moveMountedCollObj( rsComm_t *rsComm, dataObjInfo_t *srcDataObjInfo,
                    int srcType, dataObjInp_t *destDataObjInp ) {
    int status;

    if ( srcType == DATA_OBJ_T ) {
        status = moveMountedCollDataObj( rsComm, srcDataObjInfo,
                                         destDataObjInp );
    }
    else if ( srcType == COLL_OBJ_T ) {
        status = moveMountedCollCollObj( rsComm, srcDataObjInfo,
                                         destDataObjInp );
    }
    else {
        status = SYS_UNMATCHED_SPEC_COLL_TYPE;
    }
    return status;
}

int
moveMountedCollDataObj( rsComm_t *rsComm, dataObjInfo_t *srcDataObjInfo,
                        dataObjInp_t *destDataObjInp ) {
    dataObjInfo_t destDataObjInfo;
    fileRenameInp_t fileRenameInp;
    int status;

    if ( rsComm == NULL || srcDataObjInfo == NULL || destDataObjInp == NULL ) {
        return USER__NULL_INPUT_ERR;
    }
    std::memset(&destDataObjInfo, 0, sizeof(destDataObjInfo));
    std::memset(&fileRenameInp, 0, sizeof(fileRenameInp));
    rstrcpy( destDataObjInfo.objPath, destDataObjInp->objPath, MAX_NAME_LEN );
    rstrcpy( destDataObjInfo.dataType, srcDataObjInfo->dataType, NAME_LEN );
    destDataObjInfo.dataSize = srcDataObjInfo->dataSize;

    rstrcpy( destDataObjInfo.rescName, srcDataObjInfo->rescName, NAME_LEN );
    rstrcpy( destDataObjInfo.rescHier, srcDataObjInfo->rescHier, MAX_NAME_LEN );
    irods::error ret = resc_mgr.hier_to_leaf_id(srcDataObjInfo->rescHier,destDataObjInfo.rescId);
    if( !ret.ok() ) {
        irods::log(PASS(ret));
    }
    status = getFilePathName( rsComm, &destDataObjInfo, destDataObjInp );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "moveMountedCollDataObj: getFilePathName err for %s. status = %d",
                 destDataObjInfo.objPath, status );
        return status;
    }

    status = filePathTypeInResc( rsComm, destDataObjInfo.objPath, destDataObjInfo.filePath, destDataObjInfo.rescHier );
    if ( status == LOCAL_DIR_T ) {
        status = SYS_PATH_IS_NOT_A_FILE;
        rodsLog( LOG_ERROR,
                 "moveMountedCollDataObj: targ path %s is a dir. status = %d",
                 destDataObjInfo.filePath, status );
        return status;
    }
    else if ( status == LOCAL_FILE_T ) {
        dataObjInfo_t myDataObjInfo;
        status = chkOrphanFile( rsComm, destDataObjInfo.filePath,
                                destDataObjInfo.rescName, &myDataObjInfo );
        if ( status == 1 ) {
            /* orphan */
            char new_fn[ MAX_NAME_LEN ];
            rstrcpy( fileRenameInp.oldFileName, destDataObjInfo.filePath, MAX_NAME_LEN );
            int error_code = renameFilePathToNewDir( rsComm, ORPHAN_DIR, &fileRenameInp, 1, new_fn );
            if ( error_code < 0 ) {
                rodsLog( LOG_ERROR, "renameFilePathToNewDir failed in moveMountedCollDataObj with error code %d", error_code );
            }
            snprintf( destDataObjInfo.filePath, sizeof( destDataObjInfo.filePath ), "%s", new_fn );
        }
        else if ( status == 0 ) {
            /* obj exist */
            return SYS_COPY_ALREADY_IN_RESC;
        }
        else {
            return status;
        }
    }
    status = l3Rename( rsComm, srcDataObjInfo, destDataObjInfo.filePath );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "moveMountedCollDataObj: l3Rename error from %s to %s, status = %d",
                 srcDataObjInfo->filePath, destDataObjInfo.filePath, status );
        return status;
    }
    status = svrRegDataObj( rsComm, &destDataObjInfo );
    if ( status < 0 ) {
        l3Rename( rsComm, &destDataObjInfo, srcDataObjInfo->filePath );
        rodsLog( LOG_ERROR,
                 "moveMountedCollDataObj: rsRegDataObj for %s failed, status = %d",
                 destDataObjInfo.objPath, status );
    }
    return status;
}

int
moveMountedCollCollObj( rsComm_t *rsComm, dataObjInfo_t *srcDataObjInfo,
                        dataObjInp_t *destDataObjInp ) {
    int l3descInx;
    fileReaddirInp_t fileReaddirInp;
    rodsDirent_t *rodsDirent = NULL;
    rodsStat_t *fileStatOut = NULL;
    dataObjInfo_t subSrcDataObjInfo;
    dataObjInp_t subDestDataObjInp;
    int status;
    int savedStatus = 0;

    subSrcDataObjInfo = *srcDataObjInfo;
    std::memset(&subDestDataObjInp, 0, sizeof(subDestDataObjInp));
    l3descInx = l3Opendir( rsComm, srcDataObjInfo );
    fileReaddirInp.fileInx = l3descInx;
    rsMkCollR( rsComm, "/", destDataObjInp->objPath );
    while ( rsFileReaddir( rsComm, &fileReaddirInp, &rodsDirent ) >= 0 ) {
        if ( rodsDirent == NULL ) {
            savedStatus = SYS_INTERNAL_NULL_INPUT_ERR;
            break;
        }

        rodsDirent_t myRodsDirent = *rodsDirent;
        free( rodsDirent );

        if ( strcmp( myRodsDirent.d_name, "." ) == 0 ||
                strcmp( myRodsDirent.d_name, ".." ) == 0 ) {
            continue;
        }

        snprintf( subSrcDataObjInfo.objPath, MAX_NAME_LEN, "%s/%s",
                  srcDataObjInfo->objPath, myRodsDirent.d_name );
        snprintf( subSrcDataObjInfo.subPath, MAX_NAME_LEN, "%s/%s",
                  srcDataObjInfo->subPath, myRodsDirent.d_name );
        snprintf( subSrcDataObjInfo.filePath, MAX_NAME_LEN, "%s/%s",
                  srcDataObjInfo->filePath, myRodsDirent.d_name );

        status = l3Stat( rsComm, &subSrcDataObjInfo, &fileStatOut );
        if ( status < 0 || fileStatOut == NULL ) { // JMC cppcheck
            rodsLog( LOG_ERROR,
                     "moveMountedCollCollObj: l3Stat for %s error, status = %d",
                     subSrcDataObjInfo.filePath, status );
            return status;
        }
        snprintf( subSrcDataObjInfo.dataCreate, TIME_LEN, "%d",
                  fileStatOut->st_ctim );
        snprintf( subSrcDataObjInfo.dataModify, TIME_LEN, "%d",
                  fileStatOut->st_mtim );
        snprintf( subDestDataObjInp.objPath, MAX_NAME_LEN, "%s/%s",
                  destDataObjInp->objPath, myRodsDirent.d_name );
        if ( ( fileStatOut->st_mode & S_IFREG ) != 0 ) { /* a file */
            subSrcDataObjInfo.dataSize = fileStatOut->st_size;
            status = moveMountedCollDataObj( rsComm, &subSrcDataObjInfo,
                                             &subDestDataObjInp );
        }
        else {
            status = moveMountedCollCollObj( rsComm, &subSrcDataObjInfo,
                                             &subDestDataObjInp );
        }
        if ( status < 0 ) {
            savedStatus = status;
            rodsLog( LOG_ERROR,
                     "moveMountedCollCollObj: moveMountedColl for %s error, stat = %d",
                     subSrcDataObjInfo.objPath, status );
        }
        free( fileStatOut );
        fileStatOut = NULL;
    }
    l3Rmdir( rsComm, srcDataObjInfo );
    return savedStatus;
}
