#include "irods/apiHeaderAll.h"
#include "irods/dataObjOpr.hpp"
#include "irods/objMetaOpr.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/rsBulkDataObjReg.hpp"
#include "irods/rsRegDataObj.hpp"
#include "irods/rsModDataObjMeta.hpp"
#include "irods/replica_proxy.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/irods_file_object.hpp"
#include "irods/irods_configuration_keywords.hpp"

#include <cstring>

int
rsBulkDataObjReg( rsComm_t *rsComm, genQueryOut_t *bulkDataObjRegInp,
                  genQueryOut_t **bulkDataObjRegOut ) {
    sqlResult_t *objPath;
    int status;
    rodsServerHost_t *rodsServerHost = NULL;

    if ( bulkDataObjRegInp->rowCnt <= 0 ) {
        return 0;
    }

    if ( ( objPath =
                getSqlResultByInx( bulkDataObjRegInp, COL_DATA_NAME ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "rsBulkDataObjReg: getSqlResultByInx for COL_DATA_NAME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    status = getAndConnRcatHost( rsComm, PRIMARY_RCAT, ( const char* )objPath->value,
                                 &rodsServerHost );
    if ( status < 0 || rodsServerHost == NULL ) { // JMC cppcheck
        return status;
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
        std::string svc_role;
        irods::error ret = get_catalog_service_role(svc_role);
        if(!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }

        if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            status = _rsBulkDataObjReg( rsComm, bulkDataObjRegInp,
                                        bulkDataObjRegOut );
        } else if( irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
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
        status = rcBulkDataObjReg( rodsServerHost->conn, bulkDataObjRegInp,
                                   bulkDataObjRegOut );
    }

    return status;
}

int _rsBulkDataObjReg(
    rsComm_t *rsComm,
    genQueryOut_t *bulkDataObjRegInp,
    genQueryOut_t **bulkDataObjRegOut)
{
    namespace ir = irods::experimental::replica;

    std::string svc_role;
    if (const auto ret = get_catalog_service_role(svc_role); !ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        sqlResult_t *objPath, *dataType, *dataSize, *rescName, *rescID, *filePath,
                    *dataMode, *oprType, *replNum, *chksum;
        char *tmpObjPath, *tmpDataType, *tmpDataSize, *tmpRescName, *tmpRescID, *tmpFilePath,
             *tmpDataMode, *tmpOprType, *tmpReplNum, *tmpChksum;
        char *tmpObjId;
        int status;

        if ( ( rescID =
                    getSqlResultByInx( bulkDataObjRegInp, COL_D_RESC_ID ) ) == NULL ) {
            rodsLog( LOG_NOTICE,
                     "rsBulkDataObjReg: getSqlResultByInx for COL_D_RESC_ID failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        if ( ( objPath =
                    getSqlResultByInx( bulkDataObjRegInp, COL_DATA_NAME ) ) == NULL ) {
            rodsLog( LOG_NOTICE,
                     "rsBulkDataObjReg: getSqlResultByInx for COL_DATA_NAME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        if ( ( dataType =
                    getSqlResultByInx( bulkDataObjRegInp, COL_DATA_TYPE_NAME ) ) == NULL ) {
            rodsLog( LOG_NOTICE,
                     "rsBulkDataObjReg: getSqlResultByInx for COL_DATA_TYPE_NAME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        if ( ( dataSize =
                    getSqlResultByInx( bulkDataObjRegInp, COL_DATA_SIZE ) ) == NULL ) {
            rodsLog( LOG_NOTICE,
                     "rsBulkDataObjReg: getSqlResultByInx for COL_DATA_SIZE failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        if ( ( rescName =
                    getSqlResultByInx( bulkDataObjRegInp, COL_D_RESC_NAME ) ) == NULL ) {
            rodsLog( LOG_NOTICE,
                     "rsBulkDataObjReg: getSqlResultByInx for COL_D_RESC_NAME failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        if ( ( filePath =
                    getSqlResultByInx( bulkDataObjRegInp, COL_D_DATA_PATH ) ) == NULL ) {
            rodsLog( LOG_NOTICE,
                     "rsBulkDataObjReg: getSqlResultByInx for COL_D_DATA_PATH failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        if ( ( dataMode =
                    getSqlResultByInx( bulkDataObjRegInp, COL_DATA_MODE ) ) == NULL ) {
            rodsLog( LOG_NOTICE,
                     "rsBulkDataObjReg: getSqlResultByInx for COL_DATA_MODE failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        if ( ( oprType =
                    getSqlResultByInx( bulkDataObjRegInp, OPR_TYPE_INX ) ) == NULL ) {
            rodsLog( LOG_ERROR,
                     "rsBulkDataObjReg: getSqlResultByInx for OPR_TYPE_INX failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        if ( ( replNum =
                    getSqlResultByInx( bulkDataObjRegInp, COL_DATA_REPL_NUM ) ) == NULL ) {
            rodsLog( LOG_ERROR,
                     "rsBulkDataObjReg: getSqlResultByInx for COL_DATA_REPL_NUM failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        chksum = getSqlResultByInx( bulkDataObjRegInp, COL_D_DATA_CHECKSUM );

        /* the output */
        initBulkDataObjRegOut( bulkDataObjRegOut );

        sqlResult_t* objId{};
        if ( ( objId =
                    getSqlResultByInx( *bulkDataObjRegOut, COL_D_DATA_ID ) ) == NULL ) {
            rodsLog( LOG_ERROR,
                     "rsBulkDataObjReg: getSqlResultByInx for COL_D_DATA_ID failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }

        std::vector<std::pair<ir::replica_proxy_t, irods::experimental::lifetime_manager<DataObjInfo>>> result_info;

        ( *bulkDataObjRegOut )->rowCnt = bulkDataObjRegInp->rowCnt;
        for (int i = 0; i < bulkDataObjRegInp->rowCnt; i++ ) {
            tmpObjPath = &objPath->value[objPath->len * i];
            tmpDataType = &dataType->value[dataType->len * i];
            tmpDataSize = &dataSize->value[dataSize->len * i];
            tmpRescName = &rescName->value[rescName->len * i];
            tmpRescID   = &rescID->value[rescID->len * i];
            tmpFilePath = &filePath->value[filePath->len * i];
            tmpDataMode = &dataMode->value[dataMode->len * i];
            tmpOprType = &oprType->value[oprType->len * i];
            tmpReplNum =  &replNum->value[replNum->len * i];
            tmpObjId = &objId->value[objId->len * i];
            static_cast<void>(tmpObjId);

            dataObjInfo_t dataObjInfo{};
            dataObjInfo.flags = NO_COMMIT_FLAG;
            rstrcpy( dataObjInfo.objPath, tmpObjPath, MAX_NAME_LEN );
            rstrcpy( dataObjInfo.dataType, tmpDataType, NAME_LEN );
            dataObjInfo.dataSize = strtoll( tmpDataSize, 0, 0 );
            rstrcpy( dataObjInfo.rescName, tmpRescName, NAME_LEN );

            dataObjInfo.rescId = strtoll(tmpRescID, 0, 0);
            std::string resc_hier;
            irods::error ret = resc_mgr.leaf_id_to_hier(dataObjInfo.rescId, resc_hier);
            if( !ret.ok() ) {
                irods::log(PASS(ret));
            }
            else {
                rstrcpy( dataObjInfo.rescHier, resc_hier.c_str(), MAX_NAME_LEN );
            }
            rstrcpy( dataObjInfo.filePath, tmpFilePath, MAX_NAME_LEN );
            rstrcpy( dataObjInfo.dataMode, tmpDataMode, SHORT_STR_LEN );
            dataObjInfo.replNum = atoi( tmpReplNum );
            if ( chksum != NULL ) {
                tmpChksum = &chksum->value[chksum->len * i];
                if ( strlen( tmpChksum ) > 0 ) {
                    rstrcpy( dataObjInfo.chksum, tmpChksum, NAME_LEN );
                }
            }

            dataObjInfo.replStatus = GOOD_REPLICA;
            if ( strcmp( tmpOprType, REGISTER_OPR ) == 0 ) {
                status = svrRegDataObj( rsComm, &dataObjInfo );
            }
            else {
                status = modDataObjSizeMeta( rsComm, &dataObjInfo, tmpDataSize );
            }

            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "rsBulkDataObjReg: RegDataObj or ModDataObj failed for %s,stat=%d",
                         tmpObjPath, status );
                chlRollback( rsComm );
                freeGenQueryOut( bulkDataObjRegOut );
                *bulkDataObjRegOut = NULL;
                return status;
            }

            result_info.push_back(ir::duplicate_replica(dataObjInfo));
        }

        if (const auto ec = chlCommit(rsComm); ec < 0) {
            rodsLog(LOG_ERROR, "rsBulkDataObjReg: chlCommit failed, status = %d", ec );
            freeGenQueryOut(bulkDataObjRegOut);
            *bulkDataObjRegOut = NULL;
            return ec;
        }

        // Added due to lack of notification of new data object
        // to resource hiers during reg operation. ticket #1753
        for (auto& ri : result_info) {
            auto& r = std::get<ir::replica_proxy_t>(ri);

            irods::file_object_ptr file_obj(new irods::file_object(rsComm, r.get()));

            if (auto ret = fileModified(rsComm, file_obj); !ret.ok()) {
                const auto msg = fmt::format(
                    "[{}:{}] - failed to signal resource that the data object was registered "
                    "[error code=[{}], path=[{}], hierarchy=[{}]]",
                    __FUNCTION__, __LINE__, ret.code(), r.logical_path(), r.hierarchy());

                ret = PASSMSG(msg, ret);

                irods::log(ret);

                return ret.code();
            }
        }

        return status;
    } else if( irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        return SYS_NO_RCAT_SERVER_ERR;
    } else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        return SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }
} // _rsBulkDataObjReg

int
modDataObjSizeMeta( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo,
                    char *strDataSize ) {
    modDataObjMeta_t modDataObjMetaInp;
    keyValPair_t regParam;
    char tmpStr[MAX_NAME_LEN];
    int status;

    std::memset(&modDataObjMetaInp, 0, sizeof(modDataObjMetaInp));
    std::memset(&regParam, 0, sizeof(regParam));
    addKeyVal( &regParam, DATA_SIZE_KW, strDataSize );
    addKeyVal( &regParam, ALL_REPL_STATUS_KW, "" );
    snprintf( tmpStr, MAX_NAME_LEN, "%d", ( int ) time( NULL ) );
    addKeyVal( &regParam, DATA_MODIFY_KW, tmpStr );
    if ( strlen( dataObjInfo->chksum ) > 0 ) {
        addKeyVal( &regParam, CHKSUM_KW, dataObjInfo->chksum );
    }
    addKeyVal( &regParam, IN_PDMO_KW, "" );
    modDataObjMetaInp.dataObjInfo = dataObjInfo;
    modDataObjMetaInp.regParam = &regParam;

    status = _rsModDataObjMeta( rsComm, &modDataObjMetaInp );

    clearKeyVal( &regParam );

    return status;
}
