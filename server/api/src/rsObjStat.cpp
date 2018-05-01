/*** Copyright (c), The Unregents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsObjStat.c
 */

#include "rsObjStat.hpp"
#include "objStat.h"
#include "rcMisc.h"
#include "genQuery.h"
#include "querySpecColl.h"
#include "objMetaOpr.hpp"
#include "collection.hpp"
#include "specColl.hpp"
#include "resource.hpp"
#include "rcGlobalExtern.h"
#include "rsGlobalExtern.hpp"
#include "dataObjClose.h"
#include "miscServerFunct.hpp"
#include "irods_configuration_keywords.hpp"
#include "rsGenQuery.hpp"

int
rsObjStat(
    rsComm_t *rsComm,
    dataObjInp_t *dataObjInp,
    rodsObjStat_t **rodsObjStatOut ) {
    int status;
    rodsServerHost_t *rodsServerHost = nullptr;
    specCollCache_t *specCollCache = nullptr;
    int linkCnt;

    linkCnt = resolveLinkedPath( rsComm, dataObjInp->objPath, &specCollCache,
                                 nullptr );

    *rodsObjStatOut = nullptr;
    status = getAndConnRcatHost(
                 rsComm,
                 SLAVE_RCAT,
                 ( const char* )dataObjInp->objPath,
                 &rodsServerHost );
    if ( status < 0 || nullptr == rodsServerHost ) { // JMC cppcheck - nullptr
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
            status = _rsObjStat( rsComm, dataObjInp, rodsObjStatOut );
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
        if ( isLocalZone( dataObjInp->objPath ) ) {
            /* see if it is a sub path of a specColl cached locally. If it is,
             * it will save time resolving it */
            status = statPathInSpecColl( rsComm, dataObjInp->objPath, 1, rodsObjStatOut );

            if ( status >= 0 ) {
                /* the path is in a specColl */
                return status;
            }
            else if ( status != SYS_SPEC_COLL_NOT_IN_CACHE ) {
                /* path is in the path of specColl cache but does not exist */
                if ( linkCnt > 0 && *rodsObjStatOut != nullptr ) {
                    /* a soft link - returns specColl */
                    if ( ( *rodsObjStatOut )->specColl == nullptr ) {
                        replSpecColl( &specCollCache->specColl,
                                      &( *rodsObjStatOut )->specColl );
                    }
                    rstrcpy( ( *rodsObjStatOut )->specColl->objPath,
                             dataObjInp->objPath, MAX_NAME_LEN );
                }

                return status;
            }
            /* falls through if the path is not in a cached specColl */
        }
        /* not in cache, need to do a remote call */
        status = rcObjStat( rodsServerHost->conn, dataObjInp, rodsObjStatOut );
        if ( status >= 0 && *rodsObjStatOut != nullptr && ( *rodsObjStatOut )->specColl != nullptr ) {
            /* queue it in cache */
            queueSpecCollCacheWithObjStat( *rodsObjStatOut );
        }
    }

    if ( linkCnt > 0 && *rodsObjStatOut != nullptr ) {
        /* fill in (*rodsObjStatOut)->specColl if it is linked */
        if ( ( *rodsObjStatOut )->specColl == nullptr ) {
            replSpecColl( &specCollCache->specColl,
                          &( *rodsObjStatOut )->specColl );
        }
        rstrcpy( ( *rodsObjStatOut )->specColl->objPath, dataObjInp->objPath, MAX_NAME_LEN );

    }
    return status;
}

int
_rsObjStat( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
            rodsObjStat_t **rodsObjStatOut ) {

    int status;
    char *tmpStr;
    specCollCache_t *specCollCache;

    /* do data first to catch data registered in spec Coll */
    if ( ( tmpStr = getValByKey( &dataObjInp->condInput, SEL_OBJ_TYPE_KW ) ) ==
            nullptr || strcmp( tmpStr, "dataObj" ) == 0 ) {
        status = dataObjStat( rsComm, dataObjInp, rodsObjStatOut );
        if ( status >= 0 ) {
            return status;
        }
    }

    if ( tmpStr == nullptr || strcmp( tmpStr, "collection" ) == 0 ) {
        status = collStat( rsComm, dataObjInp, rodsObjStatOut );
        /* specColl may already been obtained from collStat */
        if ( status >= 0 ) {
            if ( ( *rodsObjStatOut )->specColl == nullptr ) {
                if ( getSpecCollCache( rsComm, dataObjInp->objPath, 0,
                                       &specCollCache ) >= 0 ) {
                    replSpecColl( &specCollCache->specColl,
                                  &( *rodsObjStatOut )->specColl );
                }
            }
            return status;
        }
    }

    /*  not normal dataObj or coll. now check specColl */
    /* XXXX need to check a rule if it supports spec collection */
    status = statPathInSpecColl( rsComm, dataObjInp->objPath, 0, rodsObjStatOut );
    /* use USER_FILE_DOES_NOT_EXIST instead of OBJ_PATH_DOES_NOT_EXIST
     * because icommand depends on it */
    if ( status < 0 ) {
        status = USER_FILE_DOES_NOT_EXIST;
    }
    return status;
}

int
dataObjStat( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
             rodsObjStat_t **rodsObjStatOut ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = nullptr;
    int status;
    char myColl[MAX_NAME_LEN], myData[MAX_NAME_LEN];
    char condStr[MAX_NAME_LEN];
    sqlResult_t *dataSize;
    sqlResult_t *dataMode;
    sqlResult_t *replStatus;
    sqlResult_t *dataId;
    sqlResult_t *chksum;
    sqlResult_t *ownerName;
    sqlResult_t *ownerZone;
    sqlResult_t *createTime;
    sqlResult_t *modifyTime;
    sqlResult_t *rescHier;

    /* see if objPath is a dataObj */

    memset( myColl, 0, MAX_NAME_LEN );
    memset( myData, 0, MAX_NAME_LEN );

    if ( ( status = splitPathByKey(
                        dataObjInp->objPath, myColl, MAX_NAME_LEN, myData, MAX_NAME_LEN, '/' ) ) < 0 ) {
        return OBJ_PATH_DOES_NOT_EXIST;
    }

    memset( &genQueryInp, 0, sizeof( genQueryInp ) );

    snprintf( condStr, MAX_NAME_LEN, "='%s'", myColl );
    addInxVal( &genQueryInp.sqlCondInp, COL_COLL_NAME, condStr );
    snprintf( condStr, MAX_NAME_LEN, "='%s'", myData );
    addInxVal( &genQueryInp.sqlCondInp, COL_DATA_NAME, condStr );

    addInxIval( &genQueryInp.selectInp, COL_D_DATA_ID, 1 );
    addInxIval( &genQueryInp.selectInp, COL_DATA_SIZE, 1 );
    addInxIval( &genQueryInp.selectInp, COL_DATA_MODE, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_REPL_STATUS, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_DATA_CHECKSUM, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_OWNER_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_OWNER_ZONE, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_CREATE_TIME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_MODIFY_TIME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_RESC_HIER, 1 );

    genQueryInp.maxRows = MAX_SQL_ROWS;
    status =  rsGenQuery( rsComm, &genQueryInp, &genQueryOut );

    clearGenQueryInp( &genQueryInp );

    if ( status >= 0 ) {
        if ( ( dataSize = getSqlResultByInx( genQueryOut, COL_DATA_SIZE ) )
                == nullptr ) {
            rodsLog( LOG_ERROR,
                     "_rsObjStat: getSqlResultByInx for COL_DATA_SIZE failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else if ( ( dataMode = getSqlResultByInx( genQueryOut,
                               COL_DATA_MODE ) ) == nullptr ) {
            rodsLog( LOG_ERROR,
                     "_rsObjStat: getSqlResultByInx for COL_DATA_MODE failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else if ( ( replStatus = getSqlResultByInx( genQueryOut,
                                 COL_D_REPL_STATUS ) ) == nullptr ) {
            rodsLog( LOG_ERROR,
                     "_rsObjStat: getSqlResultByInx for COL_D_REPL_STATUS failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else if ( ( dataId = getSqlResultByInx( genQueryOut,
                                                COL_D_DATA_ID ) ) == nullptr ) {
            rodsLog( LOG_ERROR,
                     "_rsObjStat: getSqlResultByInx for COL_D_DATA_ID failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else if ( ( chksum = getSqlResultByInx( genQueryOut,
                                                COL_D_DATA_CHECKSUM ) ) == nullptr ) {
            rodsLog( LOG_ERROR,
                     "_rsObjStat:getSqlResultByInx for COL_D_DATA_CHECKSUM failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else if ( ( ownerName = getSqlResultByInx( genQueryOut,
                                COL_D_OWNER_NAME ) ) == nullptr ) {
            rodsLog( LOG_ERROR,
                     "_rsObjStat:getSqlResultByInx for COL_D_OWNER_NAME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else if ( ( ownerZone = getSqlResultByInx( genQueryOut,
                                COL_D_OWNER_ZONE ) ) == nullptr ) {
            rodsLog( LOG_ERROR,
                     "_rsObjStat:getSqlResultByInx for COL_D_OWNER_ZONE failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else if ( ( createTime = getSqlResultByInx( genQueryOut,
                                 COL_D_CREATE_TIME ) ) == nullptr ) {
            rodsLog( LOG_ERROR,
                     "_rsObjStat:getSqlResultByInx for COL_D_CREATE_TIME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else if ( ( modifyTime = getSqlResultByInx( genQueryOut,
                                 COL_D_MODIFY_TIME ) ) == nullptr ) {
            rodsLog( LOG_ERROR,
                     "_rsObjStat:getSqlResultByInx for COL_D_MODIFY_TIME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else if ( ( rescHier = getSqlResultByInx( genQueryOut,
                                                COL_D_RESC_HIER ) ) == nullptr ) {
            rodsLog( LOG_ERROR,
                     "_rsObjStat:getSqlResultByInx for COL_D_RESC_HIER failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else {
            int i;

            *rodsObjStatOut = ( rodsObjStat_t * ) malloc( sizeof( rodsObjStat_t ) );
            memset( *rodsObjStatOut, 0, sizeof( rodsObjStat_t ) );
            ( *rodsObjStatOut )->objType = DATA_OBJ_T;
            status = ( int )DATA_OBJ_T;
            /* XXXXXX . dont have numCopies anymore. Replaced by dataMode
             * (*rodsObjStatOut)->numCopies = genQueryOut->rowCnt; */

            for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
                if ( atoi( &replStatus->value[replStatus->len * i] ) > 0 ) {
                    rstrcpy( ( *rodsObjStatOut )->dataId,
                             &dataId->value[dataId->len * i], NAME_LEN );
                    ( *rodsObjStatOut )->objSize =
                        strtoll( &dataSize->value[dataSize->len * i], nullptr, 0 );
                    ( *rodsObjStatOut )->dataMode =
                        atoi( &dataMode->value[dataMode->len * i] );
                    rstrcpy( ( *rodsObjStatOut )->chksum,
                             &chksum->value[chksum->len * i], NAME_LEN );
                    rstrcpy( ( *rodsObjStatOut )->ownerName,
                             &ownerName->value[ownerName->len * i], NAME_LEN );
                    rstrcpy( ( *rodsObjStatOut )->ownerZone,
                             &ownerZone->value[ownerZone->len * i], NAME_LEN );
                    rstrcpy( ( *rodsObjStatOut )->createTime,
                             &createTime->value[createTime->len * i], TIME_LEN );
                    rstrcpy( ( *rodsObjStatOut )->modifyTime,
                             &modifyTime->value[modifyTime->len * i], TIME_LEN );
                    rstrcpy( ( *rodsObjStatOut )->rescHier,
                             &rescHier->value[rescHier->len * i], MAX_NAME_LEN );
                    break;
                }
            }

            if ( strlen( ( *rodsObjStatOut )->dataId ) == 0 ) {
                /* just use the first one */
                rstrcpy( ( *rodsObjStatOut )->dataId, dataId->value, NAME_LEN );
                ( *rodsObjStatOut )->objSize = strtoll( dataSize->value, nullptr, 0 );
                rstrcpy( ( *rodsObjStatOut )->chksum, chksum->value, NAME_LEN );
                rstrcpy( ( *rodsObjStatOut )->ownerName, ownerName->value,
                         NAME_LEN );
                rstrcpy( ( *rodsObjStatOut )->ownerZone, ownerZone->value,
                         NAME_LEN );
                rstrcpy( ( *rodsObjStatOut )->createTime, createTime->value,
                         TIME_LEN );
                rstrcpy( ( *rodsObjStatOut )->modifyTime, modifyTime->value,
                         TIME_LEN );
                rstrcpy( ( *rodsObjStatOut )->rescHier, rescHier->value,
                         MAX_NAME_LEN );
            }
        }
    }

    freeGenQueryOut( &genQueryOut );

    return status;
}
