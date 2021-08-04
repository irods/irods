#include "irods_repl_rebalance.hpp"
#include "irods_resource_plugin.hpp"
#include "irods_file_object.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_virtual_path.hpp"
#include "irods_repl_retry.hpp"
#include "irods_repl_types.hpp"
#include "icatHighLevelRoutines.hpp"
#include "dataObjRepl.h"
#include "genQuery.h"
#include "rsGenQuery.hpp"
#include "boost/format.hpp"
#include "boost/lexical_cast.hpp"
#include "rodsError.h"


namespace {
    irods::error repl_for_rebalance(
        irods::plugin_context& _ctx,
        const std::string& _obj_path,
        const std::string& _current_resc,
        const std::string& _src_hier,
        const std::string& _dst_hier,
        const std::string& _src_resc,
        const std::string& _dst_resc,
        const int          _mode ) {
        // =-=-=-=-=-=-=-
        // generate a resource hierarchy that ends at this resource for pdmo
        irods::hierarchy_parser parser;
        parser.set_string( _src_hier );
        std::string sub_hier;
        parser.str( sub_hier, _current_resc );

        dataObjInp_t data_obj_inp{};
        rstrcpy( data_obj_inp.objPath, _obj_path.c_str(), MAX_NAME_LEN );
        data_obj_inp.createMode = _mode;
        addKeyVal( &data_obj_inp.condInput, RESC_HIER_STR_KW,      _src_hier.c_str() );
        addKeyVal( &data_obj_inp.condInput, DEST_RESC_HIER_STR_KW, _dst_hier.c_str() );
        addKeyVal( &data_obj_inp.condInput, RESC_NAME_KW,          _src_resc.c_str() );
        addKeyVal( &data_obj_inp.condInput, DEST_RESC_NAME_KW,     _dst_resc.c_str() );
        addKeyVal( &data_obj_inp.condInput, IN_PDMO_KW,             sub_hier.c_str() );
        addKeyVal( &data_obj_inp.condInput, ADMIN_KW,              "" );

        try {
            // =-=-=-=-=-=-=-
            // process the actual call for replication
            const auto status = data_obj_repl_with_retry( _ctx, data_obj_inp );
            if ( status < 0 ) {
                return ERROR( status,
                              boost::format( "Failed to replicate the data object [%s]" ) %
                              _obj_path );
            }
        }
        catch( const irods::exception& e ) {
            irods::log(e);
            return irods::error( e );
        }

        return SUCCESS();
    }

    // throws irods::exception
    sqlResult_t* extract_sql_result(const genQueryInp_t& genquery_inp, genQueryOut_t* genquery_out_ptr, const int column_number) {
        if (sqlResult_t *sql_result = getSqlResultByInx(genquery_out_ptr, column_number)) {
            return sql_result;
        }
        THROW(
            NULL_VALUE_ERR,
            boost::format("getSqlResultByInx failed. column [%d] genquery_inp contents:\n%s\n\n possible iquest [%s]") %
            column_number %
            genquery_inp_to_diagnostic_string(&genquery_inp) %
            genquery_inp_to_iquest_string(&genquery_inp));
    };

    struct ReplicationSourceInfo {
        std::string object_path;
        std::string resource_hierarchy;
        int data_mode;
    };

    // throws irods::exception
    ReplicationSourceInfo get_source_data_object_attributes(
        rsComm_t* _comm,
        const rodsLong_t _data_id,
        const std::vector<leaf_bundle_t>& _leaf_bundles) {

        if (!_comm) {
            THROW(SYS_INTERNAL_NULL_INPUT_ERR, "null comm ptr");
        }

        irods::GenQueryInpWrapper genquery_inp_wrapped;
        genquery_inp_wrapped.get().maxRows = MAX_SQL_ROWS;
        addInxVal(&genquery_inp_wrapped.get().sqlCondInp, COL_D_DATA_ID, (boost::format("= '%lld'") % _data_id).str().c_str());

        std::stringstream cond_ss;
        for (auto& bun : _leaf_bundles) {
            for (auto id : bun) {
                cond_ss << "= '" << id << "' || ";
            }
        }
        std::string cond_str = cond_ss.str().substr(0, cond_ss.str().size()-4);
        addInxVal(&genquery_inp_wrapped.get().sqlCondInp, COL_D_RESC_ID, cond_str.c_str());

        addInxVal(&genquery_inp_wrapped.get().sqlCondInp, COL_D_REPL_STATUS, (boost::format("= '%d'") % GOOD_REPLICA).str().c_str());

        addInxIval(&genquery_inp_wrapped.get().selectInp, COL_DATA_NAME, 1);
        addInxIval(&genquery_inp_wrapped.get().selectInp, COL_COLL_NAME, 1);
        addInxIval(&genquery_inp_wrapped.get().selectInp, COL_DATA_MODE, 1);
        addInxIval(&genquery_inp_wrapped.get().selectInp, COL_D_RESC_ID, 1);

        irods::GenQueryOutPtrWrapper genquery_out_ptr_wrapped;
        const int status_rsGenQuery = rsGenQuery(_comm, &genquery_inp_wrapped.get(), &genquery_out_ptr_wrapped.get());
        if (status_rsGenQuery < 0) {
            THROW(
                status_rsGenQuery,
                boost::format("rsGenQuery failed. genquery_inp contents:\n%s\n\n possible iquest [%s]") %
                genquery_inp_to_diagnostic_string(&genquery_inp_wrapped.get()) %
                genquery_inp_to_iquest_string(&genquery_inp_wrapped.get()));
        }

        if (!genquery_out_ptr_wrapped.get()) {
            THROW(
                SYS_INTERNAL_NULL_INPUT_ERR,
                boost::format("rsGenQuery failed. genquery_inp contents:\n%s\n\n possible iquest [%s]") %
                genquery_inp_to_diagnostic_string(&genquery_inp_wrapped.get()) %
                genquery_inp_to_iquest_string(&genquery_inp_wrapped.get()));
        }


        sqlResult_t *data_name_result = extract_sql_result(genquery_inp_wrapped.get(), genquery_out_ptr_wrapped.get(), COL_DATA_NAME);
        char *data_name = &data_name_result->value[0];

        sqlResult_t *coll_name_result = extract_sql_result(genquery_inp_wrapped.get(), genquery_out_ptr_wrapped.get(), COL_COLL_NAME);
        char *coll_name = &coll_name_result->value[0];

        auto cast_genquery_result = [&genquery_inp_wrapped](char *s) -> rodsLong_t {
            try {
                return boost::lexical_cast<rodsLong_t>(s);
            } catch (const boost::bad_lexical_cast&) {
                THROW(
                    INVALID_LEXICAL_CAST,
                    boost::format("boost::lexical_cast failed. tried to cast [%s]. genquery_inp contents:\n%s\n\n possible iquest [%s]") %
                    s %
                    genquery_inp_to_diagnostic_string(&genquery_inp_wrapped.get()) %
                    genquery_inp_to_iquest_string(&genquery_inp_wrapped.get()));
            }
        };

        sqlResult_t *resc_id_result = extract_sql_result(genquery_inp_wrapped.get(), genquery_out_ptr_wrapped.get(), COL_D_RESC_ID);
        const rodsLong_t resc_id = cast_genquery_result(&resc_id_result->value[0]);

        sqlResult_t *data_mode_result = extract_sql_result(genquery_inp_wrapped.get(), genquery_out_ptr_wrapped.get(), COL_DATA_MODE);
        const int data_mode = cast_genquery_result(&data_mode_result->value[0]);

        ReplicationSourceInfo ret;
        ret.object_path = (boost::format("%s%s%s") % coll_name % irods::get_virtual_path_separator() % data_name).str();
        irods::error err = resc_mgr.leaf_id_to_hier(resc_id, ret.resource_hierarchy);
        if (!err.ok()) {
            THROW(err.code(),
                  boost::format("leaf_id_to_hier failed. resc id [%lld] genquery inp:\n%s") %
                  resc_id %
                  genquery_inp_to_diagnostic_string(&genquery_inp_wrapped.get()));
        }
        ret.data_mode = data_mode;
        return ret;
    }

    struct ReplicaAndRescId {
        rodsLong_t data_id;
        rodsLong_t replica_number;
        rodsLong_t resource_id;
    };

    // throws irods::exception
    std::vector<ReplicaAndRescId> get_out_of_date_replicas_batch(
        rsComm_t* _comm,
        const std::vector<leaf_bundle_t>& _bundles,
        const std::string& _invocation_timestamp,
        const int _batch_size) {
        if (!_comm) {
            THROW(SYS_INTERNAL_NULL_INPUT_ERR, "null rsComm");
        }
        if (_bundles.empty()) {
            THROW(SYS_INVALID_INPUT_PARAM, "empty bundles");
        }
        if (_batch_size <= 0) {
            THROW(SYS_INVALID_INPUT_PARAM, boost::format("invalid batch size [%d]") % _batch_size);
        }
        if (_invocation_timestamp.empty()) {
            THROW(SYS_INVALID_INPUT_PARAM, "empty invocation timestamp");
        }

        irods::GenQueryInpWrapper genquery_inp_wrapped;
        genquery_inp_wrapped.get().maxRows = _batch_size;

        std::stringstream cond_ss;
        for (auto& bun : _bundles) {
            for (auto id : bun) {
                cond_ss << "= '" << id << "' || ";
            }
        }
        const std::string cond_str = cond_ss.str().substr(0, cond_ss.str().size()-4);
        addInxVal(&genquery_inp_wrapped.get().sqlCondInp, COL_D_RESC_ID, cond_str.c_str());
        addInxVal(&genquery_inp_wrapped.get().sqlCondInp, COL_D_REPL_STATUS, (boost::format("= '%d'") % STALE_REPLICA).str().c_str());
        const std::string timestamp_str = "<= '" + _invocation_timestamp + "'";
        addInxVal(&genquery_inp_wrapped.get().sqlCondInp, COL_D_MODIFY_TIME, timestamp_str.c_str());
        addInxIval(&genquery_inp_wrapped.get().selectInp, COL_D_DATA_ID, 1);
        addInxIval(&genquery_inp_wrapped.get().selectInp, COL_DATA_REPL_NUM, 1);
        addInxIval(&genquery_inp_wrapped.get().selectInp, COL_D_RESC_ID, 1);

        irods::GenQueryOutPtrWrapper genquery_out_ptr_wrapped;
        const int status_rsGenQuery = rsGenQuery(_comm, &genquery_inp_wrapped.get(), &genquery_out_ptr_wrapped.get());

        std::vector<ReplicaAndRescId> ret;
        if (CAT_NO_ROWS_FOUND == status_rsGenQuery) {
            return ret;
        } else if (status_rsGenQuery < 0) {
            THROW(
                status_rsGenQuery,
                boost::format("rsGenQuery failed. genquery_inp contents:\n%s\npossible iquest [%s]") %
                genquery_inp_to_diagnostic_string(&genquery_inp_wrapped.get()) %
                genquery_inp_to_iquest_string(&genquery_inp_wrapped.get()));
        } else if (nullptr == genquery_out_ptr_wrapped.get()) {
            THROW(
                SYS_INTERNAL_NULL_INPUT_ERR,
                boost::format("rsGenQuery failed. genquery_inp contents:\n%s\npossible iquest [%s]") %
                genquery_inp_to_diagnostic_string(&genquery_inp_wrapped.get()) %
                genquery_inp_to_iquest_string(&genquery_inp_wrapped.get()));
        }

        sqlResult_t *data_id_results       = extract_sql_result(genquery_inp_wrapped.get(), genquery_out_ptr_wrapped.get(), COL_D_DATA_ID);
        sqlResult_t *data_repl_num_results = extract_sql_result(genquery_inp_wrapped.get(), genquery_out_ptr_wrapped.get(), COL_DATA_REPL_NUM);
        sqlResult_t *data_resc_id_results  = extract_sql_result(genquery_inp_wrapped.get(), genquery_out_ptr_wrapped.get(), COL_D_RESC_ID);

        ret.reserve(genquery_out_ptr_wrapped.get()->rowCnt);
        for (int i=0; i<genquery_out_ptr_wrapped.get()->rowCnt; ++i) {
            ReplicaAndRescId repl_and_resc;
            auto cast_genquery_result = [&genquery_inp_wrapped](int i, char *s) {
                try {
                    return boost::lexical_cast<rodsLong_t>(s);
                } catch (const boost::bad_lexical_cast&) {
                    THROW(
                        INVALID_LEXICAL_CAST,
                        boost::format("boost::lexical_cast failed. index [%d]. tried to cast [%s]. genquery_inp contents:\n%s\npossible iquest [%s]") %
                        i %
                        s %
                        genquery_inp_to_diagnostic_string(&genquery_inp_wrapped.get()) %
                        genquery_inp_to_iquest_string(&genquery_inp_wrapped.get()));
                }
            };
            repl_and_resc.data_id        = cast_genquery_result(i, &data_id_results      ->value[data_id_results      ->len * i]);
            repl_and_resc.replica_number = cast_genquery_result(i, &data_repl_num_results->value[data_repl_num_results->len * i]);
            repl_and_resc.resource_id    = cast_genquery_result(i, &data_resc_id_results ->value[data_resc_id_results ->len * i]);
            ret.push_back(repl_and_resc);
        }
        return ret;
    }

    // throws irods::exception
    std::string get_child_name_that_is_ancestor_of_bundle(
        const std::string&   _resc_name,
        const leaf_bundle_t& _bundle) {
        std::string hier;
        irods::error err = resc_mgr.leaf_id_to_hier(_bundle[0], hier);
        if (!err.ok()) {
            THROW(err.code(), err.result());
        }

        irods::hierarchy_parser parse;
        err = parse.set_string(hier);
        if (!err.ok()) {
            THROW(err.code(), err.result());
        }

        std::string ret;
        err = parse.next(_resc_name, ret);
        if (!err.ok()) {
            THROW(err.code(), err.result());
        }
        return ret;
    }

    std::string leaf_bundles_to_string(
        const std::vector<leaf_bundle_t>& _leaf_bundles) {
        std::stringstream ss;
        for (auto& b : _leaf_bundles) {
            ss << '[';
            for (auto d : b) {
                ss << d << ", ";
            }
            ss << "], ";
        }
        return ss.str();
    }

    // throws irods::exception
    void proc_results_for_rebalance(
        irods::plugin_context&            _ctx,
        const std::string&                _parent_resc_name,
        const std::string&                _child_resc_name,
        const size_t                      _bun_idx,
        const std::vector<leaf_bundle_t>& _bundles,
        const dist_child_result_t&        _data_ids_to_replicate) {
        if (!_ctx.comm()) {
            THROW(SYS_INVALID_INPUT_PARAM,
                  boost::format("null comm pointer. resource [%s]. child resource [%s]. bundle index [%d]. bundles [%s]") %
                  _parent_resc_name %
                  _child_resc_name %
                  _bun_idx %
                  leaf_bundles_to_string(_bundles));
        }

        if (_data_ids_to_replicate.empty()) {
            THROW(SYS_INVALID_INPUT_PARAM,
                  boost::format("empty data id list. resource [%s]. child resource [%s]. bundle index [%d]. bundles [%s]") %
                  _parent_resc_name %
                  _child_resc_name %
                  _bun_idx %
                  leaf_bundles_to_string(_bundles));
        }

        //irods::file_object_ptr file_obj{boost::dynamic_pointer_cast<irods::file_object>(_ctx.fco())};

        irods::error first_rebalance_error = SUCCESS();
        for (auto data_id_to_replicate : _data_ids_to_replicate) {
            const ReplicationSourceInfo source_info = get_source_data_object_attributes(_ctx.comm(), data_id_to_replicate, _bundles);

            // create a file object so we can resolve a valid hierarchy to which to replicate
            irods::file_object_ptr f_ptr(new irods::file_object(_ctx.comm(), source_info.object_path, "", "", 0, source_info.data_mode, 0));
            // short circuit the magic re-repl
            f_ptr->in_pdmo(irods::hierarchy_parser{source_info.resource_hierarchy}.str(_parent_resc_name));

            // init the parser with the fragment of the upstream hierarchy not including the repl node as it should add itself
            const size_t pos = source_info.resource_hierarchy.find(_parent_resc_name);
            if (std::string::npos == pos) {
                THROW(SYS_INVALID_INPUT_PARAM, boost::format("missing repl name [%s] in source hier string [%s]") % _parent_resc_name % source_info.resource_hierarchy);
            }

            // Trim hierarchy up to parent because the resource adds itself later in hierarchy resolution
            std::string src_frag = irods::hierarchy_parser{source_info.resource_hierarchy}.str(_parent_resc_name);
            irods::hierarchy_parser parser{src_frag};

            // resolve the target child resource plugin
            irods::resource_ptr dst_resc;
            const irods::error err_resolve = resc_mgr.resolve(_child_resc_name, dst_resc);
            if (!err_resolve.ok()) {
                THROW(err_resolve.code(), boost::format("failed to resolve resource plugin. child resc [%s] parent resc [%s] bundle index [%d] bundles [%s] data id [%lld]. resolve message [%s]") %
                      _child_resc_name %
                      _parent_resc_name %
                      _bun_idx %
                      leaf_bundles_to_string(_bundles) %
                      data_id_to_replicate %
                      err_resolve.result());
            }

            // then we need to query the target resource and ask it to determine a dest resc hier for the repl
            std::string host_name{};
            float vote = 0.0;
            const irods::error err_vote = dst_resc->call<const std::string*, const std::string*, irods::hierarchy_parser*, float*>(
                _ctx.comm(),
                irods::RESOURCE_OP_RESOLVE_RESC_HIER,
                f_ptr,
                &irods::CREATE_OPERATION,
                &host_name,
                &parser,
                &vote );
            if (!err_vote.ok()) {
                THROW(err_resolve.code(), boost::format("failed to get dest hierarchy. child resc [%s] parent resc [%s] bundle index [%d] bundles [%s] data id [%lld]. vote message [%s]") %
                      _child_resc_name %
                      _parent_resc_name %
                      _bun_idx %
                      leaf_bundles_to_string(_bundles) %
                      data_id_to_replicate %
                      err_vote.result());
            }

            const std::string root_resc = parser.first_resc();
            const std::string dst_hier = parser.str();
            rodsLog(LOG_NOTICE, "%s: creating new replica for data id [%lld] (%s) from [%s] on [%s]", __FUNCTION__, data_id_to_replicate, source_info.object_path.c_str(), source_info.resource_hierarchy.c_str(), dst_hier.c_str());

            const irods::error err_rebalance = repl_for_rebalance(
                _ctx,
                source_info.object_path,
                _parent_resc_name,
                source_info.resource_hierarchy,
                dst_hier,
                root_resc,
                root_resc,
                source_info.data_mode);
            if (!err_rebalance.ok()) {
                if (first_rebalance_error.ok()) {
                    first_rebalance_error = err_rebalance;
                }
                rodsLog(LOG_ERROR, "%s: repl_for_rebalance failed. object path [%s] parent resc [%s] source hier [%s] dest hier [%s] root resc [%s] data mode [%d]",
                        __FUNCTION__, source_info.object_path.c_str(), _parent_resc_name.c_str(), source_info.resource_hierarchy.c_str(), dst_hier.c_str(), root_resc.c_str(), source_info.data_mode);
                irods::log(PASS(err_rebalance));
                if (_ctx.comm()->rError.len < MAX_ERROR_MESSAGES) {
                    addRErrorMsg(&_ctx.comm()->rError, err_rebalance.code(), err_rebalance.result().c_str());
                }
            }
        }

        if (!first_rebalance_error.ok()) {
            THROW(first_rebalance_error.code(),
                  boost::format("%s: repl_for_rebalance failed. child_resc [%s] parent resc [%s]. rebalance message [%s]") %
                  __FUNCTION__ %
                  _child_resc_name %
                  _parent_resc_name %
                  first_rebalance_error.result());
        }
    }
}

namespace irods {
    // throws irods::exception
    void update_out_of_date_replicas(
        irods::plugin_context& _ctx,
        const std::vector<leaf_bundle_t>& _leaf_bundles,
        const int _batch_size,
        const std::string& _invocation_timestamp,
        const std::string& _resource_name) {

        while (true) {
            const std::vector<ReplicaAndRescId> replicas_to_update = get_out_of_date_replicas_batch(_ctx.comm(), _leaf_bundles, _invocation_timestamp, _batch_size);
            if (replicas_to_update.empty()) {
                break;
            }

            error first_error = SUCCESS();
            for (const auto& replica_to_update : replicas_to_update) {
                std::string destination_hierarchy;
                const error err_dst_hier = resc_mgr.leaf_id_to_hier(replica_to_update.resource_id, destination_hierarchy);
                if (!err_dst_hier.ok()) {
                    THROW(err_dst_hier.code(),
                          boost::format("leaf_id_to_hier failed. data id [%lld]. replica number [%d] resource id [%lld]") %
                          replica_to_update.data_id %
                          replica_to_update.replica_number %
                          replica_to_update.resource_id);
                }

                ReplicationSourceInfo source_info = get_source_data_object_attributes(_ctx.comm(), replica_to_update.data_id, _leaf_bundles);
                hierarchy_parser hierarchy_parser;
                const error err_parser = hierarchy_parser.set_string(source_info.resource_hierarchy);
                if (!err_parser.ok()) {
                    THROW(
                        err_parser.code(),
                        boost::format("set_string failed. resource hierarchy [%s]. object path [%s]") %
                                      source_info.resource_hierarchy %
                        source_info.object_path);
                }
                std::string root_resc;
                const error err_first_resc = hierarchy_parser.first_resc(root_resc);
                if (!err_first_resc.ok()) {
                    THROW(
                        err_first_resc.code(),
                        boost::format("first_resc failed. resource hierarchy [%s]. object path [%s]") %
                        source_info.resource_hierarchy %
                        source_info.object_path);
                }

                rodsLog(LOG_NOTICE, "update_out_of_date_replicas: updating out-of-date replica for data id [%ji] from [%s] to [%s]",
                        static_cast<intmax_t>(replica_to_update.data_id),
                        source_info.resource_hierarchy.c_str(),
                        destination_hierarchy.c_str());
                const error err_repl = repl_for_rebalance(
                    _ctx,
                    source_info.object_path,
                    _resource_name,
                    source_info.resource_hierarchy,
                    destination_hierarchy,
                    root_resc,
                    root_resc,
                    source_info.data_mode);

                if (!err_repl.ok()) {
                    if (first_error.ok()) {
                        first_error = err_repl;
                    }
                    const error error_to_log = PASS(err_repl);
                    if (_ctx.comm()->rError.len < MAX_ERROR_MESSAGES) {
                        addRErrorMsg(&_ctx.comm()->rError, error_to_log.code(), error_to_log.result().c_str());
                    }
                    rodsLog(LOG_ERROR,
                            "update_out_of_date_replicas: repl_for_rebalance failed with code [%ji] and message [%s]. object [%s] source hierarchy [%s] data id [%ji] destination repl num [%ji] destination hierarchy [%s]",
                            static_cast<intmax_t>(err_repl.code()), err_repl.result().c_str(), source_info.object_path.c_str(), source_info.resource_hierarchy.c_str(),
                            static_cast<intmax_t>(replica_to_update.data_id), static_cast<intmax_t>(replica_to_update.replica_number), destination_hierarchy.c_str());
                }
            }
            if (!first_error.ok()) {
                THROW(first_error.code(), first_error.result());
            }
        }
    }

    // throws irods::exception
    void create_missing_replicas(
        irods::plugin_context& _ctx,
        const std::vector<leaf_bundle_t>& _leaf_bundles,
        const int _batch_size,
        const std::string& _invocation_timestamp,
        const std::string& _resource_name) {
        for (size_t i=0; i<_leaf_bundles.size(); ++i) {
            const std::string child_name = get_child_name_that_is_ancestor_of_bundle(_resource_name, _leaf_bundles[i]);
            while (true) {
                dist_child_result_t data_ids_needing_new_replicas;
                const int status_chlGetReplListForLeafBundles = chlGetReplListForLeafBundles(_batch_size, i, &_leaf_bundles, &_invocation_timestamp, &data_ids_needing_new_replicas);
                if (status_chlGetReplListForLeafBundles != 0) {
                    THROW(status_chlGetReplListForLeafBundles,
                          boost::format("failed to get data objects needing new replicas for resource [%s] bundle index [%d] bundles [%s]")
                          % _resource_name
                          % i
                          % leaf_bundles_to_string(_leaf_bundles));
                }
                if (data_ids_needing_new_replicas.empty()) {
                    break;
                }

                proc_results_for_rebalance(_ctx, _resource_name, child_name, i, _leaf_bundles, data_ids_needing_new_replicas);
            }
        }
    }
} // namespace irods
