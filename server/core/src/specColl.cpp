#include "collection.hpp"
#include "dataObjClose.h"
#include "dataObjInpOut.h"
#include "dataObjOpr.hpp"
#include "genQuery.h"
#include "icatDefines.h"
#include "modColl.h"
#include "objInfo.h"
#include "objMetaOpr.hpp"
#include "physPath.hpp"
#include "rcGlobalExtern.h"
#include "rcMisc.h"
#include "resource.hpp"
#include "rods.h"
#include "rsDataObjClose.hpp"
#include "rsFileCreate.hpp"
#include "rsGenQuery.hpp"
#include "rsGlobalExtern.hpp"
#include "rsModColl.hpp"
#include "rsObjStat.hpp"
#include "rsSubStructFileCreate.hpp"
#include "ruleExecSubmit.h"
#include "specColl.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_stacktrace.hpp"
#include "key_value_proxy.hpp"

static int HaveFailedSpecCollPath = 0;
static char FailedSpecCollPath[MAX_NAME_LEN];

namespace
{
    auto create_sub_struct_file(rsComm_t *rsComm, const int l1descInx) -> int
    {
        dataObjInfo_t *dataObjInfo = L1desc[l1descInx].dataObjInfo;
        std::string location{};
        irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
        if (!ret.ok()) {
            irods::log(LOG_ERROR, fmt::format(
                "{} - failed in get_loc_for_hier_string:[{}]; ec:[{}]",
                __FUNCTION__, ret.result(), ret.code()));
            return ret.code();
        }

        subFile_t subFile{};
        rstrcpy( subFile.subFilePath, dataObjInfo->subPath, MAX_NAME_LEN );
        rstrcpy( subFile.addr.hostAddr, location.c_str(), NAME_LEN );

        subFile.specColl = dataObjInfo->specColl;
        subFile.mode = getFileMode( L1desc[l1descInx].dataObjInp );
        return rsSubStructFileCreate( rsComm, &subFile );
    } // create_sub_struct_file
} // anonymous namespace

/* querySpecColl - The query can produce multiple answer and only one
 * is correct. e.g., objPath = /x/yabc can produce answers:
 * /x/y, /x/ya, /x/yabc, etc. The calling subroutine need to screen
 * /x/y, /x/ya out
 * check queueSpecCollCache () for screening.
 */
int
querySpecColl( rsComm_t *rsComm, const char *objPath, genQueryOut_t **genQueryOut ) {
    genQueryInp_t genQueryInp;
    int status;
    char condStr[MAX_NAME_LEN];

    if ( HaveFailedSpecCollPath && strcmp( objPath, FailedSpecCollPath ) == 0 ) {
        return CAT_NO_ROWS_FOUND;
    }

    /* see if objPath is in the path of a spec collection */
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );

    snprintf( condStr, MAX_NAME_LEN, "parent_of '%s'", objPath );
    addInxVal( &genQueryInp.sqlCondInp, COL_COLL_NAME, condStr );
    rstrcpy( condStr, "like '_%'", MAX_NAME_LEN );
    addInxVal( &genQueryInp.sqlCondInp, COL_COLL_TYPE, condStr );

    addInxIval( &genQueryInp.selectInp, COL_COLL_ID, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_OWNER_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_OWNER_ZONE, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_CREATE_TIME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_MODIFY_TIME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_TYPE, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_INFO1, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_INFO2, 1 );

    genQueryInp.maxRows = MAX_SQL_ROWS;

    status =  rsGenQuery( rsComm, &genQueryInp, genQueryOut );

    clearGenQueryInp( &genQueryInp );

    if ( status < 0 ) {
        rstrcpy( FailedSpecCollPath, objPath, MAX_NAME_LEN );
        HaveFailedSpecCollPath = 1;
        return status;
    }

    return 0;
}

/* queueSpecCollCache - queue the specColl given in genQueryOut.
 * genQueryOut may contain multiple answer and only one
 * is correct. e.g., objPath = /x/yabc can produce answers:
 * /x/y, /x/ya, /x/yabc, etc. The calling subroutine need to screen
 * /x/y, /x/ya out
 */

int
queueSpecCollCache( rsComm_t *rsComm, genQueryOut_t *genQueryOut, const char *objPath ) { // JMC - backport 4680
    int status;
    int i;
    sqlResult_t *dataId;
    sqlResult_t *ownerName;
    sqlResult_t *ownerZone;
    sqlResult_t *createTime;
    sqlResult_t *modifyTime;
    sqlResult_t *collType;
    sqlResult_t *collection;
    sqlResult_t *collInfo1;
    sqlResult_t *collInfo2;
    char *tmpDataId, *tmpOwnerName, *tmpOwnerZone, *tmpCreateTime,
         *tmpModifyTime, *tmpCollType, *tmpCollection, *tmpCollInfo1,
         *tmpCollInfo2;
    specColl_t *specColl;

    if ( ( dataId = getSqlResultByInx( genQueryOut, COL_COLL_ID ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "queueSpecCollCache: getSqlResultByInx for COL_COLL_ID failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    else if ( ( ownerName = getSqlResultByInx( genQueryOut,
                            COL_COLL_OWNER_NAME ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "queueSpecCollCache:getSqlResultByInx for COL_COLL_OWNER_NAME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    else if ( ( ownerZone = getSqlResultByInx( genQueryOut,
                            COL_COLL_OWNER_ZONE ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "queueSpecCollCache:getSqlResultByInx for COL_COLL_OWNER_ZONE failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    else if ( ( createTime = getSqlResultByInx( genQueryOut,
                             COL_COLL_CREATE_TIME ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "queueSpecCollCache:getSqlResultByInx for COL_COLL_CREATE_TIME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    else if ( ( modifyTime = getSqlResultByInx( genQueryOut,
                             COL_COLL_MODIFY_TIME ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "queueSpecCollCache:getSqlResultByInx for COL_COLL_MODIFY_TIME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    else if ( ( collType = getSqlResultByInx( genQueryOut,
                           COL_COLL_TYPE ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "queueSpecCollCache:getSqlResultByInx for COL_COLL_TYPE failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    else if ( ( collection = getSqlResultByInx( genQueryOut,
                             COL_COLL_NAME ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "queueSpecCollCache:getSqlResultByInx for COL_COLL_NAME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    else if ( ( collInfo1 = getSqlResultByInx( genQueryOut,
                            COL_COLL_INFO1 ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "queueSpecCollCache:getSqlResultByInx for COL_COLL_INFO1 failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    else if ( ( collInfo2 = getSqlResultByInx( genQueryOut,
                            COL_COLL_INFO2 ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "queueSpecCollCache:getSqlResultByInx for COL_COLL_INFO2 failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    for ( i = 0; i <= genQueryOut->rowCnt; i++ ) {
        int len;

        tmpCollection = &collection->value[collection->len * i];

        len = strlen( tmpCollection );
        const char* tmpPtr = objPath + len;

        if ( *tmpPtr == '\0' || *tmpPtr == '/' ) {
            specCollCache_t * tmpSpecCollCache = ( specCollCache_t* )malloc( sizeof( specCollCache_t ) );
            memset( tmpSpecCollCache, 0, sizeof( specCollCache_t ) );

            tmpDataId = &dataId->value[dataId->len * i];
            tmpOwnerName = &ownerName->value[ownerName->len * i];
            tmpOwnerZone = &ownerZone->value[ownerZone->len * i];
            tmpCreateTime = &createTime->value[createTime->len * i];
            tmpModifyTime = &modifyTime->value[modifyTime->len * i];
            tmpCollType = &collType->value[collType->len * i];
            tmpCollInfo1 = &collInfo1->value[collInfo1->len * i];
            tmpCollInfo2 = &collInfo2->value[collInfo2->len * i];

            specColl = &tmpSpecCollCache->specColl;
            status = resolveSpecCollType( tmpCollType, tmpCollection, tmpCollInfo1, tmpCollInfo2, specColl );
            if ( status < 0 ) {
                free( tmpSpecCollCache );
                return status;
            }

            // =-=-=-=-=-=-=-
            // JMC - backport 4680
            if ( specColl->collClass == STRUCT_FILE_COLL &&
                    specColl->type      == TAR_STRUCT_FILE_T ) {
                /* tar struct file. need to get phyPath */
                status = getPhyPath( rsComm, specColl->objPath, specColl->resource, specColl->phyPath, specColl->rescHier );
                if ( status < 0 ) {
                    rodsLog( LOG_ERROR, "queueSpecCollCache - getPhyPath failed for [%s] on resource [%s] with cache dir [%s] and collection [%s]",
                             specColl->objPath, specColl->resource, specColl->cacheDir, specColl->collection );
                    free( tmpSpecCollCache );
                    return status;
                }
            }
            // =-=-=-=-=-=-=-
            rstrcpy( tmpSpecCollCache->collId, tmpDataId, NAME_LEN );
            rstrcpy( tmpSpecCollCache->ownerName, tmpOwnerName, NAME_LEN );
            rstrcpy( tmpSpecCollCache->ownerZone, tmpOwnerZone, NAME_LEN );
            rstrcpy( tmpSpecCollCache->createTime, tmpCreateTime, TIME_LEN );
            rstrcpy( tmpSpecCollCache->modifyTime, tmpModifyTime, TIME_LEN );
            tmpSpecCollCache->next = SpecCollCacheHead;
            SpecCollCacheHead = tmpSpecCollCache;

            return 0;
        }
    }

    return CAT_NO_ROWS_FOUND;
}

int
queueSpecCollCacheWithObjStat( rodsObjStat_t *rodsObjStatOut ) {
    specCollCache_t *tmpSpecCollCache;

    tmpSpecCollCache = ( specCollCache_t* )malloc( sizeof( specCollCache_t ) );
    memset( tmpSpecCollCache, 0, sizeof( specCollCache_t ) );

    tmpSpecCollCache->specColl = *rodsObjStatOut->specColl;

    rstrcpy( tmpSpecCollCache->collId, rodsObjStatOut->dataId, NAME_LEN );
    rstrcpy( tmpSpecCollCache->ownerName, rodsObjStatOut->ownerName, NAME_LEN );
    rstrcpy( tmpSpecCollCache->ownerZone, rodsObjStatOut->ownerZone, NAME_LEN );
    rstrcpy( tmpSpecCollCache->createTime, rodsObjStatOut->createTime, TIME_LEN );
    rstrcpy( tmpSpecCollCache->modifyTime, rodsObjStatOut->modifyTime, TIME_LEN );


    tmpSpecCollCache->next = SpecCollCacheHead;
    SpecCollCacheHead = tmpSpecCollCache;

    return 0;

}

specCollCache_t *
matchSpecCollCache(const char *objPath ) {
    specCollCache_t *tmpSpecCollCache = SpecCollCacheHead;

    while ( tmpSpecCollCache != NULL ) {
        int len = strlen( tmpSpecCollCache->specColl.collection );
        if ( strncmp( tmpSpecCollCache->specColl.collection, objPath, len )
                == 0 ) {
            const char *tmpPtr = objPath + len;

            if ( *tmpPtr == '\0' || *tmpPtr == '/' ) {
                return tmpSpecCollCache;
            }
        }
        tmpSpecCollCache = tmpSpecCollCache->next;
    }
    return NULL;
}

int
getSpecCollCache( rsComm_t *rsComm, const char *objPath,
                  int inCachOnly, specCollCache_t **specCollCache ) {
    int status;
    genQueryOut_t *genQueryOut = NULL;

    if ( ( *specCollCache = matchSpecCollCache( objPath ) ) != NULL ) {
        return 0;
    }
    else if ( inCachOnly > 0 ) {
        return SYS_SPEC_COLL_NOT_IN_CACHE;
    }

    status = querySpecColl( rsComm, objPath, &genQueryOut );
    if ( status < 0 ) {
        freeGenQueryOut( &genQueryOut );
        return status;
    }

    status = queueSpecCollCache( rsComm, genQueryOut, objPath ); // JMC - backport 4680
    freeGenQueryOut( &genQueryOut );

    if ( status < 0 ) {
        return status;
    }
    *specCollCache = SpecCollCacheHead;  /* queued at top */

    return 0;
}

int
modCollInfo2( rsComm_t *rsComm, specColl_t *specColl, int clearFlag ) {

    int status;
    char collInfo2[MAX_NAME_LEN];
    collInp_t modCollInp;

    memset( &modCollInp, 0, sizeof( modCollInp ) );
    rstrcpy( modCollInp.collName, specColl->collection, MAX_NAME_LEN );
    //addKeyVal( &modCollInp.condInput, COLLECTION_TYPE_KW,
    //           TAR_STRUCT_FILE_STR ); /* need this or rsModColl fail */
    if ( clearFlag > 0 ) {
        rstrcpy( collInfo2, "NULL_SPECIAL_VALUE", MAX_NAME_LEN );
    }
    else {
        makeCachedStructFileStr( collInfo2, specColl );
    }
    addKeyVal( &modCollInp.condInput, COLLECTION_INFO2_KW, collInfo2 );
    status = rsModColl( rsComm, &modCollInp );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "tarSubStructFileWrite:rsModColl error for Coll %s,stat=%d",
                 modCollInp.collName, status );
    }
    return status;
}

/* statPathInSpecColl - stat the path given in objPath assuming it is
 * in the path of a special collection. The inCachOnly flag asks it to
 * check the specColl in the global cache only. The output of the
 * stat is given in rodsObjStatOut.
 *
 */

int
statPathInSpecColl( rsComm_t *rsComm, char *objPath,
                    int inCachOnly, rodsObjStat_t **rodsObjStatOut ) {
    int status;
    dataObjInfo_t *dataObjInfo = NULL;
    specColl_t *specColl;
    specCollCache_t *specCollCache;

    if ( ( status = getSpecCollCache( rsComm, objPath, inCachOnly,
                                      &specCollCache ) ) < 0 ) {
        if ( status != SYS_SPEC_COLL_NOT_IN_CACHE &&
                status != CAT_NO_ROWS_FOUND ) {
            rodsLog( LOG_ERROR,
                     "statPathInSpecColl: getSpecCollCache for %s, status = %d",
                     objPath, status );
        }
        return status;
    }

    if ( *rodsObjStatOut == NULL ) {
        *rodsObjStatOut = ( rodsObjStat_t * ) malloc( sizeof( rodsObjStat_t ) );
    }
    memset( *rodsObjStatOut, 0, sizeof( rodsObjStat_t ) );
    specColl = &specCollCache->specColl;
    rstrcpy( ( *rodsObjStatOut )->dataId, specCollCache->collId, NAME_LEN );
    rstrcpy( ( *rodsObjStatOut )->ownerName, specCollCache->ownerName, NAME_LEN );
    rstrcpy( ( *rodsObjStatOut )->ownerZone, specCollCache->ownerZone, NAME_LEN );

    status = specCollSubStat( rsComm, specColl, objPath, UNKNOWN_COLL_PERM, &dataObjInfo );

    if ( status < 0 ) {
        if ( dataObjInfo != NULL ) {
            if ( dataObjInfo->specColl != NULL ) {
                ( *rodsObjStatOut )->specColl = dataObjInfo->specColl;
            }
            else {
                replSpecColl( &specCollCache->specColl,
                              &( *rodsObjStatOut )->specColl );
            }
            if ( specColl->collClass == LINKED_COLL ) {
                rstrcpy( ( *rodsObjStatOut )->specColl->objPath,
                         dataObjInfo->objPath, MAX_NAME_LEN );
            }
            else {
                ( *rodsObjStatOut )->specColl->objPath[0] = '\0';
            }
            dataObjInfo->specColl = NULL;
        }
        ( *rodsObjStatOut )->objType = UNKNOWN_OBJ_T;
        rstrcpy( ( *rodsObjStatOut )->createTime, specCollCache->createTime,
                 TIME_LEN );
        rstrcpy( ( *rodsObjStatOut )->modifyTime, specCollCache->modifyTime,
                 TIME_LEN );
        freeAllDataObjInfo( dataObjInfo );
        /* XXXXX 0 return is creating a problem for fuse */
        return 0;
    }
    else {
        ( *rodsObjStatOut )->specColl = dataObjInfo->specColl;
        dataObjInfo->specColl = NULL;

        if ( specColl->collClass == LINKED_COLL ) {
            rstrcpy( ( *rodsObjStatOut )->ownerName, dataObjInfo->dataOwnerName,
                     NAME_LEN );
            rstrcpy( ( *rodsObjStatOut )->ownerZone, dataObjInfo->dataOwnerZone,
                     NAME_LEN );
            snprintf( ( *rodsObjStatOut )->dataId, NAME_LEN, "%lld",
                      dataObjInfo->dataId );
            /* save the linked path here */
            rstrcpy( ( *rodsObjStatOut )->specColl->objPath,
                     dataObjInfo->objPath, MAX_NAME_LEN );
        }
        ( *rodsObjStatOut )->objType = ( objType_t )status;
        ( *rodsObjStatOut )->objSize = dataObjInfo->dataSize;
        rstrcpy( ( *rodsObjStatOut )->createTime, dataObjInfo->dataCreate,
                 TIME_LEN );
        rstrcpy( ( *rodsObjStatOut )->modifyTime, dataObjInfo->dataModify,
                 TIME_LEN );
        freeAllDataObjInfo( dataObjInfo );
    }

    return status;
}

/* specCollSubStat - Given specColl and the object path (subPath),
 * returns a dataObjInfo struct with dataObjInfo->specColl != NULL.
 * Returns COLL_OBJ_T if the path is a collection or DATA_OBJ_T if the
 * path is a data oobject.
 */

int
specCollSubStat( rsComm_t *rsComm, specColl_t *specColl,
                 char *subPath, specCollPerm_t specCollPerm, dataObjInfo_t **dataObjInfo ) {
    int status;
    int objType;
    rodsStat_t *rodsStat = NULL;
    dataObjInfo_t *myDataObjInfo = NULL;;

    if ( dataObjInfo == NULL ) {
        return USER__NULL_INPUT_ERR;
    }
    *dataObjInfo = NULL;

    if ( specColl->collClass == MOUNTED_COLL ) {
        /* a mount point */
        myDataObjInfo = *dataObjInfo = ( dataObjInfo_t * ) malloc( sizeof( dataObjInfo_t ) );

        memset( myDataObjInfo, 0, sizeof( dataObjInfo_t ) );

        /*status = resolveResc (specColl->resource, &myDataObjInfo->rescInfo);
        if (status < 0) {
            rodsLog( LOG_ERROR,"specCollSubStat: resolveResc error for %s, status = %d",
                     specColl->resource, status);
            freeDataObjInfo (myDataObjInfo);
            *dataObjInfo = NULL;
            return status;
        }*/

        rstrcpy( myDataObjInfo->objPath, subPath, MAX_NAME_LEN );
        rstrcpy( myDataObjInfo->subPath, subPath, MAX_NAME_LEN );
        rstrcpy( myDataObjInfo->rescName, specColl->resource, NAME_LEN );
        rstrcpy( myDataObjInfo->rescHier, specColl->rescHier, MAX_NAME_LEN );
        rstrcpy( myDataObjInfo->dataType, "generic", NAME_LEN );
        irods::error ret = resc_mgr.hier_to_leaf_id(myDataObjInfo->rescHier,myDataObjInfo->rescId);
        if( !ret.ok() ) {
            irods::log(PASS(ret));
        }

        status = getMountedSubPhyPath( specColl->collection,
                                       specColl->phyPath, subPath, myDataObjInfo->filePath );
        if ( status < 0 ) {
            freeDataObjInfo( myDataObjInfo );
            *dataObjInfo = NULL;
            return status;
        }
        replSpecColl( specColl, &myDataObjInfo->specColl );
    }
    else if ( specColl->collClass == LINKED_COLL ) {

        /* a link point */
        specCollCache_t *specCollCache = NULL;
        char newPath[MAX_NAME_LEN];
        specColl_t *curSpecColl;
        char *accessStr;
        dataObjInp_t myDataObjInp;
        rodsObjStat_t *rodsObjStatOut = NULL;

        *dataObjInfo = NULL;
        curSpecColl = specColl;

        status = getMountedSubPhyPath( curSpecColl->collection,
                                       curSpecColl->phyPath, subPath, newPath );
        if ( status < 0 ) {
            return status;
        }

        status = resolveLinkedPath( rsComm, newPath, &specCollCache, NULL );
        if ( status < 0 ) {
            return status;
        }
        if ( specCollCache != NULL &&
                specCollCache->specColl.collClass != LINKED_COLL ) {

            status = specCollSubStat( rsComm, &specCollCache->specColl,
                                      newPath, specCollPerm, dataObjInfo );
            return status;
        }
        bzero( &myDataObjInp, sizeof( myDataObjInp ) );
        rstrcpy( myDataObjInp.objPath, newPath, MAX_NAME_LEN );

        status = collStat( rsComm, &myDataObjInp, &rodsObjStatOut );
        if ( status >= 0 && NULL != rodsObjStatOut ) {      /* a collection */ // JMC cppcheck - nullptr
            myDataObjInfo = *dataObjInfo =
                                ( dataObjInfo_t * ) malloc( sizeof( dataObjInfo_t ) );
            memset( myDataObjInfo, 0, sizeof( dataObjInfo_t ) );
            replSpecColl( curSpecColl, &myDataObjInfo->specColl );
            rstrcpy( myDataObjInfo->objPath, newPath, MAX_NAME_LEN );
            myDataObjInfo->dataId = strtoll( rodsObjStatOut->dataId, 0, 0 );
            rstrcpy( myDataObjInfo->dataOwnerName, rodsObjStatOut->ownerName, NAME_LEN );
            rstrcpy( myDataObjInfo->dataOwnerZone, rodsObjStatOut->ownerZone, NAME_LEN );
            rstrcpy( myDataObjInfo->dataCreate,    rodsObjStatOut->createTime, TIME_LEN );
            rstrcpy( myDataObjInfo->dataModify,    rodsObjStatOut->modifyTime, TIME_LEN );
            freeRodsObjStat( rodsObjStatOut );
            return COLL_OBJ_T;
        }
        freeRodsObjStat( rodsObjStatOut );

        /* data object */
        if ( specCollPerm == READ_COLL_PERM ) {
            accessStr = ACCESS_READ_OBJECT;
        }
        else if ( specCollPerm == WRITE_COLL_PERM ) {
            accessStr = ACCESS_DELETE_OBJECT;
        }
        else {
            accessStr = NULL;
        }

        status = getDataObjInfo( rsComm, &myDataObjInp, dataObjInfo,
                                 accessStr, 0 );
        if ( status < 0 ) {
            myDataObjInfo = *dataObjInfo =
                                ( dataObjInfo_t * ) malloc( sizeof( dataObjInfo_t ) );
            memset( myDataObjInfo, 0, sizeof( dataObjInfo_t ) );
            replSpecColl( curSpecColl, &myDataObjInfo->specColl );
            rstrcpy( myDataObjInfo->objPath, newPath, MAX_NAME_LEN );
            rodsLog( LOG_DEBUG,
                     "specCollSubStat: getDataObjInfo error for %s, status = %d",
                     newPath, status );
            return status;
        }
        else {
            replSpecColl( curSpecColl, &( *dataObjInfo )->specColl );
            return DATA_OBJ_T;
        }
    }
    else if ( getStructFileType( specColl ) >= 0 ) {

        /* bundle */
        dataObjInp_t myDataObjInp;
        dataObjInfo_t *tmpDataObjInfo;

        bzero( &myDataObjInp, sizeof( myDataObjInp ) );
        rstrcpy( myDataObjInp.objPath, specColl->objPath, MAX_NAME_LEN );
        // add the resource hierarchy to the condInput of the inp
        addKeyVal( &myDataObjInp.condInput, RESC_HIER_STR_KW, specColl->rescHier );
        status = getDataObjInfo( rsComm, &myDataObjInp, dataObjInfo, NULL, 1 );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "specCollSubStat: getDataObjInfo error for %s, status = %d",
                     myDataObjInp.objPath, status );
            *dataObjInfo = NULL;
            return status;
        }

        /* screen out any stale copies */
        status = sortObjInfoForOpen( dataObjInfo, &myDataObjInp.condInput, 0 );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "specCollSubStat: sortObjInfoForOpen error for %s. status = %d",
                     myDataObjInp.objPath, status );
            return status;
        }

        if ( strlen( specColl->resource ) > 0 ) {
            if ( requeDataObjInfoByResc( dataObjInfo, specColl->resource,
                                         0, 1 ) >= 0 ) {
                if ( strstr( ( *dataObjInfo )->rescHier,
                             specColl->resource ) == 0 ) {
                    rodsLog( LOG_ERROR,
                             "specCollSubStat: %s in %s does not match cache resc %s",
                             myDataObjInp.objPath, ( *dataObjInfo )->rescName,
                             specColl->resource );
                    freeAllDataObjInfo( *dataObjInfo );
                    *dataObjInfo = NULL;
                    return SYS_CACHE_STRUCT_FILE_RESC_ERR;
                }
            }
            else {
                rodsLog( LOG_ERROR,
                         "specCollSubStat: requeDataObjInfoByResc %s, resc %s error",
                         myDataObjInp.objPath, specColl->resource );
                freeAllDataObjInfo( *dataObjInfo );
                *dataObjInfo = NULL;
                return SYS_CACHE_STRUCT_FILE_RESC_ERR;
            }
        }

        /* free all the other dataObjInfo */
        if ( ( *dataObjInfo )->next != NULL ) {
            freeAllDataObjInfo( ( *dataObjInfo )->next );
            ( *dataObjInfo )->next = NULL;
        }

        /* fill in DataObjInfo */
        tmpDataObjInfo = *dataObjInfo;
        replSpecColl( specColl, &tmpDataObjInfo->specColl );
        rstrcpy( specColl->resource, tmpDataObjInfo->rescName, NAME_LEN );
        rstrcpy( specColl->rescHier, tmpDataObjInfo->rescHier, MAX_NAME_LEN );
        rstrcpy( specColl->phyPath, tmpDataObjInfo->filePath, MAX_NAME_LEN );
        rstrcpy( tmpDataObjInfo->subPath, subPath, MAX_NAME_LEN );
        specColl->replNum = tmpDataObjInfo->replNum;

        if ( strcmp( ( *dataObjInfo )->subPath, specColl->collection ) == 0 ) {
            /* no need to go down */
            return COLL_OBJ_T;
        }
    }
    else {
        rodsLog( LOG_ERROR,
                 "specCollSubStat: Unknown specColl collClass = %d",
                 specColl->collClass );
        return SYS_UNKNOWN_SPEC_COLL_CLASS;
    }
    status = l3Stat( rsComm, *dataObjInfo, &rodsStat );

    if ( status < 0 ) {
        return status;
    }

    if ( rodsStat->st_ctim != 0 ) {
        snprintf( ( *dataObjInfo )->dataCreate, TIME_LEN, "%d", rodsStat->st_ctim );
        snprintf( ( *dataObjInfo )->dataModify, TIME_LEN, "%d", rodsStat->st_mtim );
    }

    if ( rodsStat->st_mode & S_IFDIR ) {
        objType = COLL_OBJ_T;
    }
    else {
        objType = DATA_OBJ_T;
        ( *dataObjInfo )->dataSize = rodsStat->st_size;
    }
    free( rodsStat );

    return objType;
}

/* resolvePathInSpecColl - given the object path in dataObjInp->objPath, see if
 * it is in the path of a special collection (mounted or structfile).
 * If it is not in a special collection, returns a -ive value.
 * The inCachOnly flag asks it to check the specColl in the global cache only
 * If it is, returns a dataObjInfo struct with dataObjInfo->specColl != NULL.
 * Returns COLL_OBJ_T if the path is a collection or DATA_OBJ_T if the
 * path is a data oobject.
 */
int
resolvePathInSpecColl( rsComm_t *rsComm, char *objPath,
                       specCollPerm_t specCollPerm, int inCachOnly, dataObjInfo_t **dataObjInfo ) {
    specCollCache_t *specCollCache;
    specColl_t *cachedSpecColl;
    int status;
    char *accessStr;

    if ( objPath == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
    if ( ( status = getSpecCollCache( rsComm, objPath, inCachOnly,
                                      &specCollCache ) ) < 0 ) {

        return status;
    }
    else {
        cachedSpecColl = &specCollCache->specColl;
    }

    if ( specCollPerm != UNKNOWN_COLL_PERM ) {
        if ( specCollPerm == WRITE_COLL_PERM ) {
            accessStr = ACCESS_DELETE_OBJECT;
        }
        else {
            accessStr = ACCESS_READ_OBJECT;
        }

        if ( specCollCache->perm < specCollPerm ) {
            status = checkCollAccessPerm( rsComm, cachedSpecColl->collection,
                                          accessStr );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "resolvePathInSpecColl:checkCollAccessPerm err for %s,stat=%d",
                         cachedSpecColl->collection, status );
                return status;
            }
            else {
                specCollCache->perm = specCollPerm;
            }
        }
    }

    status = specCollSubStat( rsComm, cachedSpecColl, objPath,
                              specCollPerm, dataObjInfo );

    if ( status < 0 ) {
        if ( *dataObjInfo != NULL ) {
            /* does not exist. return the dataObjInfo anyway */
            return SYS_SPEC_COLL_OBJ_NOT_EXIST;
        }
        rodsLog( LOG_ERROR,
                 "resolvePathInSpecColl: specCollSubStat error for %s, status = %d",
                 objPath, status );
        return status;
    }
    else {
        if ( *dataObjInfo != NULL ) {
            if ( specCollPerm == WRITE_COLL_PERM ) {
                ( *dataObjInfo )->writeFlag = 1;
            }
        }
    }

    return status;
}

int
resolveLinkedPath( rsComm_t *rsComm, char *objPath,
                   specCollCache_t **specCollCache, keyValPair_t *condInput ) {
    int linkCnt = 0;
    specColl_t *curSpecColl;
    char prevNewPath[MAX_NAME_LEN];
    specCollCache_t *oldSpecCollCache = NULL;
    int status;

    *specCollCache = NULL;

    if ( getValByKey( condInput, TRANSLATED_PATH_KW ) != NULL ) {
        return 0;
    }

    addKeyVal( condInput, TRANSLATED_PATH_KW, "" );
    while ( (status = getSpecCollCache( rsComm, objPath, 0,  specCollCache )) >= 0 &&
            ( *specCollCache )->specColl.collClass == LINKED_COLL ) {
        oldSpecCollCache = *specCollCache;
        if ( linkCnt++ >= MAX_LINK_CNT ) {
            rodsLog( LOG_ERROR,
                     "resolveLinkedPath: linkCnt for %s exceeds %d",
                     objPath, MAX_LINK_CNT );
            return SYS_LINK_CNT_EXCEEDED_ERR;
        }

        curSpecColl = &( *specCollCache )->specColl;
        if ( strcmp( curSpecColl->collection, objPath ) == 0 &&
                getValByKey( condInput, NO_TRANSLATE_LINKPT_KW ) != NULL ) {
            return 0;
        }
        rstrcpy( prevNewPath, objPath, MAX_NAME_LEN );
        status = getMountedSubPhyPath( curSpecColl->collection,
                                       curSpecColl->phyPath, prevNewPath, objPath );
        if ( status < 0 ) {
            return status;
        }
    }
    if ( *specCollCache == NULL ) {
        *specCollCache = oldSpecCollCache;
    }
    // Issue 3913 - this failure must be bubbled up to the user
    if (status == USER_STRLEN_TOOLONG) {
        return USER_STRLEN_TOOLONG;
    }
    return linkCnt;
}

namespace irods
{
    auto get_special_collection_type_for_data_object(RsComm& _comm, DataObjInp& _inp) -> int
    {
        rodsObjStat* stat = nullptr;
        const irods::at_scope_exit free_obj_stat_out{[&stat] { freeRodsObjStat(stat); }};

        irods::experimental::key_value_proxy{_inp.condInput}[SEL_OBJ_TYPE_KW] = "dataObj";
        if (const int ec = rsObjStat(&_comm, &_inp, &stat); ec < 0) {
            irods::log(LOG_DEBUG, fmt::format(
                "[{}:{}] - rsObjStat failed with [{}]",
                __FUNCTION__, __LINE__, ec));

            return NO_SPEC_COLL;
        }

        if (stat) {
            if (COLL_OBJ_T == stat->objType) {
                return USER_INPUT_PATH_ERR;
            }

            if (stat->specColl) {
                return stat->specColl->collClass;
            }
        }

        return NO_SPEC_COLL;
    } // get_special_collection_type_for_data_object

    auto data_object_create_in_special_collection(RsComm* rsComm, DataObjInp& dataObjInp) -> int
    {
        dataObjInfo_t* dataObjInfo{};
        const int status = resolvePathInSpecColl( rsComm, dataObjInp.objPath, WRITE_COLL_PERM, 0, &dataObjInfo );
        if (!dataObjInfo) {
            rodsLog(LOG_ERROR, "%s :: dataObjInfo is null", __FUNCTION__ );
            return status;
        }

        if (status >= 0) {
            irods::log(LOG_ERROR, fmt::format(
                "{}: phyPath {} already exist",
                __FUNCTION__, dataObjInfo->filePath));
            freeDataObjInfo( dataObjInfo );
            return SYS_COPY_ALREADY_IN_RESC;
        }
        else if (status != SYS_SPEC_COLL_OBJ_NOT_EXIST) {
            freeDataObjInfo( dataObjInfo );
            return status;
        }

        // Objects in special collections are exempt from intermediate replicas and logical
        // locking, so make sure the replica is good from creation as it has been historically.
        dataObjInfo->replStatus = GOOD_REPLICA;

        const auto l1_index = irods::populate_L1desc_with_inp(dataObjInp, *dataObjInfo, dataObjInp.dataSize);
        if (l1_index < 3) {
            freeDataObjInfo(dataObjInfo);

            if (l1_index < 0) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - failed to generate L1 descriptor; path:[{}],ec:[{}]",
                    __FUNCTION__, __LINE__, dataObjInp.objPath, l1_index));

                return l1_index;
            }


            return SYS_FILE_DESC_OUT_OF_RANGE;
        }

        if (irods::experimental::key_value_proxy{dataObjInp.condInput}.contains(NO_OPEN_FLAG_KW)) {
            return l1_index;
        }

        const auto l3_index = getStructFileType(dataObjInfo->specColl) >= 0
            ? create_sub_struct_file(rsComm, l1_index)
            : irods::create_physical_file_for_replica(*rsComm, dataObjInp, *dataObjInfo);

        if (l3_index < 0) {
            freeDataObjInfo(dataObjInfo);

            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to create physical file "
                "[error_code=[{}], logical path=[{}], physical file=[{}]]",
                __FUNCTION__, __LINE__, l3_index, dataObjInp.objPath, L1desc[l1_index].dataObjInfo->filePath));

            freeL1desc(l1_index);

            return l3_index;
        }

        L1desc[l1_index].l3descInx = l3_index;

        return l1_index;
    } // data_object_create_in_special_collection
} // namespace irods
