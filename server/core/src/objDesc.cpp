#include "irods/rcMisc.h"
#include "irods/rodsDef.h"
#include "irods/objDesc.hpp"
#include "irods/dataObjOpr.hpp"
#include "irods/rodsDef.h"
#include "irods/rsGlobalExtern.hpp"
#include "irods/fileChksum.h"
#include "irods/modDataObjMeta.h"
#include "irods/objMetaOpr.hpp"
#include "irods/collection.hpp"
#include "irods/resource.hpp"
#include "irods/dataObjClose.h"
#include "irods/rcGlobalExtern.h"
#include "irods/genQuery.h"
#include "irods/rsGenQuery.hpp"
#include "irods/rsGetHierFromLeafId.hpp"
#include "irods/rsQuerySpecColl.hpp"
#include "irods/rsDataObjClose.hpp"
#include "irods/irods_resource_backport.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/get_hier_from_leaf_id.h"
#include "irods/key_value_proxy.hpp"
#include "irods/replica_proxy.hpp"

#include <cstring>

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
            close( FileDesc[L1desc[i].l3descInx].fd );
        }
    }
    return 0;
}

int freeL1desc_struct(l1desc& _l1desc)
{
    if (_l1desc.dataObjInfo) {
        freeDataObjInfo(_l1desc.dataObjInfo);
        // TODO: potential leak - but crashes when 'all' version used... investigate this
        //freeAllDataObjInfo(_l1desc.dataObjInfo);
    }

    if (_l1desc.otherDataObjInfo) {
        freeAllDataObjInfo(_l1desc.otherDataObjInfo);
    }

    if (_l1desc.replDataObjInfo) {
        freeDataObjInfo(_l1desc.replDataObjInfo);
        //freeAllDataObjInfo(_l1desc.replDataObjInfo);
    }

    if (_l1desc.dataObjInpReplFlag == 1 && _l1desc.dataObjInp) {
        clearDataObjInp(_l1desc.dataObjInp);
        free(_l1desc.dataObjInp);
    }

    _l1desc.replica_token.clear();

    memset(&_l1desc, 0, sizeof(l1desc));

    return 0;
} // freeL1desc

int freeL1desc(const int l1descInx) {
    if ( l1descInx < 3 || l1descInx >= NUM_L1_DESC ) {
        rodsLog( LOG_NOTICE, "freeL1desc: l1descInx %d out of range", l1descInx );
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }

    return freeL1desc_struct(L1desc[l1descInx]);
} // freeL1desc

int
fillL1desc( int l1descInx, dataObjInp_t *dataObjInp,
            dataObjInfo_t *dataObjInfo, int replStatus, rodsLong_t dataSize ) {
    keyValPair_t *condInput;
    char *tmpPtr;

    // Initialize the bytesWritten to -1 rather than 0.  If this is negative then we
    // know no bytes have been written.  This is so that zero length files can be handled
    // similarly to non-zero length files.
    L1desc[l1descInx].bytesWritten = -1;

    char* resc_hier = getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW );
    if ( dataObjInfo->rescHier[0] == '\0' && resc_hier ) {
        rstrcpy( dataObjInfo->rescHier, resc_hier, MAX_NAME_LEN );
        if (dataObjInfo->rescName[0] == '\0') {
            const std::string root = irods::hierarchy_parser{resc_hier}.first_resc();
            rstrcpy( dataObjInfo->rescName, root.c_str(), NAME_LEN );
        }
    }

    condInput = &dataObjInp->condInput;
    char* in_pdmo = getValByKey( condInput, IN_PDMO_KW );
    if ( in_pdmo != NULL ) {
        rstrcpy( L1desc[l1descInx].in_pdmo, in_pdmo, MAX_NAME_LEN );
    }
    else {
        rstrcpy( L1desc[l1descInx].in_pdmo, "", MAX_NAME_LEN );
    }

    const auto open_type{getValByKey(condInput, OPEN_TYPE_KW)};
    if (open_type) {
        L1desc[l1descInx].openType = std::atoi(open_type);
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

    if (getValByKey(&dataObjInp->condInput, PURGE_CACHE_KW)) {
        L1desc[l1descInx].purgeCacheFlag = 1;
    }

    const char* kvp_str = getValByKey(&dataObjInp->condInput, KEY_VALUE_PASSTHROUGH_KW);
    if (kvp_str) {
        addKeyVal(&dataObjInfo->condInput, KEY_VALUE_PASSTHROUGH_KW, kvp_str);
    }

    return 0;
}

int
initDataObjInfoWithInp( dataObjInfo_t *dataObjInfo, dataObjInp_t *dataObjInp ) {
    namespace ix = irods::experimental;

    if (!dataObjInp || !dataObjInfo) {
        rodsLog(LOG_ERROR, "[%s] - null input", __FUNCTION__);
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
    auto kvp = ix::make_key_value_proxy(dataObjInp->condInput);
    memset( dataObjInfo, 0, sizeof( dataObjInfo_t ) );

    rstrcpy( dataObjInfo->objPath, dataObjInp->objPath, MAX_NAME_LEN );

    if (kvp.contains(DATA_ID_KW)) {
        dataObjInfo->dataId = std::atoll(kvp.at(DATA_ID_KW).value().data());
    }

    if (kvp.contains(RESC_NAME_KW)) {
        const auto resc_name = kvp.at(RESC_NAME_KW).value();
        rstrcpy(dataObjInfo->rescName, resc_name.data(), NAME_LEN);
        if (!kvp.contains(RESC_HIER_STR_KW)) {
            rstrcpy( dataObjInfo->rescHier, resc_name.data(), MAX_NAME_LEN );
        }
    }

    if (kvp.contains(RESC_HIER_STR_KW)) {
        auto hier = kvp.at(RESC_HIER_STR_KW).value();
        rstrcpy(dataObjInfo->rescHier, hier.data(), MAX_NAME_LEN);
    }

    irods::error ret = resc_mgr.hier_to_leaf_id(dataObjInfo->rescHier,dataObjInfo->rescId);
    if( !ret.ok() ) {
        irods::log(PASS(ret));
    }

    snprintf( dataObjInfo->dataMode, SHORT_STR_LEN, "%d", dataObjInp->createMode );

    if (kvp.contains(DATA_TYPE_KW)) {
        auto data_type = kvp.at(DATA_TYPE_KW).value();
        rstrcpy(dataObjInfo->dataType, data_type.data(), NAME_LEN);
    }
    else {
        rstrcpy(dataObjInfo->dataType, "generic", NAME_LEN);
    }

    if (kvp.contains(FILE_PATH_KW)) {
        auto file_path = kvp.at(FILE_PATH_KW).value();
        rstrcpy( dataObjInfo->filePath, file_path.data(), MAX_NAME_LEN );
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
               keyValPair_t *condInput, char *destRescHier, char *srcRescHier, int oprType ) {
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
    doinp.oprType = oprType;

    initReiWithDataObjInp( &rei, rsComm, &doinp );

    if (destRescHier && strlen(destRescHier)) {

        // get resource (hierarchy) location
        std::string location;
        irods::error ret = irods::get_loc_for_hier_string( destRescHier, location );
        if ( !ret.ok() ) {
            irods::log( PASSMSG( "getNumThreads - failed in get_loc_for_hier_string", ret ) );
            clearKeyVal(rei.condInputData);
            free(rei.condInputData);
            return -1;
        }

        irods::error err = irods::is_hier_live( destRescHier );
        if ( err.ok() ) {
            // fill rei.condInputData with resource properties
            ret = irods::get_resc_properties_as_kvp(destRescHier, rei.condInputData);
            if ( !ret.ok() ) {
                irods::log( PASSMSG( "getNumThreads - failed in get_resc_properties_as_kvp", ret ) );
            }

            // PEP
            status = applyRule( "acSetNumThreads", NULL, &rei, NO_SAVE_REI );

            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "getNumThreads: acSetNumThreads error, status = %d",
                         status );
            }
            else {

                numDestThr = rei.status;
                if ( numDestThr == 0 ) {
                    clearKeyVal(rei.condInputData);
                    free(rei.condInputData);
                    return 0;
                }
                else if ( numDestThr == 1 && srcRescHier == NULL &&
                          isLocalHost( location.c_str() ) ) {
                    /* one thread and resource on local host */
                    clearKeyVal(rei.condInputData);
                    free(rei.condInputData);
                    return 0;
                }
            }
        }
    }

    if (destRescHier && strlen(destRescHier) && srcRescHier && strlen(srcRescHier)) {
        if ( numDestThr > 0 && strcmp( destRescHier, srcRescHier ) == 0 ) {
            clearKeyVal(rei.condInputData);
            free(rei.condInputData);

            return numDestThr;
        }

        // get resource (hierarchy) location
        std::string location;
        irods::error ret = irods::get_loc_for_hier_string( destRescHier, location );
        if ( !ret.ok() ) {
            irods::log( PASSMSG( "getNumThreads - failed in get_loc_for_hier_string", ret ) );
            clearKeyVal(rei.condInputData);
            free(rei.condInputData);

            return -1;
        }

        irods::error err = irods::is_hier_live( srcRescHier );
        if ( err.ok() ) {
            // fill rei.condInputData with resource properties
            ret = irods::get_resc_properties_as_kvp(destRescHier, rei.condInputData);
            if ( !ret.ok() ) {
                irods::log( PASSMSG( "getNumThreads - failed in get_resc_properties_as_kvp", ret ) );
            }

            // PEP
            status = applyRule( "acSetNumThreads", NULL, &rei, NO_SAVE_REI );

            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "getNumThreads: acSetNumThreads error, status = %d",
                         status );
            }
            else {
                numSrcThr = rei.status;
                if ( numSrcThr == 0 ) {
                    clearKeyVal(rei.condInputData);
                    free(rei.condInputData);

                    return 0;
                }
            }
        }
    }

    if ( numDestThr > 0 ) {
        clearKeyVal(rei.condInputData);
        free(rei.condInputData);
        if ( getValByKey( condInput, RBUDP_TRANSFER_KW ) != NULL ) {
            return 1;
        }
        else {
            return numDestThr;
        }
    }
    if ( numSrcThr > 0 ) {
        clearKeyVal(rei.condInputData);
        free(rei.condInputData);
        if ( getValByKey( condInput, RBUDP_TRANSFER_KW ) != NULL ) {
            return 1;
        }
        else {
            return numSrcThr;
        }
    }
    /* should not be here. do one with no resource */
    status = applyRule( "acSetNumThreads", NULL, &rei, NO_SAVE_REI );
    clearKeyVal(rei.condInputData);
    free(rei.condInputData);
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

int allocCollHandle() {
    // look for a free collHandle_t
    for (std::vector<collHandle_t>::iterator it = CollHandle.begin(); it != CollHandle.end(); ++it) {
    	if (it->inuseFlag <= FD_FREE) {
    		it->inuseFlag = FD_INUSE;
    		return it - CollHandle.begin();
    	}
    }

    // if none found make a new one
    collHandle_t my_coll_handle;
    memset(&my_coll_handle, 0, sizeof(collHandle_t));

    // mark as in use
    my_coll_handle.inuseFlag = FD_INUSE;

    // add to vector
    CollHandle.push_back(my_coll_handle);

    // return index
    return CollHandle.size() - 1;
}

int freeCollHandle( int handleInx ) {
    if ( handleInx < 0 || static_cast<std::size_t>(handleInx) >= CollHandle.size() ) {
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
    queryHandle->getHierForId = ( funcPtr ) rsGetHierFromLeafId;

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
    std::memset(dataObjInfo, 0, sizeof(dataObjInfo_t));
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

namespace irods
{
    auto populate_L1desc_with_inp(
        DataObjInp& _inp,
        DataObjInfo& _info,
        const rodsLong_t dataSize) -> int
    {
        namespace ir = irods::experimental::replica;

        int l1_index = allocL1desc();
        if (l1_index < 0) {
            THROW(l1_index, fmt::format("[{}] - failed to allocate l1 descriptor", __FUNCTION__));
        }

        auto& l1desc = L1desc[l1_index];

        // Initialize the bytesWritten to -1 rather than 0.  If this is negative then we
        // know no bytes have been written.  This is so that zero length files can be handled
        // similarly to non-zero length files.
        l1desc.bytesWritten = -1;

        irods::experimental::key_value_proxy cond_input{_inp.condInput};

        if (cond_input.contains(IN_PDMO_KW)) {
            rstrcpy(l1desc.in_pdmo, cond_input.at(IN_PDMO_KW).value().data(), MAX_NAME_LEN );
        }
        else {
            rstrcpy(l1desc.in_pdmo, "", MAX_NAME_LEN );
        }

        if (cond_input.contains(OPEN_TYPE_KW)) {
            l1desc.openType = std::stoi(cond_input.at(OPEN_TYPE_KW).value().data());
        }

        l1desc.dataObjInp = static_cast<DataObjInp*>(std::malloc(sizeof(DataObjInp)));
        std::memset(l1desc.dataObjInp, 0, sizeof(DataObjInp));
        replDataObjInp(&_inp, l1desc.dataObjInp);
        l1desc.dataObjInp->dataSize = dataSize;

        auto [duplicate_replica, duplicate_replica_lm] = ir::duplicate_replica(_info);
        l1desc.dataObjInfo = duplicate_replica_lm.release();

        l1desc.dataObjInpReplFlag = 1;
        l1desc.oprType = _inp.oprType;
        l1desc.replStatus = duplicate_replica.replica_status();
        l1desc.dataSize = dataSize;
        l1desc.purgeCacheFlag = static_cast<int>(cond_input.contains(PURGE_CACHE_KW));

        if (cond_input.contains(REG_CHKSUM_KW)) {
            l1desc.chksumFlag = REG_CHKSUM;
            std::snprintf(l1desc.chksum, sizeof(l1desc.chksum), "%s", cond_input.at(REG_CHKSUM_KW).value().data());
        }
        else if (cond_input.contains(VERIFY_CHKSUM_KW)) {
            l1desc.chksumFlag = VERIFY_CHKSUM;
            std::snprintf(l1desc.chksum, sizeof(l1desc.chksum), "%s", cond_input.at(VERIFY_CHKSUM_KW).value().data());
        }

        if (cond_input.contains(KEY_VALUE_PASSTHROUGH_KW)) {
            duplicate_replica.cond_input()[KEY_VALUE_PASSTHROUGH_KW] = cond_input.at(KEY_VALUE_PASSTHROUGH_KW);
        }

        return l1_index;
    } // populate_L1desc_with_inp
} // namespace irods
