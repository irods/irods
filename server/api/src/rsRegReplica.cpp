#include "rsRegReplica.hpp"

#include "icatHighLevelRoutines.hpp"
#include "irods_configuration_keywords.hpp"
#include "irods_file_object.hpp"
#include "irods_hierarchy_parser.hpp"
#include "miscServerFunct.hpp"
#include "objMetaOpr.hpp"
#include "regReplica.h"
#include "rsFileChksum.hpp"
#include "rsFileStat.hpp"
#include "rsModDataObjMeta.hpp"
#include "rsUnregDataObj.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "replica_proxy.hpp"

#include "boost/lexical_cast.hpp"

#include "fmt/format.h"

namespace
{
    std::string compute_checksum_for_resc(
        rsComm_t&              _comm,
        const std::string_view _logical_path,
        const std::string_view _physical_path,
        const std::string_view _resc_hier,
        const rodsLong_t&      _data_size)
    {
        fileChksumInp_t chk_inp{};
        rstrcpy( chk_inp.objPath, _logical_path.data(), MAX_NAME_LEN);
        rstrcpy( chk_inp.fileName, _physical_path.data(), MAX_NAME_LEN);
        rstrcpy( chk_inp.rescHier, _resc_hier.data(), MAX_NAME_LEN);
        chk_inp.dataSize = _data_size;

        char* chksum{};
        const auto chksum_err = rsFileChksum(&_comm, &chk_inp, &chksum);

        if(DIRECT_ARCHIVE_ACCESS == chksum_err) {
            return "";
        }

        if(chksum_err < 0) {
            THROW(chksum_err, fmt::format(
                  "[{}:{}] - rsDataObjChksum failed for [{}] on [{}]",
                  __func__, __LINE__, _logical_path, _resc_hier));
        }

        return chksum;
    } // compute_checksum_for_resc

    void verify_and_update_replica(RsComm& _comm, regReplica_t& _inp)
    {
        namespace ir = irods::experimental::replica;

        auto source = ir::make_replica_proxy(*_inp.srcDataObjInfo);
        auto dest = ir::make_replica_proxy(*_inp.destDataObjInfo);

        const auto cond_input = irods::experimental::make_key_value_proxy(_inp.condInput);

        keyValPair_t kvp{};
        auto reg_param = irods::experimental::make_key_value_proxy(kvp);
        if (cond_input.contains(DATA_SIZE_KW)) {
            const auto data_size = cond_input.at(DATA_SIZE_KW).value();
            try {
                dest.size(boost::lexical_cast<decltype(dest.size())>(data_size));
                reg_param[DATA_SIZE_KW] = data_size;
            }
            catch (const boost::bad_lexical_cast&) {
                irods::log(LOG_ERROR, fmt::format(
                           "[{}:{}] - bad_lexical_cast for dataSize [{}]; setting to 0",
                           __func__, __LINE__, data_size));
                dest.size(0);
                reg_param.erase(DATA_SIZE_KW);
            }
        }

        if (!reg_param.contains(DATA_SIZE_KW)) {
            namespace fs = irods::experimental::filesystem;

            const auto dst_size = ir::get_replica_size_from_storage(_comm,
                                                                    fs::path{dest.logical_path().data()},
                                                                    dest.hierarchy(),
                                                                    dest.physical_path());

            if (UNKNOWN_FILE_SZ != dst_size && dst_size != source.size()) {
                dest.size(dst_size);
                reg_param[DATA_SIZE_KW] = std::to_string(dst_size);
            }
        }

        // optional checksum verification, should one exist on the source replica
        std::string dst_checksum;
        if (!source.checksum().empty()) {
            dst_checksum = compute_checksum_for_resc(_comm,
                                                     dest.logical_path(),
                                                     dest.physical_path(),
                                                     dest.hierarchy(),
                                                     dest.size());
            if (!dst_checksum.empty() && source.checksum() != dst_checksum) {
                dest.checksum(dst_checksum);
                reg_param[CHKSUM_KW] = dst_checksum;
            }
        }

        if (!reg_param.empty()) {
            reg_param[ALL_REPL_STATUS_KW] = "TRUE";

            modDataObjMeta_t mod_inp{};
            mod_inp.dataObjInfo = dest.get();
            mod_inp.regParam    = reg_param.get();

            if (const auto ec = rsModDataObjMeta(&_comm, &mod_inp); ec < 0) {
                THROW(ec, fmt::format(
                    "[{}:{}] - rsModDataObjMeta failed for [{}] on resc [{}]",
                    __func__, __LINE__, dest.logical_path(), dest.hierarchy()));
            }
        }
    } // verify_and_update_replica

    int _rsRegReplica(RsComm& _comm, regReplica_t& _inp)
    {
        int status;

        auto cond_input = irods::experimental::make_key_value_proxy(_inp.condInput);
        auto* source_doi = _inp.srcDataObjInfo;
        auto* dest_doi = _inp.destDataObjInfo;
        if (cond_input.contains(SU_CLIENT_USER_KW)) {
            const int savedClientAuthFlag = _comm.clientUser.authInfo.authFlag;

            _comm.clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;

            status = chlRegReplica(&_comm, source_doi, dest_doi, cond_input.get());

            _comm.clientUser.authInfo.authFlag = savedClientAuthFlag;
        }
        else {
            status = chlRegReplica(&_comm, source_doi, dest_doi, cond_input.get());

            if ( status >= 0 ) {
                status = dest_doi->replNum;
            }
        }

        if (status != CAT_SUCCESS_BUT_WITH_NO_INFO && status != CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME) {
            return status;
        }

        // =-=-=-=-=-=-=-
        // JMC - backport 4608
        /* register a repl with a copy with the same resource and phyPaht.
          * could be caused by 2 staging at the same time */
        if (const auto ec = checkDupReplica(&_comm, source_doi->dataId, dest_doi->rescName, dest_doi->filePath);
            ec >= 0) {
            dest_doi->replNum = ec; // JMC - backport 4668
            dest_doi->dataId = source_doi->dataId;
            return ec;
        }

        return status;
    } // _rsRegReplica

    int _call_file_modified_for_replica(
        rsComm_t*     rsComm,
        regReplica_t* regReplicaInp )
    {
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
} // anonymous namespace

int rsRegReplica(rsComm_t *rsComm, regReplica_t *regReplicaInp)
{
    rodsServerHost_t *rodsServerHost = nullptr;
    const auto status = getAndConnRcatHost(rsComm,
                                           MASTER_RCAT,
                                           regReplicaInp->srcDataObjInfo->objPath,
                                           &rodsServerHost);
    if (status < 0 || !rodsServerHost) {
        return status;
    }

    auto cond_input = irods::experimental::make_key_value_proxy(regReplicaInp->condInput);

    if (rodsServerHost->localFlag != LOCAL_HOST) {
        // Add IN_REPL_KW to prevent replication on the redirected server (the provider)
        const auto in_repl = cond_input.contains(IN_REPL_KW);

        if (!in_repl) {
            cond_input[IN_REPL_KW] = "";
        }

        const auto ec = rcRegReplica(rodsServerHost->conn, regReplicaInp);
        if (ec < 0) {
            irods::log(LOG_ERROR, fmt::format("[{}:{}] - failed registering replica [{}] ec=[{}]",
                        __func__, __LINE__, regReplicaInp->destDataObjInfo->objPath, ec));
            cond_input.erase(IN_REPL_KW);
            return ec;
        }

        if (in_repl) {
            return ec;
        }

        // Remove the keyword as we will want to replicate on this server (the consumer)
        cond_input.erase(IN_REPL_KW);
        regReplicaInp->destDataObjInfo->replNum = ec;

        if (cond_input.contains(REGISTER_AS_INTERMEDIATE_KW)) {
            return ec;
        }

        return _call_file_modified_for_replica( rsComm, regReplicaInp );
    }

    std::string svc_role;
    if (const auto ret = get_catalog_service_role(svc_role); !ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if (irods::CFG_SERVICE_ROLE_CONSUMER == svc_role) {
        return SYS_NO_RCAT_SERVER_ERR;
    }

    if (irods::CFG_SERVICE_ROLE_PROVIDER != svc_role) {
        rodsLog(LOG_ERROR, "role not supported [%s]", svc_role.c_str());
        return SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }

    const int ec = _rsRegReplica(*rsComm, *regReplicaInp);
    if (ec < 0 || cond_input.contains(REGISTER_AS_INTERMEDIATE_KW)) {
        return ec;
    }

    try {
        verify_and_update_replica(*rsComm, *regReplicaInp);
    }
    catch (const irods::exception& e) {
        irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __func__, __LINE__, e.client_display_what()));

        unregDataObj_t unlink_inp{};
        unlink_inp.dataObjInfo = regReplicaInp->destDataObjInfo;
        if (const auto unreg_ec = rsUnregDataObj(rsComm, &unlink_inp); unreg_ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to unregister replica "
                "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                __func__, __LINE__, unreg_ec,
                regReplicaInp->destDataObjInfo->objPath,
                regReplicaInp->destDataObjInfo->rescHier));
        }

        return e.code();
    }

    if (!cond_input.contains(IN_REPL_KW)) {
        return _call_file_modified_for_replica( rsComm, regReplicaInp );
    }

    return ec;
} // rsRegReplica
