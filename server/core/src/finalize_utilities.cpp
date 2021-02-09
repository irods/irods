#include "finalize_utilities.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_exception.hpp"
#include "irods_re_structs.hpp"
#include "irods_resource_backport.hpp"
#include "irods_serialization.hpp"
#include "objDesc.hpp"
#include "physPath.hpp"
#include "rcMisc.h"
#include "rsDataObjTrim.hpp"
#include "rsModAVUMetadata.hpp"
#include "rsModAccessControl.hpp"
#include "rs_replica_close.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "data_object_proxy.hpp"
#include "replica_proxy.hpp"

#include <string>
#include <vector>

#include "json.hpp"

namespace irods
{
    auto apply_metadata_from_cond_input(RsComm& _comm, const DataObjInp& _inp) -> void
    {
        if ( const char* serialized_metadata = getValByKey( &_inp.condInput, METADATA_INCLUDED_KW ) ) {
            std::vector<std::string> deserialized_metadata = irods::deserialize_metadata( serialized_metadata );
            for ( size_t i = 0; i + 2 < deserialized_metadata.size(); i += 3 ) {
                modAVUMetadataInp_t modAVUMetadataInp;
                memset( &modAVUMetadataInp, 0, sizeof( modAVUMetadataInp ) );

                modAVUMetadataInp.arg0 = strdup( "add" );
                modAVUMetadataInp.arg1 = strdup( "-d" );
                modAVUMetadataInp.arg2 = strdup( _inp.objPath );
                modAVUMetadataInp.arg3 = strdup( deserialized_metadata[i].c_str() );
                modAVUMetadataInp.arg4 = strdup( deserialized_metadata[i + 1].c_str() );
                modAVUMetadataInp.arg5 = strdup( deserialized_metadata[i + 2].c_str() );
                int status = rsModAVUMetadata(&_comm, &modAVUMetadataInp );
                clearModAVUMetadataInp( &modAVUMetadataInp );
                if ( CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME != status && status < 0 ) {
                    THROW( status, "rsModAVUMetadata failed" );
                }
            }
        }
    } // apply_metadata_from_cond_input

    auto apply_acl_from_cond_input(RsComm& _comm, const DataObjInp& _inp) -> void
    {
        if ( const char* serialized_acl = getValByKey( &_inp.condInput, ACL_INCLUDED_KW ) ) {
            std::vector<std::vector<std::string> > deserialized_acl = irods::deserialize_acl( serialized_acl );
            for ( std::vector<std::vector<std::string> >::const_iterator iter = deserialized_acl.begin(); iter != deserialized_acl.end(); ++iter ) {
                modAccessControlInp_t modAccessControlInp;
                modAccessControlInp.recursiveFlag = 0;
                modAccessControlInp.accessLevel = strdup( ( *iter )[0].c_str() );
                modAccessControlInp.userName = ( char * )malloc( sizeof( char ) * NAME_LEN );
                modAccessControlInp.zone = ( char * )malloc( sizeof( char ) * NAME_LEN );
                const int status_parseUserName = parseUserName( ( *iter )[1].c_str(), modAccessControlInp.userName, modAccessControlInp.zone );
                if ( status_parseUserName < 0 ) {
                    THROW( status_parseUserName, "parseUserName failed" );
                }
                modAccessControlInp.path = strdup( _inp.objPath );
                int status = rsModAccessControl(&_comm, &modAccessControlInp );
                clearModAccessControlInp( &modAccessControlInp );
                if ( status < 0 ) {
                    THROW( status, "rsModAccessControl failed" );
                }
            }
        }
    } // apply_acl_from_cond_input

    auto verify_checksum(RsComm& _comm, DataObjInfo& _info, std::string_view _original_checksum) -> std::string
    {
        if (_original_checksum.empty()) {
            return {};
        }

        char* checksum_string = nullptr;
        irods::at_scope_exit free_checksum_string{[&checksum_string] { free(checksum_string); }};

        auto obj = irods::experimental::data_object::make_data_object_proxy(_info);
        auto replica = irods::experimental::replica::make_replica_proxy(*obj.get());

        replica.cond_input()[ORIG_CHKSUM_KW] = _original_checksum;

        if (const int ec = _dataObjChksum(&_comm, replica.get(), &checksum_string); ec < 0) {
            THROW(ec, "failed in _dataObjChksum");
        }

        replica.cond_input().erase(ORIG_CHKSUM_KW);

        // verify against the input value
        if (_original_checksum != checksum_string) {
            THROW(USER_CHKSUM_MISMATCH, fmt::format(
                "{}: mismatch chksum for {}.inp={},compute {}",
                __FUNCTION__, replica.logical_path(), _original_checksum, checksum_string));
        }

        return {checksum_string};
    } // verify_checksum

    auto register_new_checksum(RsComm& _comm, DataObjInfo& _info, std::string_view _original_checksum) -> std::string
    {
        char* checksum_string = nullptr;
        irods::at_scope_exit free_checksum_string{[&checksum_string] { free(checksum_string); }};

        auto obj = irods::experimental::data_object::make_data_object_proxy(_info);
        auto replica = irods::experimental::replica::make_replica_proxy(*obj.get());

        if (!_original_checksum.empty()) {
            replica.cond_input()[ORIG_CHKSUM_KW] = _original_checksum;
        }

        if (const int ec = _dataObjChksum(&_comm, replica.get(), &checksum_string ); ec < 0) {
            THROW(ec, "failed in _dataObjChksum");
        }

        if (!_original_checksum.empty()) {
            replica.cond_input().erase(ORIG_CHKSUM_KW);
        }

        if (!checksum_string) {
            irods::log(LOG_NOTICE, fmt::format(
                "[{}] - checksum returned empty for [{}]",
                __FUNCTION__, replica.logical_path()));
            return {};
        }

        return {checksum_string};
    } // register_new_checksum

    auto get_size_in_vault(RsComm& _comm, DataObjInfo& _info, const bool _verify_size, const rodsLong_t _recorded_size) -> rodsLong_t
    {
        const rodsLong_t size_in_vault = getSizeInVault(&_comm, &_info);

        // since we are not filtering out writes to archive resources, the
        // archive plugins report UNKNOWN_FILE_SZ as their size since they may
        // not be able to stat the file.  filter that out and trust the plugin
        // in this instance
        if (UNKNOWN_FILE_SZ == size_in_vault && _recorded_size >= 0) {
            return _recorded_size;
        }

        // an error occurred when getting size from storage
        if (size_in_vault < 0 && UNKNOWN_FILE_SZ != size_in_vault) {
            THROW(static_cast<int>(size_in_vault), fmt::format(
                "{}: getSizeInVault error for {}, status = {}",
                __FUNCTION__, _info.objPath, size_in_vault));
        }

        // check for consistency of the write operation
        if (_verify_size && size_in_vault != _recorded_size && _recorded_size > 0) {
            THROW(SYS_COPY_LEN_ERR, fmt::format(
                "{}: size in vault {} != target size {}",
                __FUNCTION__, size_in_vault, _recorded_size));
        }

        return size_in_vault;
    } // get_size_in_vault

    auto purge_cache(RsComm& _comm, DataObjInfo& _info) -> int
    {
        auto replica = irods::experimental::replica::make_replica_proxy(_info);

        DataObjInp inp{};
        rstrcpy( inp.objPath, replica.logical_path().data(), MAX_NAME_LEN );

        auto cond_input = irods::experimental::key_value_proxy{inp.condInput};
        irods::at_scope_exit free_cond_input{[&cond_input] { clearKeyVal(cond_input.get()); }};
        cond_input[COPIES_KW] = "1";
        cond_input[REPL_NUM_KW] = std::to_string(replica.replica_number());
        cond_input[RESC_HIER_STR_KW] = replica.hierarchy();

        int status = rsDataObjTrim(&_comm, &inp);
        if (status < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "{}: error trimming replica on [{}] for [{}]",
                __FUNCTION__, replica.hierarchy(), replica.logical_path()));
        }
        return status;
    } // purge_cache

    auto duplicate_l1_descriptor(const l1desc& _src) -> l1desc
    {
        l1desc dest{};
        std::memcpy(&dest, &_src, sizeof(l1desc));

        if (_src.dataObjInp) {
            DataObjInp* doi = static_cast<DataObjInp*>(std::malloc(sizeof(DataObjInp)));
            replDataObjInp(_src.dataObjInp, doi);
            dest.dataObjInp = doi;
        }

        if (_src.dataObjInfo) {
            auto [obj, obj_lm] = irods::experimental::replica::duplicate_replica(*_src.dataObjInfo);
            dest.dataObjInfo = obj_lm.release();
        }

        dest.remoteZoneHost = nullptr;
        dest.otherDataObjInfo = nullptr;
        dest.replDataObjInfo = nullptr;

        // TODO: need duplication logic
        //if (_src.remoteZoneHost) {
            //rodsServerHost* rzh{};
            //std::malloc(sizeof(rodsServerHost));
        //}

        return dest;
    } // duplicate_l1_descriptor

    auto stale_replica(RsComm& _comm, const l1desc& _l1desc, DataObjInfo& _info) -> int
    {
        _info.replStatus = STALE_REPLICA;

        keyValPair_t regParam{};
        auto kvp = irods::experimental::make_key_value_proxy(regParam);

        kvp[IN_PDMO_KW] = _info.rescHier;
        kvp[REPL_STATUS_KW] = std::to_string(_info.replStatus);

        const auto cond_input = irods::experimental::make_key_value_proxy(_l1desc.dataObjInp->condInput);
        if (cond_input.contains(ADMIN_KW)) {
            kvp[ADMIN_KW] = "";
        }

        modDataObjMeta_t inp{};
        inp.dataObjInfo = &_info;
        inp.regParam = kvp.get();

        return rsModDataObjMeta(&_comm, &inp);
    } // stale_replica

    auto apply_static_post_pep(RsComm& _comm, l1desc& _l1desc, const int _operation_status, std::string_view _pep_name) -> int
    {
        RuleExecInfo rei{};
        initReiWithDataObjInp(&rei, &_comm, _l1desc.dataObjInp);
        rei.doi = _l1desc.dataObjInfo;
        rei.status = _operation_status;

        // make resource properties available as rule session variables
        irods::get_resc_properties_as_kvp(rei.doi->rescHier, rei.condInputData);

        rei.status = applyRule(_pep_name.data(), NULL, &rei, NO_SAVE_REI );

        /* doi might have changed */
        _l1desc.dataObjInfo = rei.doi;
        clearKeyVal(rei.condInputData);
        free(rei.condInputData);

        return rei.status;
    } // apply_static_pep

    auto close_replica_without_catalog_update(RsComm& _comm, const int _fd) -> int
    {
        nlohmann::json in_json;
        in_json["fd"] = _fd;
        in_json["send_notifications"] = false;
        in_json["update_size"] = false;
        in_json["update_status"] = false;

        const int ec = rs_replica_close(&_comm, in_json.dump().data());

        if (ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}] - error closing replica; ec:[{}]",
                __FUNCTION__, ec));
        }

        return ec;
    } // close_replica_without_catalog_update
} // namespace irods
