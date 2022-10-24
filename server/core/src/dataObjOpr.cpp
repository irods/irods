#include "irods/dataObjOpr.hpp"

#include "irods/rcMisc.h"
#include "irods/objMetaOpr.hpp"
#include "irods/resource.hpp"
#include "irods/collection.hpp"
#include "irods/specColl.hpp"
#include "irods/physPath.hpp"
#include "irods/modDataObjMeta.h"
#include "irods/ruleExecSubmit.h"
#include "irods/ruleExecDel.h"
#include "irods/genQuery.h"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/miscUtil.h"
#include "irods/rsIcatOpr.hpp"
#include "irods/getHierarchyForResc.h"
#include "irods/dataObjTrim.h"
#include "irods/rsGenQuery.hpp"
#include "irods/rsDataObjTrim.hpp"
#include "irods/rsModDataObjMeta.hpp"
#include "irods/irods_resource_backport.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/irods_log.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/irods_random.hpp"
#include "irods/irods_file_object.hpp"
#include "irods/rodsPath.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <fmt/format.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <algorithm>

using namespace boost::filesystem;

/// @brief Function which determines if a logical path is created at the root level.
irods::error validate_logical_path(const std::string& _path)
{
    std::string zone_name;

    // Allocate enough bytes for the maximum size of a zone name.
    // It is defined in R_ZONE_MAIN. We add one extra byte for the
    // root collection "/".
    zone_name.reserve(251);

    // Loop over the ZoneInfo linked list and see if the path has a root
    // object which matches any zone.
    for (const zoneInfo_t* zone_info = ZoneInfoHead; zone_info; zone_info = zone_info->next) {
        // Build a root zone name
        zone_name.clear();
        zone_name += '/';
        zone_name += zone_info->zoneName;

        // If the zone name appears at the root then this is a good path.
        if (has_prefix(_path.data(), zone_name.data())) {
            return SUCCESS();
        }
    }

    constexpr const char* const msg = "a valid zone name does not appear at the root of the object path [{}]";
    return ERROR(SYS_INVALID_INPUT_PARAM, fmt::format(msg, _path));
} // validate_logical_path

int
getDataObjInfo(
    rsComm_t*       rsComm,
    dataObjInp_t*   dataObjInp,
    dataObjInfo_t** dataObjInfoHead,
    char*           accessPerm,
    int             ignoreCondInput ) {

    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    int i, status;
    dataObjInfo_t *dataObjInfo;
    char *tmpStr;
    sqlResult_t *dataId, *collId, *replNum, *version, *dataType, *dataSize,
                *hierString, *filePath, *dataOwnerName, *dataOwnerZone,
                *replStatus, *statusString, *chksum, *dataExpiry, *dataMapId,
                *dataComments, *dataCreate, *dataModify, *dataMode, *dataName, *collName;
    char *tmpDataId, *tmpCollId, *tmpReplNum, *tmpVersion, *tmpDataType,
         *tmpDataSize, *tmpFilePath,
         *tmpDataOwnerName, *tmpDataOwnerZone, *tmpReplStatus, *tmpStatusString,
         *tmpChksum, *tmpDataExpiry, *tmpDataMapId, *tmpDataComments,
         *tmpDataCreate, *tmpDataModify, *tmpDataMode, *tmpDataName, *tmpCollName,
         *tmpHierString;

    char accStr[LONG_NAME_LEN];
    int qcondCnt;
    int writeFlag;

    *dataObjInfoHead = NULL;

    qcondCnt = initDataObjInfoQuery( dataObjInp, &genQueryInp,
                                     ignoreCondInput );

    if ( qcondCnt < 0 ) {
        return qcondCnt;
    }

    addInxIval( &genQueryInp.selectInp, COL_D_DATA_ID, 1 );
    addInxIval( &genQueryInp.selectInp, COL_DATA_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_COLL_ID, 1 );
    addInxIval( &genQueryInp.selectInp, COL_DATA_REPL_NUM, 1 );
    addInxIval( &genQueryInp.selectInp, COL_DATA_VERSION, 1 );
    addInxIval( &genQueryInp.selectInp, COL_DATA_TYPE_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_DATA_SIZE, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_DATA_PATH, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_OWNER_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_OWNER_ZONE, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_REPL_STATUS, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_DATA_STATUS, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_DATA_CHECKSUM, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_EXPIRY, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_MAP_ID, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_COMMENTS, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_CREATE_TIME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_MODIFY_TIME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_DATA_MODE, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_RESC_HIER, 1 );

    if ( accessPerm != NULL ) {
        snprintf( accStr, LONG_NAME_LEN, "%s", rsComm->clientUser.userName );
        addKeyVal( &genQueryInp.condInput, USER_NAME_CLIENT_KW, accStr );

        snprintf( accStr, LONG_NAME_LEN, "%s", rsComm->clientUser.rodsZone );
        addKeyVal( &genQueryInp.condInput, RODS_ZONE_CLIENT_KW, accStr );

        snprintf( accStr, LONG_NAME_LEN, "%s", accessPerm );
        addKeyVal( &genQueryInp.condInput, ACCESS_PERMISSION_KW, accStr );
    }
    if ( ( tmpStr = getValByKey( &dataObjInp->condInput, TICKET_KW ) ) != NULL ) {
        addKeyVal( &genQueryInp.condInput, TICKET_KW, tmpStr );
    }

    genQueryInp.maxRows = MAX_SQL_ROWS;

    status = rsGenQuery( rsComm, &genQueryInp, &genQueryOut );

    if ( status < 0 ) {
        if ( status != CAT_NO_ROWS_FOUND ) {
            rodsLog( LOG_NOTICE,
                     "%s: rsGenQuery error, status = %d", __FUNCTION__,
                     status );
        }

        genQueryInp.continueInx = genQueryOut->continueInx;
        genQueryInp.maxRows = 0;
        freeGenQueryOut( &genQueryOut );
        rsGenQuery( rsComm, &genQueryInp, &genQueryOut );

        freeGenQueryOut( &genQueryOut );
        clearGenQueryInp( &genQueryInp );

        return status;
    }

    clearGenQueryInp( &genQueryInp );

    if ( genQueryOut == NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: NULL genQueryOut", __FUNCTION__);
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( ( dataOwnerName =
                getSqlResultByInx( genQueryOut, COL_D_OWNER_NAME ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_D_OWNER_NAME failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( dataName =
                getSqlResultByInx( genQueryOut, COL_DATA_NAME ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_DATA_NAME failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( collName =
                getSqlResultByInx( genQueryOut, COL_COLL_NAME ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_COLL_NAME failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( dataId = getSqlResultByInx( genQueryOut, COL_D_DATA_ID ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_D_DATA_ID failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( collId = getSqlResultByInx( genQueryOut, COL_D_COLL_ID ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_D_COLL_ID failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( replNum = getSqlResultByInx( genQueryOut, COL_DATA_REPL_NUM ) ) ==
            NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_DATA_REPL_NUM failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( version = getSqlResultByInx( genQueryOut, COL_DATA_VERSION ) ) ==
            NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_DATA_VERSION failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( dataType = getSqlResultByInx( genQueryOut, COL_DATA_TYPE_NAME ) ) ==
            NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_DATA_TYPE_NAME failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( dataSize = getSqlResultByInx( genQueryOut, COL_DATA_SIZE ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_DATA_SIZE failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( hierString = getSqlResultByInx( genQueryOut, COL_D_RESC_HIER ) ) ==
            NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_D_RESC_HIER failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( filePath = getSqlResultByInx( genQueryOut, COL_D_DATA_PATH ) ) ==
            NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_D_DATA_PATH failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( dataOwnerZone =
                getSqlResultByInx( genQueryOut, COL_D_OWNER_ZONE ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_D_OWNER_ZONE failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( replStatus =
                getSqlResultByInx( genQueryOut, COL_D_REPL_STATUS ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_D_REPL_STATUS failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( statusString =
                getSqlResultByInx( genQueryOut, COL_D_DATA_STATUS ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_D_DATA_STATUS failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( chksum =
                getSqlResultByInx( genQueryOut, COL_D_DATA_CHECKSUM ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_D_DATA_CHECKSUM failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( dataExpiry =
                getSqlResultByInx( genQueryOut, COL_D_EXPIRY ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_D_EXPIRY failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( dataMapId =
                getSqlResultByInx( genQueryOut, COL_D_MAP_ID ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_D_MAP_ID failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( dataComments =
                getSqlResultByInx( genQueryOut, COL_D_COMMENTS ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_D_COMMENTS failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( dataCreate =
                getSqlResultByInx( genQueryOut, COL_D_CREATE_TIME ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_D_CREATE_TIME failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( dataModify =
                getSqlResultByInx( genQueryOut, COL_D_MODIFY_TIME ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_D_MODIFY_TIME failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( dataMode =
                getSqlResultByInx( genQueryOut, COL_DATA_MODE ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "%s: getSqlResultByInx for COL_DATA_MODE failed", __FUNCTION__);
        return UNMATCHED_KEY_OR_INDEX;
    }

    writeFlag = getWriteFlag( dataObjInp->openFlags );

    for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
        dataObjInfo = ( dataObjInfo_t * ) malloc( sizeof( dataObjInfo_t ) );
        memset( dataObjInfo, 0, sizeof( dataObjInfo_t ) );

        tmpDataId = &dataId->value[dataId->len * i];
        tmpCollId = &collId->value[collId->len * i];
        tmpReplNum = &replNum->value[replNum->len * i];
        tmpVersion = &version->value[version->len * i];
        tmpDataType = &dataType->value[dataType->len * i];
        tmpDataSize = &dataSize->value[dataSize->len * i];
        tmpHierString = &hierString->value[hierString->len * i];

        tmpFilePath = &filePath->value[filePath->len * i];
        tmpDataOwnerName = &dataOwnerName->value[dataOwnerName->len * i];
        tmpDataOwnerZone = &dataOwnerZone->value[dataOwnerZone->len * i];
        tmpReplStatus = &replStatus->value[replStatus->len * i];
        tmpStatusString = &statusString->value[statusString->len * i];
        tmpChksum = &chksum->value[chksum->len * i];
        tmpDataExpiry = &dataExpiry->value[dataExpiry->len * i];
        tmpDataMapId = &dataMapId->value[dataMapId->len * i];
        tmpDataComments = &dataComments->value[dataComments->len * i];
        tmpDataCreate = &dataCreate->value[dataCreate->len * i];
        tmpDataModify = &dataModify->value[dataModify->len * i];
        tmpDataMode = &dataMode->value[dataMode->len * i];
        tmpDataName = &dataName->value[dataName->len * i];
        tmpCollName = &collName->value[collName->len * i];

        snprintf( dataObjInfo->objPath, MAX_NAME_LEN, "%s/%s", tmpCollName, tmpDataName );
        rstrcpy( dataObjInfo->rescHier, tmpHierString, MAX_NAME_LEN );
        irods::error ret = resc_mgr.hier_to_leaf_id( tmpHierString, dataObjInfo->rescId );
        if (!ret.ok()) {
            freeDataObjInfo(dataObjInfo);
            irods::log(PASS(ret));
            return ret.code();
        }
        std::string hier( tmpHierString );

        irods::hierarchy_parser parser;
        parser.set_string( tmpHierString );
        std::string leaf;
        parser.last_resc( leaf );
        rstrcpy( dataObjInfo->rescName, leaf.c_str(), NAME_LEN );

        rstrcpy( dataObjInfo->dataType, tmpDataType, NAME_LEN );
        dataObjInfo->dataSize = strtoll( tmpDataSize, 0, 0 );
        rstrcpy( dataObjInfo->chksum, tmpChksum, NAME_LEN );
        rstrcpy( dataObjInfo->version, tmpVersion, NAME_LEN );
        rstrcpy( dataObjInfo->filePath, tmpFilePath, MAX_NAME_LEN );
        rstrcpy( dataObjInfo->dataOwnerName, tmpDataOwnerName, NAME_LEN );
        rstrcpy( dataObjInfo->dataOwnerZone, tmpDataOwnerZone, NAME_LEN );
        dataObjInfo->replNum = atoi( tmpReplNum );
        dataObjInfo->replStatus = atoi( tmpReplStatus );
        rstrcpy( dataObjInfo->statusString, tmpStatusString, NAME_LEN );
        dataObjInfo->dataId = strtoll( tmpDataId, 0, 0 );
        dataObjInfo->collId = strtoll( tmpCollId, 0, 0 );
        dataObjInfo->dataMapId = atoi( tmpDataMapId );
        rstrcpy( dataObjInfo->dataComments, tmpDataComments, LONG_NAME_LEN );
        rstrcpy( dataObjInfo->dataExpiry, tmpDataExpiry, TIME_LEN );
        rstrcpy( dataObjInfo->dataCreate, tmpDataCreate, TIME_LEN );
        rstrcpy( dataObjInfo->dataModify, tmpDataModify, TIME_LEN );
        rstrcpy( dataObjInfo->dataMode, tmpDataMode, SHORT_STR_LEN );
        dataObjInfo->writeFlag = writeFlag;

        dataObjInfo->next = nullptr;

        constexpr int is_single_object = 1;
        constexpr int prepend_object = 0;
        queDataObjInfo(dataObjInfoHead, dataObjInfo, is_single_object, prepend_object);
    } // for i

    freeGenQueryOut( &genQueryOut );

    return qcondCnt;
}

// determine if a forced open is done to a resource which does not have
// an original copy of the data object

/* sortObjInfoForOpen - Sort the dataObjInfo in dataObjInfoHead for open.
 * If RESC_HIER_STR_KW is set matching resc obj is first.
 * If it is for read (writeFlag == 0), discard old copies, then cache first,
 * archival second.
 * If it is for write, (writeFlag > 0), resource in DEST_RESC_NAME_KW first,
 * then current cache, current archival, old cache and old archival.
 */
int sortObjInfoForOpen(
    dataObjInfo_t** dataObjInfoHead,
    keyValPair_t*   condInput,
    int             writeFlag ) {

    char* resc_hier = getValByKey( condInput, RESC_HIER_STR_KW );
    if (!resc_hier) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - No resource hierarchy specified in keywords.";
        irods::log( ERROR( SYS_INVALID_INPUT_PARAM, msg.str() ) );
        return SYS_INVALID_INPUT_PARAM;
    }

    dataObjInfo_t* found_info = NULL;
    dataObjInfo_t* prev_info = NULL;
    for (dataObjInfo_t* dataObjInfo = *dataObjInfoHead;
            !found_info && dataObjInfo;
            dataObjInfo = dataObjInfo->next) {
        if ( strcmp( resc_hier, dataObjInfo->rescHier ) == 0 ) {
            found_info = dataObjInfo;
        }
        else {
            prev_info = dataObjInfo;
        }
    }

    if (!found_info) {
        // according to the below read only semantics
        // any copy in the head is a good copy.
        if (writeFlag) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - No data object found matching resource hierarchy: \"";
            msg << resc_hier;
            msg << "\"";
            irods::log( ERROR( HIERARCHY_ERROR, msg.str() ) );
            return HIERARCHY_ERROR;
        }
        return 0;
    }

    if (!prev_info) {
        // our object is at the head of the list. So delete the rest of the list, if any and we are done.
        if (found_info->next) {
            freeAllDataObjInfo(found_info->next);
            found_info->next = NULL;
        }
        return 0;
    }

    // remove our object from the list. delete that list then make our object the head.
    prev_info->next = found_info->next;
    found_info->next = NULL;
    freeAllDataObjInfo(*dataObjInfoHead);
    *dataObjInfoHead = found_info;
    return 0;
}

int create_and_sort_data_obj_info_for_open(
		const std::string& resc_hier,
		const irods::file_object_ptr file_obj,
		dataObjInfo_t **data_obj_info_head) {

	// checks
    if (resc_hier.empty()) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Empty input resource hierarchy.";
        irods::log( ERROR( SYS_INVALID_INPUT_PARAM, msg.str() ) );
        return SYS_INVALID_INPUT_PARAM;
    }

	// initialize output list
	*data_obj_info_head = NULL;

// check C++11 support
#if __cplusplus > 199711L
	// iterate over replicas
	for (auto& replica : file_obj->replicas()) {

#else
	// iterate over replicas
	std::vector< irods::physical_object >::iterator itr;
	for (itr = file_obj->replicas().begin(); itr != file_obj->replicas().end(); ++itr) {
		irods::physical_object replica = *itr;
#endif

		// look for replica with matching resource hierarchy
		if (replica.resc_hier() == resc_hier) {

			// initialize object info
			dataObjInfo_t *data_obj_info = (dataObjInfo_t*)malloc(sizeof(dataObjInfo_t));
	        memset(data_obj_info, 0, sizeof(dataObjInfo_t));

	        // copy values from physical object
	        data_obj_info->replStatus = replica.replica_status();
	        data_obj_info->replNum = replica.repl_num();
	        data_obj_info->dataMapId = replica.map_id();
	        data_obj_info->dataSize = replica.size();
	        data_obj_info->dataId = replica.id();
	        data_obj_info->collId = replica.coll_id();
	        snprintf(data_obj_info->objPath, MAX_NAME_LEN, "%s", replica.name().c_str());
	        snprintf(data_obj_info->version, NAME_LEN, "%s", replica.version().c_str());
	        snprintf(data_obj_info->dataType, NAME_LEN, "%s", replica.type_name().c_str());
	        snprintf(data_obj_info->rescName, NAME_LEN, "%s", replica.resc_name().c_str());
	        snprintf(data_obj_info->filePath, MAX_NAME_LEN, "%s", replica.path().c_str());
	        snprintf(data_obj_info->dataOwnerName, NAME_LEN, "%s", replica.owner_name().c_str());
	        snprintf(data_obj_info->dataOwnerZone, NAME_LEN, "%s", replica.owner_zone().c_str());
	        snprintf(data_obj_info->statusString, NAME_LEN, "%s", replica.status().c_str());
	        snprintf(data_obj_info->chksum, NAME_LEN, "%s", replica.checksum().c_str());
	        snprintf(data_obj_info->dataExpiry, TIME_LEN, "%s", replica.expiry_ts().c_str());
	        snprintf(data_obj_info->dataMode, SHORT_STR_LEN, "%s", replica.mode().c_str());
	        snprintf(data_obj_info->dataComments, LONG_NAME_LEN, "%s", replica.r_comment().c_str());
	        snprintf(data_obj_info->dataCreate, TIME_LEN, "%s", replica.create_ts().c_str());
	        snprintf(data_obj_info->dataModify, TIME_LEN, "%s", replica.modify_ts().c_str());
	        snprintf(data_obj_info->rescHier, MAX_NAME_LEN, "%s", replica.resc_hier().c_str());

	        // make data_obj_info the head of our list
	        *data_obj_info_head = data_obj_info;

	        // done
			return 0;
		}
	} // iterate over replicas

	// no "good" replica was found
	std::stringstream msg;
	msg << __FUNCTION__;
	msg << " - No replica found with input resource hierarchy: \"";
	msg << resc_hier;
	msg << "\"";
	irods::log( ERROR( HIERARCHY_ERROR, msg.str() ) );
	return HIERARCHY_ERROR;
}

int
getNumDataObjInfo( dataObjInfo_t *dataObjInfoHead ) {
    dataObjInfo_t *tmpDataObjInfo;
    int numInfo = 0;

    tmpDataObjInfo = dataObjInfoHead;
    while ( tmpDataObjInfo != NULL ) {
        numInfo++;
        tmpDataObjInfo = tmpDataObjInfo->next;
    }
    return numInfo;
}

int
sortDataObjInfoRandom( dataObjInfo_t **dataObjInfoHead ) {
    dataObjInfo_t *myDataObjInfo[50];
    dataObjInfo_t *tmpDataObjInfo;
    int i, j, tmpCnt, order;
    int numInfo = getNumDataObjInfo( *dataObjInfoHead );

    if ( numInfo <= 1 ) {
        return 0;
    }

    if ( numInfo > 50 ) {
        rodsLog( LOG_NOTICE,
                 "sortDataObjInfoRandom: numInfo %d > 50, setting it to 50", numInfo );
        numInfo = 50;
    }

    memset( myDataObjInfo, 0, numInfo * sizeof( dataObjInfo_t * ) );
    /* fill the array randomly */

    tmpCnt = numInfo;
    tmpDataObjInfo = *dataObjInfoHead;
    while ( tmpDataObjInfo != NULL ) {
        if ( tmpCnt > 1 ) {
            order = irods::getRandom<unsigned int>() % tmpCnt;
        }
        else {
            order = 0;
        }
        for ( i = 0, j = 0; i < numInfo; i ++ ) {
            if ( myDataObjInfo[i] == NULL ) {
                if ( order <= j ) {
                    myDataObjInfo[i] = tmpDataObjInfo;
                    break;
                }
                j ++;
            }
        }
        tmpCnt --;
        tmpDataObjInfo = tmpDataObjInfo->next;
    }

    /* now que the array in order */

    *dataObjInfoHead = NULL;
    for ( i = 0; i < numInfo; i ++ ) {
        queDataObjInfo( dataObjInfoHead, myDataObjInfo[i], 1, 1 );
    }

    return 0;
}

/* requeDataObjInfoByResc - requeue the dataObjInfo in the
 * dataObjInfoHead by putting dataObjInfo stored in preferredResc
 * at the top of the queue.
 * return 0 if dataObjInfo with preferredResc exists.
 * Otherwise, return -1.
 */

int
requeDataObjInfoByResc( dataObjInfo_t **dataObjInfoHead,
                        const char *preferredResc, int writeFlag, int topFlag ) {

    int status = -1;

    if (!preferredResc || !*dataObjInfoHead) {
        return 0;
    }

    dataObjInfo_t* tmpDataObjInfo = *dataObjInfoHead;
    if (!tmpDataObjInfo->next) {
        /* just one */
        const bool resc_in_hier = irods::hierarchy_parser{tmpDataObjInfo->rescHier}.resc_in_hier(preferredResc);
        return resc_in_hier ? 0 : -1;
    }

    dataObjInfo_t* prevDataObjInfo = NULL;
    while (tmpDataObjInfo) {
        if (irods::hierarchy_parser{tmpDataObjInfo->rescHier}.resc_in_hier(preferredResc) &&
            (writeFlag > 0 || tmpDataObjInfo->replStatus > 0)) {
            if (prevDataObjInfo) {
                prevDataObjInfo->next = tmpDataObjInfo->next;
                queDataObjInfo(dataObjInfoHead, tmpDataObjInfo, 1, topFlag);
            }
            if (topFlag > 0) {
                return 0;
            }
            status = 0;
        }
        prevDataObjInfo = tmpDataObjInfo;
        tmpDataObjInfo = tmpDataObjInfo->next;
    }
    return status;
}

/* matchAndTrimRescGrp - check for matching rescName in dataObjInfoHead
 * and rescGrpInfoHead. If there is a match, unlink and free the
 * rescGrpInfo in rescGrpInfoHead so that they won't be replicated
 * If trimjFlag - set what to trim. Valid input are : TRIM_MATCHED_RESC_INFO,
 * TRIM_MATCHED_OBJ_INFO and TRIM_UNMATCHED_OBJ_INFO
 */
int
matchAndTrimRescGrp( dataObjInfo_t **dataObjInfoHead,
                     const std::string& _resc_name, // replaces rescGrpInfoHead
                     int trimjFlag,
                     dataObjInfo_t **trimmedDataObjInfo ) {

    dataObjInfo_t *tmpDataObjInfo, *prevDataObjInfo, *nextDataObjInfo;

    if ( trimmedDataObjInfo != NULL ) {
        *trimmedDataObjInfo = NULL;
    }

    tmpDataObjInfo = *dataObjInfoHead;
    prevDataObjInfo = NULL;

    while ( tmpDataObjInfo != NULL ) {
        nextDataObjInfo = tmpDataObjInfo->next;

        irods::hierarchy_parser h_parse;
        h_parse.set_string(tmpDataObjInfo->rescHier);
        if(h_parse.resc_in_hier(_resc_name)) {

            if ( trimjFlag & TRIM_MATCHED_OBJ_INFO ) {
                if ( tmpDataObjInfo == *dataObjInfoHead ) {
                    *dataObjInfoHead = tmpDataObjInfo->next;

                }
                else if ( prevDataObjInfo != NULL ) {
                    prevDataObjInfo->next = tmpDataObjInfo->next;

                }

                if ( trimmedDataObjInfo != NULL ) {
                    queDataObjInfo( trimmedDataObjInfo, tmpDataObjInfo, 1, 0 );

                }
                else {
                    free( tmpDataObjInfo );

                }

            }
            else {
                prevDataObjInfo = tmpDataObjInfo;

            }

        }

        tmpDataObjInfo = nextDataObjInfo;

    } // while

    return 0;
}

/* initDataObjInfoQuery - initialize the genQueryInp based on dataObjInp.
 * returns the qcondCnt - count of sqlCondInp based on condition input.
 */

int
initDataObjInfoQuery( dataObjInp_t *dataObjInp, genQueryInp_t *genQueryInp,
                      int ignoreCondInput ) {
    char myColl[MAX_NAME_LEN], myData[MAX_NAME_LEN];
    char condStr[MAX_NAME_LEN];
    char *tmpStr;
    int status;
    int qcondCnt = 0;

    memset( genQueryInp, 0, sizeof( genQueryInp_t ) );

    if ( ( tmpStr = getValByKey( &dataObjInp->condInput, QUERY_BY_DATA_ID_KW ) )
            == NULL ) {
        memset( myColl, 0, MAX_NAME_LEN );
        memset( myData, 0, MAX_NAME_LEN );

        if ( ( status = splitPathByKey(
                            dataObjInp->objPath, myColl, MAX_NAME_LEN, myData, MAX_NAME_LEN, '/' ) ) < 0 ) {
            rodsLog( LOG_NOTICE,
                     "initDataObjInfoQuery: splitPathByKey for %s error, status = %d",
                     dataObjInp->objPath, status );
            return status;
        }
        snprintf( condStr, MAX_NAME_LEN, "='%s'", myColl );
        addInxVal( &genQueryInp->sqlCondInp, COL_COLL_NAME, condStr );
        snprintf( condStr, MAX_NAME_LEN, "='%s'", myData );
        addInxVal( &genQueryInp->sqlCondInp, COL_DATA_NAME, condStr );
    }
    else {
        snprintf( condStr, MAX_NAME_LEN, "='%s'", tmpStr );
        addInxVal( &genQueryInp->sqlCondInp, COL_D_DATA_ID, condStr );
    }

    if ( const char * admin = getValByKey( &dataObjInp->condInput, ADMIN_KW ) ) {
        addKeyVal( &genQueryInp->condInput, ADMIN_KW, admin );
    }

    return qcondCnt;
}

/* chkOrphanFile - check whether a filePath is a orphan file.
 *    return - 1 - the file is orphan.
 *             0 - 0 the file is not an orphan.
 *             -ive - query error.
 *             If it is not orphan and dataObjInfo != NULL, output ObjID,
 *             replNum and objPath to dataObjInfo.
 */

int
chkOrphanFile(
    rsComm_t*		rsComm,
    const char*     filePath,
    const char*		rescName,
    dataObjInfo_t*	dataObjInfo ) {
    // =-=-=-=-=-=-=-
    // disallow the orphaning of a file by an anonymous user as this
    // is a similar issue to the use of strict acls in that an anonymous
    // user may inappropriately orphan a file owned by another user
    if ( strncmp( ANONYMOUS_USER, rsComm->clientUser.userName, NAME_LEN ) == 0 ) {
        return SYS_USER_NO_PERMISSION;
    }

    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    int status;
    char condStr[MAX_NAME_LEN];

    rodsLong_t resc_id;
    irods::error ret = resc_mgr.hier_to_leaf_id(rescName,resc_id);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }
    std::string resc_id_str = boost::lexical_cast<std::string>(resc_id);;

    // resc name should be leaf name, not root name of the hier
    std::string resc_hier;
    ret = resc_mgr.leaf_id_to_hier(resc_id, resc_hier);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    snprintf( condStr, MAX_NAME_LEN, "='%s'", filePath );
    addInxVal( &genQueryInp.sqlCondInp, COL_D_DATA_PATH, condStr );
    snprintf( condStr, MAX_NAME_LEN, "='%s'", resc_id_str.c_str() );
    addInxVal( &genQueryInp.sqlCondInp, COL_D_RESC_ID, condStr );

    addInxIval( &genQueryInp.selectInp, COL_D_DATA_ID, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_DATA_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_DATA_REPL_NUM, 1 );
    addInxIval( &genQueryInp.selectInp, COL_D_RESC_ID, 1 );

    genQueryInp.maxRows = MAX_SQL_ROWS;

    // =-=-=-=-=-=-=-
    // when strict acls are enabled, this query would have returned that no file exists.
    // this would have resulted in an incorrect orphaning of a file which may actually be
    // owned by another user.  we potentially disable the use of strict acls for this single
    // query in order to avoid orphaning another users physical data.
    addKeyVal( &genQueryInp.condInput, DISABLE_STRICT_ACL_KW, "disable" );

    std::string svr_sid;
    if (irods::server_property_exists(irods::AGENT_CONN_KW)) {
        status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
    }
    else {
        try {
            irods::set_server_property<std::string>(irods::AGENT_CONN_KW, "StrictACLOverride");
            status = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
            irods::delete_server_property(irods::AGENT_CONN_KW);
        }
        catch (const irods::exception& e) {
            irods::log(e);
            return e.code();
        }
    }

    clearGenQueryInp( &genQueryInp );
    if ( status < 0 ) {
        if ( status == CAT_NO_ROWS_FOUND ) {
            freeGenQueryOut( &genQueryOut );
            rsComm->perfStat.orphanCnt ++;
            return 1;
        }
        else {
            rodsLog( LOG_ERROR,
                     "chkOrphanFile: rsGenQuery error for %s, status = %d, rescName %s",
                     filePath, status, rescName );
            /* we have unexpected query error. Assume the file is not
             * orphan */
			freeGenQueryOut( &genQueryOut );
            return status;
        }
    }
    else {
        sqlResult_t *dataId, *replNum, *dataName, *collName;
        rsComm->perfStat.nonOrphanCnt++;

        if ( ( collName =
                    getSqlResultByInx( genQueryOut, COL_COLL_NAME ) ) == NULL ) {
            rodsLog( LOG_NOTICE,
                     "chkOrphanFile: getSqlResultByInx for COL_COLL_NAME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        if ( ( dataName = getSqlResultByInx( genQueryOut, COL_DATA_NAME ) )
                == NULL ) {
            rodsLog( LOG_NOTICE,
                     "chkOrphanFile: getSqlResultByInx for COL_DATA_NAME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        if ( ( dataId = getSqlResultByInx( genQueryOut, COL_D_DATA_ID ) ) == NULL ) {
            rodsLog( LOG_NOTICE,
                     "chkOrphanFile: getSqlResultByInx for COL_D_DATA_ID failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        if ( ( replNum = getSqlResultByInx( genQueryOut, COL_DATA_REPL_NUM ) ) ==
                NULL ) {
            rodsLog( LOG_NOTICE,
                     "chkOrphanFile: getSqlResultByInx for COL_DATA_REPL_NUM failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        if ( dataObjInfo != NULL ) {
            dataObjInfo->dataId = strtoll( dataId->value, 0, 0 );
            dataObjInfo->replNum = atoi( replNum->value );
            snprintf( dataObjInfo->objPath, MAX_NAME_LEN, "%s/%s",
                      collName->value, dataName->value );
            rstrcpy( dataObjInfo->rescHier, resc_hier.c_str(), MAX_NAME_LEN );
        }

        freeGenQueryOut( &genQueryOut );

        return 0;
    }
}


/* chkOrphanDir - check whether a filePath is a orphan directory.
 *    return - 1 - the dir is orphan.
 *             0 - 0 the dir is not an orphan.
 *             -ive - query error.
 */

int
chkOrphanDir( rsComm_t *rsComm, char *dirPath, const char *rescName ) {
    char subfilePath[MAX_NAME_LEN];
    int savedStatus = 1;
    int status = 0;

    path srcDirPath( dirPath );
    if ( !exists( srcDirPath ) || !is_directory( srcDirPath ) ) {
        rodsLog( LOG_ERROR,
                 "chkOrphanDir: opendir error for %s, errno = %d",
                 dirPath, errno );
        return UNIX_FILE_OPENDIR_ERR - errno;
    }
    directory_iterator end_itr; // default construction yields past-the-end
    for ( directory_iterator itr( srcDirPath ); itr != end_itr; ++itr ) {
        path p = itr->path();
        snprintf( subfilePath, MAX_NAME_LEN, "%s",
                  p.c_str() );
        if ( !exists( p ) ) {
            rodsLog( LOG_ERROR,
                     "chkOrphanDir: stat error for %s, errno = %d",
                     subfilePath, errno );
            savedStatus = UNIX_FILE_STAT_ERR - errno;
            continue;
        }
        if ( is_directory( p ) ) {
            status = chkOrphanDir( rsComm, subfilePath, rescName );
        }
        else if ( is_regular_file( p ) ) {
            status = chkOrphanFile( rsComm, subfilePath, rescName, NULL );
        }
        if ( status == 0 ) {
            /* not orphan */
            return status;
        }
        else if ( status < 0 ) {
            savedStatus = status;
        }
    }
    return savedStatus;
}

/* matchDataObjInfoByCondInput - given dataObjInfoHead and oldDataObjInfoHead
 * put all DataObjInfo that match condInput into matchedDataObjInfo and
 * matchedOldDataObjInfo. The unmatch one stay in dataObjInfoHead and
 * oldDataObjInfoHead.
 * A CAT_NO_ROWS_FOUND means there is condition in CondInput, but none
 * in dataObjInfoHead or oldDataObjInfoHead matches the condition
 */
int matchDataObjInfoByCondInput( dataObjInfo_t **dataObjInfoHead,
                                 dataObjInfo_t **oldDataObjInfoHead, keyValPair_t *condInput,
                                 dataObjInfo_t **matchedDataObjInfo, dataObjInfo_t **matchedOldDataObjInfo )

{

    int replNumCond;
    int replNum = 0;
    int rescCond;
    bool destHierCond = false;
    char *tmpStr, *rescName = NULL;
    char* rescHier = NULL;
    char* destRescHier = NULL;
    dataObjInfo_t *prevDataObjInfo, *nextDataObjInfo, *tmpDataObjInfo;

    if ( dataObjInfoHead == NULL || *dataObjInfoHead == NULL ||
            oldDataObjInfoHead == NULL || matchedDataObjInfo == NULL ||
            matchedOldDataObjInfo == NULL ) {
        rodsLog( LOG_ERROR,
                 "requeDataObjInfoByCondInput: NULL dataObjInfo input" );
        return USER__NULL_INPUT_ERR;
    }

    if ( ( tmpStr = getValByKey( condInput, REPL_NUM_KW ) ) != NULL ) {
        replNum = atoi( tmpStr );
        replNumCond = 1;
    }
    else {
        replNumCond = 0;
    }

    destRescHier = getValByKey( condInput, DEST_RESC_HIER_STR_KW );
    rescHier = getValByKey( condInput, RESC_HIER_STR_KW );
    if ( destRescHier != NULL && rescHier != NULL ) {
        destHierCond = true;
    }

    // We only use the resource name if the resource hierarchy is not set
    if ( !destHierCond && ( rescName = getValByKey( condInput, RESC_NAME_KW ) ) != NULL ) {
        rescCond = 1;
    }
    else {
        rescCond = 0;
    }

    if ( replNumCond + rescCond == 0 && !destHierCond ) {
        return 0;
    }
    *matchedDataObjInfo = NULL;
    *matchedOldDataObjInfo = NULL;

    tmpDataObjInfo = *dataObjInfoHead;
    prevDataObjInfo = NULL;
    while ( tmpDataObjInfo != NULL ) {
        nextDataObjInfo = tmpDataObjInfo->next;
        if ( replNumCond == 1 && replNum == tmpDataObjInfo->replNum ) {


            if ( prevDataObjInfo != NULL ) {
                prevDataObjInfo->next = tmpDataObjInfo->next;
            }
            else {
                *dataObjInfoHead = ( *dataObjInfoHead )->next;
            }
            queDataObjInfo( matchedDataObjInfo, tmpDataObjInfo, 1, 0 );
        }
        else if ( destHierCond &&
                  ( strcmp( rescHier, tmpDataObjInfo->rescHier ) == 0 ||
                    strcmp( destRescHier, tmpDataObjInfo->rescHier ) == 0 ) ) {
            if ( prevDataObjInfo != NULL ) {
                prevDataObjInfo->next = tmpDataObjInfo->next;
            }
            else {
                *dataObjInfoHead = ( *dataObjInfoHead )->next;
            }
            queDataObjInfo( matchedDataObjInfo, tmpDataObjInfo, 1, 0 );
        }
        else if ( rescCond == 1 &&
                  ( strstr( tmpDataObjInfo->rescHier, rescName ) != 0 ) ) {
            if ( prevDataObjInfo != NULL ) {
                prevDataObjInfo->next = tmpDataObjInfo->next;
            }
            else {
                *dataObjInfoHead = ( *dataObjInfoHead )->next;
            }
            /* que single to the bottom */
            queDataObjInfo( matchedDataObjInfo, tmpDataObjInfo, 1, 0 );
        }
        else {
            prevDataObjInfo = tmpDataObjInfo;
        }
        tmpDataObjInfo = nextDataObjInfo;
    }

    tmpDataObjInfo = *oldDataObjInfoHead;
    prevDataObjInfo = NULL;
    while ( tmpDataObjInfo != NULL ) {
        nextDataObjInfo = tmpDataObjInfo->next;
        if ( replNumCond == 1 && replNum == tmpDataObjInfo->replNum ) {
            if ( prevDataObjInfo != NULL ) {
                prevDataObjInfo->next = tmpDataObjInfo->next;
            }
            else {
                *oldDataObjInfoHead = ( *oldDataObjInfoHead )->next;
            }
            queDataObjInfo( matchedOldDataObjInfo, tmpDataObjInfo, 1, 0 );
        }
        else if ( destHierCond &&
                  ( strcmp( rescHier, tmpDataObjInfo->rescHier ) == 0 ||
                    strcmp( destRescHier, tmpDataObjInfo->rescHier ) == 0 ) ) {
            if ( prevDataObjInfo != NULL ) {
                prevDataObjInfo->next = tmpDataObjInfo->next;
            }
            else {
                *oldDataObjInfoHead = ( *oldDataObjInfoHead )->next;
            }
            queDataObjInfo( matchedOldDataObjInfo, tmpDataObjInfo, 1, 0 );
        }
        else if ( rescCond == 1 &&
                  ( strstr( tmpDataObjInfo->rescHier, rescName ) != 0 ) ) {
            if ( prevDataObjInfo != NULL ) {
                prevDataObjInfo->next = tmpDataObjInfo->next;
            }
            else {
                *oldDataObjInfoHead = ( *oldDataObjInfoHead )->next;
            }
            queDataObjInfo( matchedOldDataObjInfo, tmpDataObjInfo, 1, 0 );
        }
        else {
            prevDataObjInfo = tmpDataObjInfo;
        }
        tmpDataObjInfo = nextDataObjInfo;
    }

    if ( *matchedDataObjInfo == NULL && *matchedOldDataObjInfo == NULL ) {
        return CAT_NO_ROWS_FOUND;
    }
    else {
        return replNumCond + rescCond;
    }
}

int
getDataObjInfoIncSpecColl( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                           dataObjInfo_t **dataObjInfo ) {
    int status, writeFlag;
    specCollPerm_t specCollPerm;
    writeFlag = getWriteFlag( dataObjInp->openFlags );
    if ( writeFlag > 0 ) {
        specCollPerm = WRITE_COLL_PERM;
        if ( rsComm->clientUser.authInfo.authFlag <= PUBLIC_USER_AUTH ) {
            rodsLog( LOG_NOTICE,
                     "%s:open for write not allowed for user %s",
                     __FUNCTION__, rsComm->clientUser.userName );
            return SYS_NO_API_PRIV;
        }
    }
    else {
        specCollPerm = READ_COLL_PERM;
    }

    if ( dataObjInp->specColl != NULL &&
            dataObjInp->specColl->collClass != NO_SPEC_COLL &&
            dataObjInp->specColl->collClass != LINKED_COLL ) {
        /* if it is linked, it already has been resolved */
        status = resolvePathInSpecColl( rsComm, dataObjInp->objPath,
                                        specCollPerm, 0, dataObjInfo );
        if ( status == SYS_SPEC_COLL_OBJ_NOT_EXIST &&
                dataObjInfo != NULL ) {
            freeDataObjInfo( *dataObjInfo );
            dataObjInfo = NULL;
        }
    }
    else if ( ( status = resolvePathInSpecColl( rsComm, dataObjInp->objPath,
                         specCollPerm, 1, dataObjInfo ) ) >= 0 ) {
        /* check if specColl in cache. May be able to save one query */
    }
    else if ( getValByKey( &dataObjInp->condInput,
                           ADMIN_RMTRASH_KW ) != NULL &&
              rsComm->proxyUser.authInfo.authFlag == LOCAL_PRIV_USER_AUTH ) {
        status = getDataObjInfo( rsComm, dataObjInp, dataObjInfo,
                                 NULL, 0 );
    }
    else if ( writeFlag > 0 && dataObjInp->oprType != REPLICATE_OPR ) {
        status = getDataObjInfo( rsComm, dataObjInp, dataObjInfo,
                                 ACCESS_MODIFY_OBJECT, 0 );
    }
    else if (getValByKey(&dataObjInp->condInput, DATA_ACCESS_KW)) {
        char* data_access_kw = getValByKey(&dataObjInp->condInput, DATA_ACCESS_KW);
        status = getDataObjInfo( rsComm, dataObjInp, dataObjInfo,
                                 data_access_kw, 0 );
    }
    else {
        status = getDataObjInfo( rsComm, dataObjInp, dataObjInfo,
                                 ACCESS_READ_OBJECT, 0 );
    }

    if ( status < 0 && dataObjInp->specColl == NULL ) {
        int status2;
        status2 = resolvePathInSpecColl( rsComm, dataObjInp->objPath,
                                         specCollPerm, 0, dataObjInfo );
        if ( status2 < 0 ) {
            if ( status2 == SYS_SPEC_COLL_OBJ_NOT_EXIST &&
                    dataObjInfo != NULL ) {
                freeDataObjInfo( *dataObjInfo );
                *dataObjInfo = NULL;
            }
        }
        else {
            status = 0;
        }
    }
    if ( status >= 0 &&
         NULL != dataObjInfo && NULL != ( *dataObjInfo )->specColl ) {
        if ( LINKED_COLL == ( *dataObjInfo )->specColl->collClass ) {
            /* already been translated */
            rstrcpy( dataObjInp->objPath, ( *dataObjInfo )->objPath,
                     MAX_NAME_LEN );
            free( ( *dataObjInfo )->specColl );
            ( *dataObjInfo )->specColl = NULL;
        }
        else if ( getStructFileType( ( *dataObjInfo )->specColl ) >= 0 ) {
            dataObjInp->numThreads = NO_THREADING;
        }
    }
    return status;
}

int regNewObjSize( rsComm_t *rsComm, char *objPath, int replNum,
                   rodsLong_t newSize ) {
    dataObjInfo_t dataObjInfo;
    keyValPair_t regParam;
    modDataObjMeta_t modDataObjMetaInp;
    char tmpStr[MAX_NAME_LEN];
    int status;

    if ( objPath == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    memset( &dataObjInfo, 0, sizeof( dataObjInfo ) );
    memset( &regParam, 0, sizeof( regParam ) );
    memset( &modDataObjMetaInp, 0, sizeof( modDataObjMetaInp ) );

    rstrcpy( dataObjInfo.objPath, objPath, MAX_NAME_LEN );
    dataObjInfo.replNum = replNum;
    snprintf( tmpStr, MAX_NAME_LEN, "%lld", newSize );
    addKeyVal( &regParam, DATA_SIZE_KW, tmpStr );

    modDataObjMetaInp.dataObjInfo = &dataObjInfo;
    modDataObjMetaInp.regParam = &regParam;
    status = rsModDataObjMeta( rsComm, &modDataObjMetaInp );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "regNewObjSize: rsModDataObjMeta error for %s, status = %d",
                 objPath, status );
    }

    return status;
}

irods::error resolve_hierarchy_for_resc_from_cond_input(
    rsComm_t*          _comm,
    const std::string& _resc,
    std::string&       _hier ) {
    // =-=-=-=-=-=-=-
    // we need to start with the last resource in the hierarchy
    irods::hierarchy_parser parser;
    parser.set_string( _resc );

    std::string last_resc;
    parser.last_resc( last_resc );

    // =-=-=-=-=-=-=-
    // see if it has a parent or a child
    irods::resource_ptr resc;
    irods::error ret = resc_mgr.resolve( last_resc, resc );
    if ( !ret.ok() ) {
        return PASS( ret );
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
        return ERROR(
                HIERARCHY_ERROR,
                "resource is mid-tier" );
    }

    // =-=-=-=-=-=-=-
    // this is a leaf node situation
    else if ( has_parent && !has_child ) {
        // =-=-=-=-=-=-=-
        // get the path from our parent resource
        // to this given leaf resource - this our hier
        ret = resc_mgr.get_hier_to_root_for_resc(
                last_resc,
                _hier );
        if(!ret.ok()){
            return PASS(ret);
        }
    } else if ( !has_parent && !has_child ) {
        _hier = last_resc;

    }

    return SUCCESS();

} // resolve_hierarchy_for_cond_input

irods::linked_list_iterator<dataObjInfo_t> begin(dataObjInfo_t* _objects) noexcept {
    return irods::linked_list_iterator<dataObjInfo_t>{_objects};
}

irods::linked_list_iterator<const dataObjInfo_t> begin(const dataObjInfo_t* _objects) noexcept {
    return irods::linked_list_iterator<const dataObjInfo_t>{_objects};
}

irods::linked_list_iterator<dataObjInfo_t> end(dataObjInfo_t* _objects) noexcept {
    return {};
}

irods::linked_list_iterator<const dataObjInfo_t> end(const dataObjInfo_t* _objects) noexcept {
    return {};
}

bool contains_replica(const dataObjInfo_t* _objects, const std::string& _resc_name) {
    return contains_replica_if(_objects, [&_resc_name](const auto& _node) {
        const std::string hier{_node.rescHier};
        auto b = boost::make_split_iterator(hier, boost::first_finder(";", boost::is_equal{}));
        boost::split_iterator<std::string::const_iterator> e{};

        return std::any_of(b, e, [&_resc_name](const auto& _resource_name) {
            return _resource_name == _resc_name;
        });
    });
}

bool contains_replica(const dataObjInfo_t* _objects, int _replica_number) {
    return contains_replica_if(_objects, [_replica_number](const auto& _node) {
        return _node.replNum == _replica_number;
    });
}

