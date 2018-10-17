/*** Copyright (c), The Unregents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* unregDataObj.c
 */

#include "regReplica.h"

#include "objMetaOpr.hpp"
#include "rsFileStat.hpp"
#include "miscServerFunct.hpp"
#include "rsRegReplica.hpp"
#include "rsFileChksum.hpp"
#include "rsModDataObjMeta.hpp"
#include "icatHighLevelRoutines.hpp"

#include "irods_file_object.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_configuration_keywords.hpp"

#include "boost/lexical_cast.hpp"

int _call_file_modified_for_replica(
    rsComm_t*     rsComm,
    regReplica_t* regReplicaInp );

namespace irods {
    namespace reg_repl {

        void get_object_and_collection_from_path(
            const std::string& _object_path,
            std::string&       _collection_name,
            std::string&       _object_name ) {
            namespace bfs = boost::filesystem;

            try {
                bfs::path p(_object_path);
                _collection_name = p.parent_path().string();
                _object_name     = p.filename().string();
            }
            catch(const bfs::filesystem_error& _e) {
                THROW(SYS_INVALID_FILE_PATH, _e.what());
            }
        } // get_object_and_collection_from_path

        std::string compute_checksum_for_resc(
            rsComm_t*          _comm,
            const std::string& _logical_path,
            const std::string& _physical_path,
            const std::string& _resc_hier ) {
           
            fileChksumInp_t chk_inp{}; 
            rstrcpy(
                chk_inp.objPath,
                _logical_path.c_str(),
                MAX_NAME_LEN);
            rstrcpy(
                chk_inp.fileName,
                _physical_path.c_str(),
                MAX_NAME_LEN);
            rstrcpy(
                chk_inp.rescHier,
                _resc_hier.c_str(),
                MAX_NAME_LEN);

            char* chksum{};
            const auto chksum_err = rsFileChksum(
                                        _comm,
                                        &chk_inp,
                                        &chksum);
            if(DIRECT_ARCHIVE_ACCESS == chksum_err) {
                return "";
            }

            if(chksum_err < 0) {
                THROW(
                    chksum_err,
                    boost::format("rsDataObjChksum failed for [%s] on [%s]") %
                    _logical_path %
                    _resc_hier);
            }

            return chksum;
        } // compute_checksum_for_resc

        rodsLong_t get_file_size_from_filesystem(
            rsComm_t*          _comm,
            const std::string& _object_path,
            const std::string& _resource_hierarchy,
            const std::string& _file_path ) {
            fileStatInp_t stat_inp{};
            rstrcpy(stat_inp.objPath,  _object_path.c_str(),  sizeof(stat_inp.objPath));
            rstrcpy(stat_inp.rescHier, _resource_hierarchy.c_str(), sizeof(stat_inp.rescHier));
            rstrcpy(stat_inp.fileName, _file_path.c_str(), sizeof(stat_inp.fileName));
            rodsStat_t *stat_out{};
            const auto status_rsFileStat = rsFileStat(_comm, &stat_inp, &stat_out);
            if(status_rsFileStat < 0) {
                THROW(
                    status_rsFileStat,
                    boost::format("rsFileStat of objPath [%s] rescHier [%s] fileName [%s] failed with [%d]") %
                    stat_inp.objPath %
                    stat_inp.rescHier %
                    stat_inp.fileName %
                    status_rsFileStat);
                return status_rsFileStat;
            }

            const auto size_in_vault = stat_out->st_size;
            free(stat_out);
            return size_in_vault;
        } // get_file_size_from_filesystem

        void verify_and_update_replica(
            rsComm_t*     _comm,
            regReplica_t* _reg_inp) {
            dataObjInfo_t* src_info = _reg_inp->srcDataObjInfo;
            dataObjInfo_t* dst_info = _reg_inp->destDataObjInfo;

            // Check for data_size_kw
            keyValPair_t reg_param{};
            auto data_size_str{getValByKey(&_reg_inp->condInput, DATA_SIZE_KW)};
            try {
                if (data_size_str) {
                    addKeyVal(&reg_param, DATA_SIZE_KW, data_size_str);
                    dst_info->dataSize = boost::lexical_cast<decltype(dst_info->dataSize)>(data_size_str);
                }
            }
            catch (boost::bad_lexical_cast&) {
                rodsLog(LOG_ERROR, "[%s] - bad_lexical_cast for dataSize [%s]; setting to 0", __FUNCTION__, data_size_str);
                dst_info->dataSize = 0;
                data_size_str = nullptr;
            }

            if (nullptr == data_size_str) {
                const auto dst_size = get_file_size_from_filesystem(_comm, dst_info->objPath, dst_info->rescHier, dst_info->filePath);
                if(UNKNOWN_FILE_SZ != dst_size && dst_size != src_info->dataSize) {
                    dst_info->dataSize = dst_size;
                    const auto dst_size_str = boost::lexical_cast<std::string>(dst_size);
                    addKeyVal(&reg_param, DATA_SIZE_KW, dst_size_str.c_str());
                }
            }

            // optional checksum verification, should one exist 
            // on the source replica
            std::string dst_checksum;
            if(strlen(src_info->chksum) > 0) {
                dst_checksum = compute_checksum_for_resc(
                                   _comm,
                                   dst_info->objPath,
                                   dst_info->filePath,
                                   dst_info->rescHier);
                if(!dst_checksum.empty() &&
                    dst_checksum != src_info->chksum) {
                    rstrcpy(
                       dst_info->chksum,
                       dst_checksum.c_str(),
                       sizeof(dst_info->chksum));
                    addKeyVal(
                       &reg_param,
                       CHKSUM_KW,
                       dst_checksum.c_str());
                }
            } // checksum

            if(reg_param.len > 0) {
                addKeyVal(
                    &reg_param,
                    ALL_REPL_STATUS_KW,
                    "TRUE" );

                modDataObjMeta_t mod_inp{};
                mod_inp.dataObjInfo = dst_info;
                mod_inp.regParam    = &reg_param;
                const auto mod_err = rsModDataObjMeta(_comm, &mod_inp);
                if(mod_err < 0) {
                    THROW(
                        mod_err,
                        boost::format("rsModDataObjMeta failed for [%s] on resc [%s]") %
                        dst_info->objPath %
                        dst_info->rescHier);
                }
            } // if reg_params
        } // verify_and_update_replica
    } // namespace reg_repl
} // namespace irods

int
rsRegReplica( rsComm_t *rsComm, regReplica_t *regReplicaInp ) {
    int status;
    rodsServerHost_t *rodsServerHost = NULL;
    dataObjInfo_t *srcDataObjInfo;

    srcDataObjInfo = regReplicaInp->srcDataObjInfo;

    status = getAndConnRcatHost(
                 rsComm,
                 MASTER_RCAT,
                 ( const char* )srcDataObjInfo->objPath,
                 &rodsServerHost );
    if ( status < 0 || NULL == rodsServerHost ) { // JMC cppcheck - nullptr
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
            status = _rsRegReplica( rsComm, regReplicaInp );
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
        // Add IN_REPL_KW to prevent replication on the redirected server (the provider)
        addKeyVal(&regReplicaInp->condInput, IN_REPL_KW, "" );
        status = rcRegReplica( rodsServerHost->conn, regReplicaInp );
        // Remove the keyword as we will want to replicate on this server (the consumer)
        rmKeyVal(&regReplicaInp->condInput, IN_REPL_KW);
        if ( status >= 0 ) {
            regReplicaInp->destDataObjInfo->replNum = status;
        }
        status = _call_file_modified_for_replica( rsComm, regReplicaInp );
        return status;
    }

    if ( status >= 0 ) {
        try {
            irods::reg_repl::verify_and_update_replica(rsComm, regReplicaInp);
        }
        catch(const irods::exception& _e) {
             irods::log(_e);
             return _e.code();
        }

        if (!getValByKey(&regReplicaInp->condInput, IN_REPL_KW)) {
            status = _call_file_modified_for_replica( rsComm, regReplicaInp );
        }
    }

    return status;
}

int
_rsRegReplica( rsComm_t *rsComm, regReplica_t *regReplicaInp ) {
    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        int status;
        dataObjInfo_t *srcDataObjInfo;
        dataObjInfo_t *destDataObjInfo;
        int savedClientAuthFlag;

        srcDataObjInfo = regReplicaInp->srcDataObjInfo;
        destDataObjInfo = regReplicaInp->destDataObjInfo;
        if ( getValByKey( &regReplicaInp->condInput, SU_CLIENT_USER_KW ) != NULL ) {
            savedClientAuthFlag = rsComm->clientUser.authInfo.authFlag;
            rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
            status = chlRegReplica( rsComm, srcDataObjInfo, destDataObjInfo,
                                    &regReplicaInp->condInput );
            /* restore it */
            rsComm->clientUser.authInfo.authFlag = savedClientAuthFlag;
        }
        else {
            status = chlRegReplica( rsComm, srcDataObjInfo, destDataObjInfo, &regReplicaInp->condInput );
            if ( status >= 0 ) {
                status = destDataObjInfo->replNum;
            }
        }
        // =-=-=-=-=-=-=-
        // JMC - backport 4608
        if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ||
                status == CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME ) { // JMC - backport 4668, 4670
            int status2;
            /* register a repl with a copy with the same resource and phyPaht.
              * could be caused by 2 staging at the same time */
            status2 = checkDupReplica( rsComm, srcDataObjInfo->dataId,
                                       destDataObjInfo->rescName,
                                       destDataObjInfo->filePath );
            if ( status2 >= 0 ) {
                destDataObjInfo->replNum = status2; // JMC - backport 4668
                destDataObjInfo->dataId = srcDataObjInfo->dataId;
                return status2;
            }
        }
        // =-=-=-=-=-=-=-
        return status;
    } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        return SYS_NO_RCAT_SERVER_ERR;
    } else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        return SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }
}

int _call_file_modified_for_replica(
    rsComm_t*     rsComm,
    regReplica_t* regReplicaInp ) {
    int status = 0;
    dataObjInfo_t* destDataObjInfo = regReplicaInp->destDataObjInfo;

    irods::file_object_ptr file_obj(
        new irods::file_object(
            rsComm,
            destDataObjInfo ) );

    char* pdmo_kw = getValByKey( &regReplicaInp->condInput, IN_PDMO_KW );
    if ( pdmo_kw != NULL ) {
        file_obj->in_pdmo( pdmo_kw );
    }

    char* admin_kw = getValByKey( &regReplicaInp->condInput, ADMIN_KW );
    if ( admin_kw != NULL ) {
        addKeyVal( (keyValPair_t*)&file_obj->cond_input(), ADMIN_KW, "" );;
    }
    const auto open_type{getValByKey(&regReplicaInp->condInput, OPEN_TYPE_KW)};
    if (open_type) {
        addKeyVal((keyValPair_t*)&file_obj->cond_input(), OPEN_TYPE_KW, open_type);
    }
    irods::error ret = fileModified( rsComm, file_obj );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Failed to signal resource that the data object \"";
        msg << destDataObjInfo->objPath;
        msg << "\" was registered";
        ret = PASSMSG( msg.str(), ret );
        irods::log( ret );
        status = ret.code();
    }

    return status;

} // _call_file_modified_for_replica
