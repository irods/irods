


// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_repl_rebalance.h"
#include "eirods_resource_plugin.h"
#include "eirods_file_object.h"
#include "eirods_hierarchy_parser.h"
#include "eirods_resource_redirect.h"

// =-=-=-=-=-=-=-
// irods includes
#include "dataObjRepl.h"

namespace eirods {
    /// =-=-=-=-=-=-=-
    /// @brief local function to replicate a new copy for 
    ///        proc_result_for_rebalance
    static 
    error repl_for_rebalance (
        rsComm_t*          _comm,
        const std::string& _obj_path,
        const std::string& _src_hier,
        const std::string& _dst_hier,
        const std::string& _src_resc,
        const std::string& _dst_resc,
        int                _mode ) {
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
        addKeyVal( &data_obj_inp.condInput, IN_PDMO_KW,            "" );

        // =-=-=-=-=-=-=-
        // process the actual call for replication
        transferStat_t* trans_stat = NULL;
        int repl_stat = rsDataObjRepl( _comm, &data_obj_inp, &trans_stat );
        if( repl_stat < 0 ) {
            char* sys_error;
            char* rods_error = rodsErrorName( repl_stat, &sys_error );
            std::stringstream msg;
            msg << "Failed to replicate the data object [" 
                << _obj_path
                << "]";
            return ERROR( repl_stat, msg.str() );
        }

        return SUCCESS();

    } // repl_for_rebalance

    /// =-=-=-=-=-=-=-
    /// @brief local function to process a result set from the rebalancing 
    ///        operation
    error proc_result_for_rebalance(
        rsComm_t*                         _comm,
        const std::string&                _obj_path,
        const std::string&                _this_resc_name,
        const std::vector< std::string >& _children,
        const repl_obj_result_t&          _results ) {
        // =-=-=-=-=-=-=-
        // check incoming params
        if( !_comm ) {
            return ERROR( 
                       SYS_INVALID_INPUT_PARAM,
                       "null comm pointer" );
        } else if( _obj_path.empty() ) {
            return ERROR( 
                       SYS_INVALID_INPUT_PARAM,
                       "empty obj path" );
        } else if( _this_resc_name.empty() ) {
            return ERROR( 
                       SYS_INVALID_INPUT_PARAM,
                       "empty this resc name" );
        } else if( _children.empty() ) {
            return ERROR( 
                       SYS_INVALID_INPUT_PARAM,
                       "empty child vector" );
        } else if( _results.empty() ) {
            return ERROR( 
                       SYS_INVALID_INPUT_PARAM,
                       "empty results vector" );
        }

        // =-=-=-=-=-=-=-
        // given this entry in the results map
        // compare the resc hiers to the full list of
        // children.  repl any that are missing
        vector< std::string >::const_iterator c_itr = _children.begin();
        for( ; c_itr != _children.end(); ++c_itr ) {
            // =-=-=-=-=-=-=-=-
            // look for child in hier strs for this data obj
            bool found = false;
            repl_obj_result_t::const_iterator o_itr = _results.begin();
            for( ; o_itr != _results.end(); ++o_itr ) {
                if( std::string::npos != o_itr->first.find( *c_itr ) ) {
                    //=-=-=-=-=-=-=-
                    // set found flag and break
                    found = true;
                    break;
                }
            }

            // =-=-=-=-=-=-=-=-
            // if its not found we repl to it
            if( !found ) {
                // =-=-=-=-=-=-=-
                // create an fco in order to call the resolve operation
                int dst_mode = _results.begin()->second;
                file_object_ptr f_ptr( new file_object( 
                                           _comm,
                                           _obj_path,
                                           "",
                                           "",
                                           0,
                                           dst_mode,
                                           0 ) );
                // =-=-=-=-=-=-=-
                // short circuit the magic re-repl
                f_ptr->in_pdmo( true );

                // =-=-=-=-=-=-=-
                // pick a source hier from the existing list.  this could
                // perhaps be done heuristically - optimize later
                const std::string& src_hier = _results.begin()->first;
                
                // =-=-=-=-=-=-=-
                // init the parser with the fragment of the 
                // upstream hierarchy not including the repl 
                // node as it should add itself
                hierarchy_parser parser;
                size_t pos = src_hier.find( _this_resc_name );
                if( std::string::npos == pos ) {
                    std::stringstream msg;
                    msg << "missing repl name ["
                        << _this_resc_name 
                        << "] in source hier string ["
                        << src_hier 
                        << "]";
                    return ERROR( 
                               SYS_INVALID_INPUT_PARAM, 
                               msg.str() );
                }

                std::string src_frag = src_hier.substr( 0, pos+_this_resc_name.size()+1 );
                parser.set_string( src_frag );

                // =-=-=-=-=-=-=-
                // handy reference to root resc name
                std::string root_resc;
                parser.first_resc( root_resc );

                // =-=-=-=-=-=-=-
                // resolve the target child resource plugin
                resource_ptr dst_resc;
                error r_err = resc_mgr.resolve( 
                                          *c_itr, 
                                          dst_resc );
                if( !r_err.ok() ) {
                    return PASS( r_err );
                }

                // =-=-=-=-=-=-=-
                // then we need to query the target resource and ask
                // it to determine a dest resc hier for the repl
                std::string host_name;
                float            vote = 0.0;
                r_err = dst_resc->call< const std::string*, 
                                        const std::string*, 
                                        hierarchy_parser*, 
                                        float* >(
                            _comm, 
                            RESOURCE_OP_RESOLVE_RESC_HIER, 
                            f_ptr, 
                            &EIRODS_CREATE_OPERATION,
                            &host_name, 
                            &parser, 
                            &vote );
                if( !r_err.ok() ) {
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
                            _obj_path,
                            src_hier,
                            dst_hier,
                            root_resc,
                            root_resc,
                            dst_mode );
                if( !r_err.ok() ) {
                    return PASS( r_err );
                }

                //=-=-=-=-=-=-=-
                // reset found flag so things keep working
                found = false; 

            } // if !found

        } // for c_itr

        return SUCCESS();

    } // proc_result_for_rebalance

}; // namespace eirods



