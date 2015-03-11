/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* objDesc.c - L1 type operation. Will call low level l1desc drivers
 */

#include "rodsDef.h"
#include "objDesc.hpp"
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
#include "reFuncDefs.hpp"
#ifdef LOG_TRANSFERS
#include <sys/time.h>
#endif

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_stacktrace.hpp"

int
initL1desc() {
    memset( L1desc, 0, sizeof( L1desc ) );
    return 0;
}

int
allocL1desc() {
    int i;

    for ( i = 3; i < NUM_L1_DESC; i++ ) {
        if ( L1desc[i].inuseFlag <= FD_FREE ) {
            L1desc[i].inuseFlag = FD_INUSE;
            return i;
        };
    }

    rodsLog( LOG_NOTICE,
             "allocL1desc: out of L1desc" );

    return SYS_OUT_OF_FILE_DESC;
}

int
isL1descInuse() {
    int i;

    for ( i = 3; i < NUM_L1_DESC; i++ ) {
        if ( L1desc[i].inuseFlag == FD_INUSE ) {
            return 1;
        };
    }
    return 0;
}

int
initSpecCollDesc() {
    memset( SpecCollDesc, 0, sizeof( SpecCollDesc ) );
    return 0;
}

int
allocSpecCollDesc() {
    int i;

    for ( i = 1; i < NUM_SPEC_COLL_DESC; i++ ) {
        if ( SpecCollDesc[i].inuseFlag <= FD_FREE ) {
            SpecCollDesc[i].inuseFlag = FD_INUSE;
            return i;
        };
    }

    rodsLog( LOG_NOTICE,
             "allocSpecCollDesc: out of SpecCollDesc" );

    return SYS_OUT_OF_FILE_DESC;
}

int
freeSpecCollDesc( int specCollInx ) {
    if ( specCollInx < 1 || specCollInx >= NUM_SPEC_COLL_DESC ) {
        rodsLog( LOG_NOTICE,
                 "freeSpecCollDesc: specCollInx %d out of range", specCollInx );
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }

    if ( SpecCollDesc[specCollInx].dataObjInfo != NULL ) {
        freeDataObjInfo( SpecCollDesc[specCollInx].dataObjInfo );
    }

    memset( &SpecCollDesc[specCollInx], 0, sizeof( specCollDesc_t ) );

    return 0;
}

int
closeAllL1desc( rsComm_t *rsComm ) {
    int i;

    if ( rsComm == NULL ) {
        return 0;
    }
    for ( i = 3; i < NUM_L1_DESC; i++ ) {
        if ( L1desc[i].inuseFlag == FD_INUSE &&
                L1desc[i].l3descInx > 2 ) {
            l3Close( rsComm, i );
        }
    }
    return 0;
}

int
freeL1desc( int l1descInx ) {
    if ( l1descInx < 3 || l1descInx >= NUM_L1_DESC ) {
        rodsLog( LOG_NOTICE,
                 "freeL1desc: l1descInx %d out of range", l1descInx );
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }

    if ( L1desc[l1descInx].dataObjInfo != NULL ) {
        freeDataObjInfo( L1desc[l1descInx].dataObjInfo );
    }

    if ( L1desc[l1descInx].otherDataObjInfo != NULL ) {
        freeAllDataObjInfo( L1desc[l1descInx].otherDataObjInfo );
    }

    if ( L1desc[l1descInx].replDataObjInfo != NULL ) {
        freeDataObjInfo( L1desc[l1descInx].replDataObjInfo );
    }

    if ( L1desc[l1descInx].dataObjInpReplFlag == 1 &&
            L1desc[l1descInx].dataObjInp != NULL ) {
        clearDataObjInp( L1desc[l1descInx].dataObjInp );
        free( L1desc[l1descInx].dataObjInp );
    }
    memset( &L1desc[l1descInx], 0, sizeof( l1desc_t ) );

    return 0;
}

int
fillL1desc( int l1descInx, dataObjInp_t *dataObjInp,
            dataObjInfo_t *dataObjInfo, int replStatus, rodsLong_t dataSize ) {
    keyValPair_t *condInput;
    char *tmpPtr;


    char* resc_hier = getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW );
    if ( dataObjInfo->rescHier[0] == '\0' && resc_hier ) {
        snprintf( dataObjInfo->rescHier, sizeof( dataObjInfo->rescHier ), "%s", resc_hier );
    }


    condInput = &dataObjInp->condInput;
    char* in_pdmo = getValByKey( condInput, IN_PDMO_KW );
    if ( in_pdmo != NULL ) {
        rstrcpy( L1desc[l1descInx].in_pdmo, in_pdmo, MAX_NAME_LEN );
    }
    else {
        rstrcpy( L1desc[l1descInx].in_pdmo, "", MAX_NAME_LEN );
    }

    if ( dataObjInp != NULL ) {
        /* always repl the .dataObjInp */
        L1desc[l1descInx].dataObjInp = ( dataObjInp_t* )malloc( sizeof( dataObjInp_t ) );
        replDataObjInp( dataObjInp, L1desc[l1descInx].dataObjInp );
        L1desc[l1descInx].dataObjInpReplFlag = 1;
    }
    else {
        /* XXXX this can be a problem in rsDataObjClose */
        L1desc[l1descInx].dataObjInp = NULL;
    }

    L1desc[l1descInx].dataObjInfo = dataObjInfo;
    if ( dataObjInp != NULL ) {
        L1desc[l1descInx].oprType = dataObjInp->oprType;
    }
    L1desc[l1descInx].replStatus = replStatus;
    L1desc[l1descInx].dataSize = dataSize;
    if ( condInput != NULL && condInput->len > 0 ) {
        if ( ( tmpPtr = getValByKey( condInput, REG_CHKSUM_KW ) ) != NULL ) {
            L1desc[l1descInx].chksumFlag = REG_CHKSUM;
            rstrcpy( L1desc[l1descInx].chksum, tmpPtr, NAME_LEN );
        }
        else if ( ( tmpPtr = getValByKey( condInput, VERIFY_CHKSUM_KW ) ) !=
                  NULL ) {
            L1desc[l1descInx].chksumFlag = VERIFY_CHKSUM;
            rstrcpy( L1desc[l1descInx].chksum, tmpPtr, NAME_LEN );
        }
    }
#ifdef LOG_TRANSFERS
    ( void )gettimeofday( &L1desc[l1descInx].openStartTime,
                          ( struct timezone * )0 );
#endif
    return 0;
}

int
initDataObjInfoWithInp( dataObjInfo_t *dataObjInfo, dataObjInp_t *dataObjInp ) {
    char *rescName = 0, *dataType = 0, *filePath = 0;
    keyValPair_t *condInput = 0;

    condInput = &dataObjInp->condInput;
    memset( dataObjInfo, 0, sizeof( dataObjInfo_t ) );
    rstrcpy( dataObjInfo->objPath, dataObjInp->objPath, MAX_NAME_LEN );

    rescName = getValByKey( condInput, RESC_NAME_KW );
    if ( rescName != NULL ) {
        rstrcpy( dataObjInfo->rescName, rescName, NAME_LEN );
    }

    char* rescHier = getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW );
    if ( rescHier ) {
        rstrcpy( dataObjInfo->rescHier, rescHier, MAX_NAME_LEN );
    }
    else {
        rstrcpy( dataObjInfo->rescHier, rescName, MAX_NAME_LEN ); // in kw else
    }

    snprintf( dataObjInfo->dataMode, SHORT_STR_LEN, "%d", dataObjInp->createMode );

    dataType = getValByKey( condInput, DATA_TYPE_KW );
    if ( dataType != NULL ) {
        rstrcpy( dataObjInfo->dataType, dataType, NAME_LEN );
    }
    else {
        rstrcpy( dataObjInfo->dataType, "generic", NAME_LEN );
    }

    filePath = getValByKey( condInput, FILE_PATH_KW );
    if ( filePath != NULL ) {
        rstrcpy( dataObjInfo->filePath, filePath, MAX_NAME_LEN );
    }

    return 0;
}

int
getL1descIndexByDataObjInfo( const dataObjInfo_t * dataObjInfo ) {
    int index;
    for ( index = 3; index < NUM_L1_DESC; index++ ) {
        if ( L1desc[index].dataObjInfo == dataObjInfo ) {
            return index;
        }
    }
    return -1;
}

/* getNumThreads - get the number of threads.
 * inpNumThr - 0 - server decide
 *             < 0 - NO_THREADING
 *             > 0 - num of threads wanted
 */

int
getNumThreads( rsComm_t *rsComm, rodsLong_t dataSize, int inpNumThr,
               keyValPair_t *condInput, char *destRescHier, char *srcRescHier ) {
    ruleExecInfo_t rei;
    dataObjInp_t doinp;
    int status;
    int numDestThr = -1;
    int numSrcThr = -1;


    if ( inpNumThr == NO_THREADING ) {
        return 0;
    }

    if ( dataSize < 0 ) {
        return 0;
    }

    if ( dataSize <= MIN_SZ_FOR_PARA_TRAN ) {
        if ( inpNumThr > 0 ) {
            inpNumThr = 1;
        }
        else {
            return 0;
        }
    }

    if ( getValByKey( condInput, NO_PARA_OP_KW ) != NULL ) {
        /* client specify no para opr */
        return 1;
    }

    memset( &doinp, 0, sizeof( doinp ) );
    doinp.numThreads = inpNumThr;

    doinp.dataSize = dataSize;

    initReiWithDataObjInp( &rei, rsComm, &doinp );

    if ( destRescHier != NULL ) {

        // get resource (hierarchy) location
        std::string location;
        irods::error ret = irods::get_loc_for_hier_string( destRescHier, location );
        if ( !ret.ok() ) {
            irods::log( PASSMSG( "getNumThreads - failed in get_loc_for_hier_string", ret ) );
            return -1;
        }

        irods::hierarchy_parser parser;
        parser.set_string( destRescHier );

        std::string last_resc;
        parser.last_resc( last_resc );

        irods::error err = irods::is_resc_live( last_resc.c_str() );
        if ( err.ok() ) {
            status = applyRule( "acSetNumThreads", NULL, &rei, NO_SAVE_REI );

            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "getNumThreads: acGetNumThreads error, status = %d",
                         status );
            }
            else {
                numDestThr = rei.status;
                if ( numDestThr == 0 ) {
                    return 0;
                }
                else if ( numDestThr == 1 && srcRescHier == NULL &&
                          isLocalHost( location.c_str() ) ) {
                    /* one thread and resource on local host */
                    return 0;
                }
            }
        }
    }

    if ( destRescHier != NULL && srcRescHier != NULL ) {
        if ( numDestThr > 0 && strcmp( destRescHier, srcRescHier ) == 0 ) {
            return numDestThr;
        }

        // get resource (hierarchy) location
        std::string location;
        irods::error ret = irods::get_loc_for_hier_string( destRescHier, location );
        if ( !ret.ok() ) {
            irods::log( PASSMSG( "getNumThreads - failed in get_loc_for_hier_string", ret ) );
            return -1;
        }

        irods::hierarchy_parser parser;
        parser.set_string( srcRescHier );

        std::string last_resc;
        parser.last_resc( last_resc );

        irods::error err = irods::is_resc_live( last_resc.c_str() );

        if ( err.ok() ) {
            status = applyRule( "acSetNumThreads", NULL, &rei, NO_SAVE_REI );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "getNumThreads: acGetNumThreads error, status = %d",
                         status );
            }
            else {
                numSrcThr = rei.status;
                if ( numSrcThr == 0 ) {
                    return 0;
                }
            }
        }
    }

    if ( numDestThr > 0 ) {
        if ( getValByKey( condInput, RBUDP_TRANSFER_KW ) != NULL ) {
            return 1;
        }
        else {
            return numDestThr;
        }
    }
    if ( numSrcThr > 0 ) {
        if ( getValByKey( condInput, RBUDP_TRANSFER_KW ) != NULL ) {
            return 1;
        }
        else {
            return numSrcThr;
        }
    }
    /* should not be here. do one with no resource */
    status = applyRule( "acSetNumThreads", NULL, &rei, NO_SAVE_REI );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "getNumThreads: acGetNumThreads error, status = %d",
                 status );
        return 0;
    }
    else {
        if ( rei.status > 0 ) {
            return rei.status;
        }
        else {
            return 0;
        }
    }
}

int
initDataOprInp( dataOprInp_t *dataOprInp, int l1descInx, int oprType ) {
    dataObjInfo_t *dataObjInfo;
    dataObjInp_t  *dataObjInp;
    char *tmpStr;


    dataObjInfo = L1desc[l1descInx].dataObjInfo;
    dataObjInp = L1desc[l1descInx].dataObjInp;

    memset( dataOprInp, 0, sizeof( dataOprInp_t ) );

    dataOprInp->oprType = oprType;
    dataOprInp->numThreads = dataObjInp->numThreads;
    dataOprInp->offset = dataObjInp->offset;
    if ( oprType == PUT_OPR ) {
        if ( dataObjInp->dataSize > 0 ) {
            dataOprInp->dataSize = dataObjInp->dataSize;
        }
        dataOprInp->destL3descInx = L1desc[l1descInx].l3descInx;
    }
    else if ( oprType == GET_OPR ) {
        if ( dataObjInfo->dataSize > 0 ) {
            dataOprInp->dataSize = dataObjInfo->dataSize;
        }
        else {
            dataOprInp->dataSize = dataObjInp->dataSize;
        }
        dataOprInp->srcL3descInx = L1desc[l1descInx].l3descInx;
    }
    else if ( oprType == SAME_HOST_COPY_OPR ) {
        int srcL1descInx = L1desc[l1descInx].srcL1descInx;
        int srcL3descInx = L1desc[srcL1descInx].l3descInx;
        dataOprInp->dataSize = L1desc[srcL1descInx].dataObjInfo->dataSize;
        dataOprInp->destL3descInx = L1desc[l1descInx].l3descInx;
        dataOprInp->srcL3descInx = srcL3descInx;
    }
    else if ( oprType == COPY_TO_REM_OPR ) {
        int srcL1descInx = L1desc[l1descInx].srcL1descInx;
        int srcL3descInx = L1desc[srcL1descInx].l3descInx;
        dataOprInp->dataSize = L1desc[srcL1descInx].dataObjInfo->dataSize;
        dataOprInp->srcL3descInx = srcL3descInx;
    }
    else if ( oprType == COPY_TO_LOCAL_OPR ) {
        int srcL1descInx = L1desc[l1descInx].srcL1descInx;
        dataOprInp->dataSize = L1desc[srcL1descInx].dataObjInfo->dataSize;
        dataOprInp->destL3descInx = L1desc[l1descInx].l3descInx;
    }
    if ( getValByKey( &dataObjInp->condInput, STREAMING_KW ) != NULL ) {
        addKeyVal( &dataOprInp->condInput, STREAMING_KW, "" );
    }

    if ( getValByKey( &dataObjInp->condInput, NO_PARA_OP_KW ) != NULL ) {
        addKeyVal( &dataOprInp->condInput, NO_PARA_OP_KW, "" );
    }

    if ( getValByKey( &dataObjInp->condInput, RBUDP_TRANSFER_KW ) != NULL ) {

        /* only do unix fs */
        // JMC - legacy resource - int rescTypeInx = dataObjInfo->rescInfo->rescTypeInx;
        // JMC - legacy resource - if (RescTypeDef[rescTypeInx].driverType == UNIX_FILE_TYPE)
        std::string type;
        irods::error err = irods::get_resc_type_for_hier_string( dataObjInfo->rescHier, type );

        if ( !err.ok() ) {
            irods::log( PASS( err ) );
        }
        else {
            if ( irods::RESOURCE_TYPE_NATIVE == type ) { // JMC ::
                addKeyVal( &dataOprInp->condInput, RBUDP_TRANSFER_KW, "" );
            }
        }
    }

    if ( getValByKey( &dataObjInp->condInput, VERY_VERBOSE_KW ) != NULL ) {
        addKeyVal( &dataOprInp->condInput, VERY_VERBOSE_KW, "" );
    }

    if ( ( tmpStr = getValByKey( &dataObjInp->condInput, RBUDP_SEND_RATE_KW ) ) !=
            NULL ) {
        addKeyVal( &dataOprInp->condInput, RBUDP_SEND_RATE_KW, tmpStr );
    }

    if ( ( tmpStr = getValByKey( &dataObjInp->condInput, RBUDP_PACK_SIZE_KW ) ) !=
            NULL ) {
        addKeyVal( &dataOprInp->condInput, RBUDP_PACK_SIZE_KW, tmpStr );
    }

    return 0;
}

int
initDataObjInfoForRepl(
    dataObjInfo_t* destDataObjInfo,
    dataObjInfo_t* srcDataObjInfo,
    const char*	   _resc_name ) {


    memset( destDataObjInfo, 0, sizeof( dataObjInfo_t ) );
    *destDataObjInfo = *srcDataObjInfo;
    memset( &destDataObjInfo->condInput, 0, sizeof( destDataObjInfo->condInput ) );
    replKeyVal( &srcDataObjInfo->condInput, &destDataObjInfo->condInput );

    destDataObjInfo->filePath[0] = '\0';
    rstrcpy( destDataObjInfo->rescName, _resc_name, NAME_LEN );

    // initialize the destination resource hierarchy to the root resource
    rstrcpy( destDataObjInfo->rescHier, _resc_name, MAX_NAME_LEN ); // orphan right now


    destDataObjInfo->replNum = destDataObjInfo->dataId = 0;

    return 0;
}

int
convL3descInx( int l3descInx ) {
    if ( l3descInx <= 2 || FileDesc[l3descInx].inuseFlag == 0 ||
            FileDesc[l3descInx].rodsServerHost == NULL ) {
        return l3descInx;
    }

    if ( FileDesc[l3descInx].rodsServerHost->localFlag == LOCAL_HOST ) {
        return l3descInx;
    }
    else {
        return FileDesc[l3descInx].fd;
    }
}

int
initCollHandle() {
    memset( CollHandle, 0, sizeof( CollHandle ) );
    return 0;
}

int
allocCollHandle() {
    int i;

    for ( i = 0; i < NUM_COLL_HANDLE; i++ ) {
        if ( CollHandle[i].inuseFlag <= FD_FREE ) {
            CollHandle[i].inuseFlag = FD_INUSE;
            return i;
        };
    }

    rodsLog( LOG_NOTICE,
             "allocCollHandle: out of CollHandle" );

    return SYS_OUT_OF_FILE_DESC;
}

int
freeCollHandle( int handleInx ) {
    if ( handleInx < 0 || handleInx >= NUM_COLL_HANDLE ) {
        rodsLog( LOG_NOTICE,
                 "freeCollHandle: handleInx %d out of range", handleInx );
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }

    /* don't free specColl. It is in cache */
    clearCollHandle( &CollHandle[handleInx], 1 );
    memset( &CollHandle[handleInx], 0, sizeof( collHandle_t ) );

    return 0;
}

int
rsInitQueryHandle( queryHandle_t *queryHandle, rsComm_t *rsComm ) {
    if ( queryHandle == NULL || rsComm == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    queryHandle->conn = rsComm;
    queryHandle->connType = RS_COMM;
    queryHandle->querySpecColl = ( funcPtr ) rsQuerySpecColl;
    queryHandle->genQuery = ( funcPtr ) rsGenQuery;

    return 0;
}

int
allocAndSetL1descForZoneOpr( int remoteL1descInx, dataObjInp_t *dataObjInp,
                             rodsServerHost_t *remoteZoneHost, openStat_t *openStat ) {
    int l1descInx;
    dataObjInfo_t *dataObjInfo;

    l1descInx = allocL1desc();
    if ( l1descInx < 0 ) {
        return l1descInx;
    }
    L1desc[l1descInx].remoteL1descInx = remoteL1descInx;
    L1desc[l1descInx].oprType = REMOTE_ZONE_OPR;
    L1desc[l1descInx].remoteZoneHost = remoteZoneHost;
    /* always repl the .dataObjInp */
    L1desc[l1descInx].dataObjInp = ( dataObjInp_t* )malloc( sizeof( dataObjInp_t ) );
    replDataObjInp( dataObjInp, L1desc[l1descInx].dataObjInp );
    L1desc[l1descInx].dataObjInpReplFlag = 1;
    dataObjInfo = L1desc[l1descInx].dataObjInfo =
                      ( dataObjInfo_t* )malloc( sizeof( dataObjInfo_t ) );
    bzero( dataObjInfo, sizeof( dataObjInfo_t ) );
    rstrcpy( dataObjInfo->objPath, dataObjInp->objPath, MAX_NAME_LEN );

    if ( openStat != NULL ) {
        dataObjInfo->dataSize = openStat->dataSize;
        rstrcpy( dataObjInfo->dataMode, openStat->dataMode, SHORT_STR_LEN );
        rstrcpy( dataObjInfo->dataType, openStat->dataType, NAME_LEN );
        L1desc[l1descInx].l3descInx = openStat->l3descInx;
        L1desc[l1descInx].replStatus = openStat->replStatus;
    }

    return l1descInx;
}

