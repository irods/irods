// =-=-=-=-=-=-=-
#include "irods_repl_rebalance.hpp"
#include "irods_resource_plugin.hpp"
#include "irods_file_object.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_virtual_path.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "dataObjRepl.h"
#include "genQuery.h"

namespace irods {

    const int MAX_ERROR_MESSAGES = 100;

/// =-=-=-=-=-=-=-
/// @brief local function to replicate a new copy for
///        proc_results_for_rebalance
    static
    error repl_for_rebalance(
        rsComm_t*          _comm,
        const std::string& _obj_path,
        const std::string& _current_resc,
        const std::string& _src_hier,
        const std::string& _dst_hier,
        const std::string& _src_resc,
        const std::string& _dst_resc,
        int                _mode ) {
        // =-=-=-=-=-=-=-
        // generate a resource hierarchy that ends at this resource for pdmo
        hierarchy_parser parser;
        parser.set_string( _src_hier );
        std::string sub_hier;
        parser.str( sub_hier, _current_resc );

        // =-=-=-=-=-=-=-
        // create a data obj input struct to call rsDataObjRepl which given
        // the _stage_sync_kw will either stage or sync the data object
        dataObjInp_t data_obj_inp;
        bzero( &data_obj_inp, sizeof( data_obj_inp ) );
        rstrcpy( data_obj_inp.objPath, _obj_path.c_str(), MAX_NAME_LEN );
        data_obj_inp.createMode = _mode;
        addKeyVal( &data_obj_inp.condInput, RESC_HIER_STR_KW,      _src_hier.c_str() );
        addKeyVal( &data_obj_inp.condInput, DEST_RESC_HIER_STR_KW, _dst_hier.c_str() );
        addKeyVal( &data_obj_inp.condInput, RESC_NAME_KW,          _src_resc.c_str() );
        addKeyVal( &data_obj_inp.condInput, DEST_RESC_NAME_KW,     _dst_resc.c_str() );
        addKeyVal( &data_obj_inp.condInput, IN_PDMO_KW,             sub_hier.c_str() );
        addKeyVal( &data_obj_inp.condInput, ADMIN_KW,              "" );

        // =-=-=-=-=-=-=-
        // process the actual call for replication
        transferStat_t* trans_stat = NULL;
        int repl_stat = rsDataObjRepl( _comm, &data_obj_inp, &trans_stat );
        free( trans_stat );
        if ( repl_stat < 0 ) {
            std::stringstream msg;
            msg << "Failed to replicate the data object ["
                << _obj_path
                << "]";
            return ERROR( repl_stat, msg.str() );
        }

        return SUCCESS();

    } // repl_for_rebalance

/// =-=-=-=-=-=-=-
/// @brief
    error gather_dirty_replicas_for_child(
        rsComm_t*                         _comm,
        const std::vector<leaf_bundle_t>& _bundles,
        const int                         _limit,
        dist_child_result_t&              _results ) {
        // =-=-=-=-=-=-=-
        // trap bad input
        if ( !_comm ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "null comm pointer" );
        }
        else if ( _bundles.empty() ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "leaf bundle is empty" );
        }
        else if ( _limit <= 0 ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "limit is less than or equal to zero" );
        }

        // =-=-=-=-=-=-=-
        // build a general query
        genQueryOut_t* gen_out = 0;
        genQueryInp_t  gen_inp;
        memset( &gen_inp, 0, sizeof( gen_inp ) );

        gen_inp.maxRows = _limit;

        // =-=-=-=-=-=-=-
        // add a condition string for every leaf in the bundle
        std::stringstream cond_ss;
        for( auto bun : _bundles ) {
            for( auto id : bun ) {
                cond_ss << "= '" << id << "' || ";
            }
        }
        std::string cond_str = cond_ss.str();
        cond_str = cond_str.substr(0,cond_str.size()-4);

        addInxVal(
            &gen_inp.sqlCondInp,
            COL_D_RESC_ID,
            cond_str.c_str() );

        // =-=-=-=-=-=-=-
        // add condition string stating dirty status only
        addInxVal( &gen_inp.sqlCondInp,
                   COL_D_REPL_STATUS,
                   "= '0'" );

        // =-=-=-=-=-=-=-
        // request the data id as output
        addInxIval( &gen_inp.selectInp,
                    COL_D_DATA_ID, 1 );

        // =-=-=-=-=-=-=-
        // execute the query
        int status = rsGenQuery( _comm, &gen_inp, &gen_out );
        clearGenQueryInp( &gen_inp );
        if ( CAT_NO_ROWS_FOUND == status ) {
            // =-=-=-=-=-=-=-
            // hopefully there are no dirty data objects
            // and this is the typical code path
			freeGenQueryOut( &gen_out );
            return SUCCESS();

        }
        else if ( status < 0 || 0 == gen_out ) {
			freeGenQueryOut( &gen_out );
            return ERROR( status, "genQuery failed." );

        }

        // =-=-=-=-=-=-=-
        // extract result
        sqlResult_t* data_id_results = getSqlResultByInx( gen_out, COL_D_DATA_ID );
        if ( !data_id_results ) {
			freeGenQueryOut( &gen_out );
            return ERROR(
                       SYS_INTERNAL_NULL_INPUT_ERR,
                       "null data id result" );
        }

        // =-=-=-=-=-=-=-
        // iterate over the rows and capture the data ids
        for ( int i = 0; i < gen_out->rowCnt; i++ ) {
            // =-=-=-=-=-=-=-
            // get its value
            char* data_id_ptr = &data_id_results->value[ data_id_results->len * i ];
            int data_id = atoi( data_id_ptr );

            // =-=-=-=-=-=-=-
            // capture the result
            _results.push_back( data_id );

        } // for i

        freeGenQueryOut( &gen_out );

        return SUCCESS();

    } // gather_dirty_replicas_for_child

/// =-=-=-=-=-=-=-
/// @brief
    error get_source_data_object_attributes(
        const int            _data_id,
        const std::vector<leaf_bundle_t>& _bundles,
        rsComm_t*            _comm,
        std::string&         _obj_path,
        std::string&         _resc_hier,
        int&                 _data_mode ) {
        // =-=-=-=-=-=-=-
        // param check
        if ( _data_id <= 0 ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "invalid data id" );
        }
        else if ( !_comm ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "null comm pointer" );
        }

        // =-=-=-=-=-=-=-
        // gen query objects
        genQueryOut_t* gen_out = 0;
        genQueryInp_t  gen_inp;
        memset( &gen_inp, 0, sizeof( genQueryInp_t ) );

        gen_inp.maxRows = MAX_SQL_ROWS;

        // =-=-=-=-=-=-=-
        // add condition string matching data id
        std::stringstream id_str;
        id_str << _data_id;
        std::string cond_str = "='" + id_str.str() + "'";
        addInxVal( &gen_inp.sqlCondInp,
                   COL_D_DATA_ID,
                   cond_str.c_str() );

        // =-=-=-=-=-=-=-
        // add condition string matching leaf ids
        std::stringstream cond_ss;
        for( auto bun : _bundles ) {
            for( auto id : bun ) {
                cond_ss << "='" << id << "' || ";
            }
        }
        cond_str = cond_ss.str();
        cond_str = cond_str.substr(0,cond_str.size()-4);
        addInxVal( &gen_inp.sqlCondInp,
                   COL_D_RESC_ID,
                   cond_str.c_str() );

        // =-=-=-=-=-=-=-
        // add condition string stating clean status only
        addInxVal( &gen_inp.sqlCondInp,
                   COL_D_REPL_STATUS,
                   "= '1'" );

        // =-=-=-=-=-=-=-
        // request the data name, coll name, resc hier, mode
        addInxIval( &gen_inp.selectInp,
                    COL_DATA_NAME, 1 );
        addInxIval( &gen_inp.selectInp,
                    COL_COLL_NAME, 1 );
        addInxIval( &gen_inp.selectInp,
                    COL_DATA_MODE, 1 );
        addInxIval( &gen_inp.selectInp,
                    COL_D_RESC_ID, 1 );

        // =-=-=-=-=-=-=-
        // execute the query
        int status = rsGenQuery( _comm, &gen_inp, &gen_out );
        clearGenQueryInp( &gen_inp );
        if ( status < 0 || 0 == gen_out ) {
			freeGenQueryOut( &gen_out );
            return ERROR(
                       status,
                       "genQuery failed." );
        }

        // =-=-=-=-=-=-=-
        // extract result
        sqlResult_t* data_name_result = getSqlResultByInx( gen_out, COL_DATA_NAME );
        if ( !data_name_result ) {
			freeGenQueryOut( &gen_out );
            return ERROR(
                       UNMATCHED_KEY_OR_INDEX,
                       "null data_name sql result" );
        }

        // =-=-=-=-=-=-=-
        // get its value
        char* data_name_ptr = &data_name_result->value[ 0 ];
        if ( !data_name_ptr ) {
			freeGenQueryOut( &gen_out );
            return ERROR(
                       SYS_INTERNAL_NULL_INPUT_ERR,
                       "null data_name_ptr result" );
        }
        std::string data_name = data_name_ptr;

        // =-=-=-=-=-=-=-
        // extract result
        sqlResult_t* coll_name_result = getSqlResultByInx( gen_out, COL_COLL_NAME );
        if ( !coll_name_result ) {
			freeGenQueryOut( &gen_out );
            return ERROR(
                       UNMATCHED_KEY_OR_INDEX,
                       "null coll_name sql result" );
        }

        // =-=-=-=-=-=-=-
        // get its value
        char* coll_name_ptr = &coll_name_result->value[ 0 ];
        if ( !coll_name_ptr ) {
			freeGenQueryOut( &gen_out );
            return ERROR(
                       SYS_INTERNAL_NULL_INPUT_ERR,
                       "null coll_name_ptr result" );
        }
        std::string coll_name = coll_name_ptr;

        // =-=-=-=-=-=-=-
        // extract result
        sqlResult_t* resc_id_result = getSqlResultByInx( gen_out, COL_D_RESC_ID );
        if ( !resc_id_result ) {
			freeGenQueryOut( &gen_out );
            return ERROR(
                       UNMATCHED_KEY_OR_INDEX,
                       "null resc_id sql result" );
        }

        // =-=-=-=-=-=-=-
        // get its value
        char* resc_id_ptr = &resc_id_result->value[ 0 ];
        if ( !resc_id_ptr ) {
			freeGenQueryOut( &gen_out );
            return ERROR(
                       SYS_INTERNAL_NULL_INPUT_ERR,
                       "null resc_id_ptr result" );
        }

        // =-=-=-=-=-=-=-
        // extract result
        sqlResult_t* data_mode_result = getSqlResultByInx( gen_out, COL_DATA_MODE );
        if ( !data_mode_result ) {
			freeGenQueryOut( &gen_out );
            return ERROR(
                       UNMATCHED_KEY_OR_INDEX,
                       "null data_mode sql result" );
        }

        // =-=-=-=-=-=-=-
        // get its value
        char* data_mode_ptr = &data_mode_result->value[ 0 ];
        if ( !data_mode_ptr ) {
			freeGenQueryOut( &gen_out );
            return ERROR(
                       SYS_INTERNAL_NULL_INPUT_ERR,
                       "null data_mode_ptr result" );
        }

        // =-=-=-=-=-=-=-
        // set the out variables
        _obj_path  = coll_name                    +
                     get_virtual_path_separator() +
                     data_name;
        _data_mode = atoi( data_mode_ptr );

        irods::error ret = resc_mgr.leaf_id_to_hier( strtoll(resc_id_ptr,0,0), _resc_hier );
        if(!ret.ok()) {
            return PASS(ret);
        }

        freeGenQueryOut( &gen_out );
        return SUCCESS();

    } // get_source_data_object_attributes

/// =-=-=-=-=-=-=-
/// @brief high level function to gather all of the data objects
///        which need rereplicated
    error gather_data_objects_for_rebalance(
        rsComm_t*                   _comm,
        const int                   _limit,
        size_t                      _child_idx,
        std::vector<leaf_bundle_t>& _leaf_bundles,
        dist_child_result_t& _results ) {
        // =-=-=-=-=-=-=-
        // clear incoming result set
        _results.clear();

        // =-=-=-=-=-=-=-
        // check for dirty replicas first
        error ret = gather_dirty_replicas_for_child(
                        _comm,
                        _leaf_bundles,
                        _limit,
                        _results );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // compute remaining limit as the difference between the
        // original limit and the number of results;
        int mod_limit = _limit - _results.size();

        // =-=-=-=-=-=-=-
        // query for remaining items which need re-replicated to this child
        if( mod_limit > 0 ) {
            int query_status = chlGetReplListForLeafBundles(
                                   mod_limit,
                                   _child_idx,
                                   &_leaf_bundles,
                                   &_results );
            if ( CAT_NO_ROWS_FOUND != query_status ) {
                return ERROR(
                           query_status,
                           "chlGetReplListForLeafBundles failed." );
            }
        }

        return SUCCESS();

    } // gather_data_objects_for_rebalance

    error filter_data_id_for_repl_to_bundle(
        rsComm_t*            _comm,
        const rodsLong_t     _data_id,
        const leaf_bundle_t& _bundle,
        bool&                _repl_flg ) {
        _repl_flg = false;

        // =-=-=-=-=-=-=-
        // build a general query
        genQueryOut_t* gen_out = 0;
        genQueryInp_t  gen_inp;
        memset( &gen_inp, 0, sizeof( gen_inp ) );

        // =-=-=-=-=-=-=-
        // request the resource id as output
        addInxIval( &gen_inp.selectInp,
                    COL_D_RESC_ID, 1 );

        // =-=-=-=-=-=-=-
        // set the data id as the condition
        std::stringstream cond_ss;
        cond_ss << "= " << _data_id;
        addInxVal(
            &gen_inp.sqlCondInp,
            COL_D_DATA_ID,
            cond_ss.str().c_str() );

        // =-=-=-=-=-=-=-
        // execute the query
        int status = rsGenQuery( _comm, &gen_inp, &gen_out );
        clearGenQueryInp( &gen_inp );
        if ( CAT_NO_ROWS_FOUND == status ) {
            // =-=-=-=-=-=-=-
            // hopefully there are no dirty data objects
            // and this is the typical code path
            freeGenQueryOut( &gen_out );
            return SUCCESS();

        }
        else if ( status < 0 || 0 == gen_out ) {
            freeGenQueryOut( &gen_out );
            return ERROR( status, "genQuery failed." );

        }

        // =-=-=-=-=-=-=-
        // extract result
        sqlResult_t* resc_id_results = getSqlResultByInx( gen_out, COL_D_RESC_ID );
        if ( !resc_id_results ) {
            freeGenQueryOut( &gen_out );
            return ERROR(
                       SYS_INTERNAL_NULL_INPUT_ERR,
                       "null resc id result" );
        }

        // =-=-=-=-=-=-=-
        // iterate over the rows and capture the data ids
        for ( int i = 0; i < gen_out->rowCnt; i++ ) {
            // =-=-=-=-=-=-=-
            // get its value
            char* resc_id_ptr = &resc_id_results->value[ resc_id_results->len * i ];
            int resc_id = atoi( resc_id_ptr );

            // =-=-=-=-=-=-=-
            // filter the result against the leaf bundle
            auto res = std::find(
                           std::begin(_bundle),
                           std::end(_bundle),
                           resc_id );
            // we found a match in the leaf bundle so we do
            // not require a replication to this child
            if(std::end(_bundle) != res) {
                _repl_flg = false;
                break;
            }
        } // for i

        freeGenQueryOut( &gen_out );


        return SUCCESS();

    } // filter_data_id_for_repl_to_bundle

/// =-=-=-=-=-=-=-
/// @brief high level function which process a result set from
///        the above gathering function for a rebalancing operation
    error proc_results_for_rebalance(
        rsComm_t*                        _comm,
        const std::string&               _parent_resc_name,
        const std::string&               _child_resc_name,
        const size_t                     _bun_idx,
        const std::vector<leaf_bundle_t> _bundles,
        const dist_child_result_t&       _results ) {

        // =-=-=-=-=-=-=-
        // check incoming params
        if ( !_comm ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "null comm pointer" );
        }
        else if ( _results.empty() ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "empty results vector" );
        }

        irods::error final_err = SUCCESS();

        // =-=-=-=-=-=-=-
        // iterate over the result set and repl the objects
        dist_child_result_t::const_iterator r_itr = _results.begin();
        for ( ; r_itr != _results.end(); ++r_itr ) {
            // =-=-=-=-=-=-=-
            // get a valid source object from which to replicate
            int src_mode = 0;
            std::string obj_path, src_hier;
            irods::error ret = get_source_data_object_attributes(
                                   *r_itr,
                                   _bundles,
                                   _comm,
                                   obj_path,
                                   src_hier,
                                   src_mode );
            if ( !ret.ok() ) {
                return PASS( ret );
            }

            // =-=-=-=-=-=-=-
            // create a file object so we can resolve a valid
            // hierarchy to which to replicate
            file_object_ptr f_ptr( new file_object(
                                       _comm,
                                       obj_path,
                                       "",
                                       "",
                                       0,
                                       src_mode,
                                       0 ) );
            // =-=-=-=-=-=-=-
            // short circuit the magic re-repl
            hierarchy_parser sub_parser;
            sub_parser.set_string( src_hier );
            std::string sub_hier;
            sub_parser.str( sub_hier, _parent_resc_name );
            f_ptr->in_pdmo( sub_hier );

            // =-=-=-=-=-=-=-
            // init the parser with the fragment of the
            // upstream hierarchy not including the repl
            // node as it should add itself
            hierarchy_parser parser;
            size_t pos = src_hier.find( _parent_resc_name );
            if ( std::string::npos == pos ) {
                std::stringstream msg;
                msg << "missing repl name ["
                    << _parent_resc_name
                    << "] in source hier string ["
                    << src_hier
                    << "]";
                return ERROR(
                           SYS_INVALID_INPUT_PARAM,
                           msg.str() );
            }

            // =-=-=-=-=-=-=-
            // substring hier from the root to the parent resc
            std::string src_frag = src_hier.substr(
                                       0, pos + _parent_resc_name.size() + 1 );
            parser.set_string( src_frag );

            // =-=-=-=-=-=-=-
            // handy reference to root resc name
            std::string root_resc;
            parser.first_resc( root_resc );

            // =-=-=-=-=-=-=-
            // resolve the target child resource plugin
            resource_ptr dst_resc;
            error r_err = resc_mgr.resolve(
                              _child_resc_name,
                              dst_resc );
            if ( !r_err.ok() ) {
                return PASS( r_err );
            }

            // =-=-=-=-=-=-=-
            // then we need to query the target resource and ask
            // it to determine a dest resc hier for the repl
            std::string host_name;
            float            vote = 0.0;
            r_err = dst_resc->call < const std::string*,
            const std::string*,
            hierarchy_parser*,
            float* > (
                _comm,
                RESOURCE_OP_RESOLVE_RESC_HIER,
                f_ptr,
                &CREATE_OPERATION,
                &host_name,
                &parser,
                &vote );
            if ( !r_err.ok() ) {
                return PASS( r_err );
            }

            // =-=-=-=-=-=-=-
            // extract the hier from the parser
            std::string dst_hier;
            parser.str( dst_hier );

            // =-=-=-=-=-=-=-
            // now that we have all the pieces in place, actually
            // do the replication
            r_err = repl_for_rebalance(
                        _comm,
                        obj_path,
                        _parent_resc_name,
                        src_hier,
                        dst_hier,
                        root_resc,
                        root_resc,
                        src_mode );
            if ( !r_err.ok() ) {
                final_err =  PASS( r_err );
                irods::log(final_err);
                if( _comm->rError.len < MAX_ERROR_MESSAGES ) {
                    addRErrorMsg(
                        &_comm->rError,
                        final_err.code(),
                        final_err.result().c_str());
                }
            }

        } // for r_itr

        return final_err;

    } // proc_results_for_rebalance

}; // namespace irods
