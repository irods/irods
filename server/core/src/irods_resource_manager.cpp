// =-=-=-=-=-=-=-
#include "rcMisc.h"
#include "irods_resource_manager.hpp"
#include "irods_log.hpp"
#include "irods_string_tokenize.hpp"
#include "irods_stacktrace.hpp"
#include "irods_resource_plugin_impostor.hpp"
#include "irods_load_plugin.hpp"
#include "irods_lexical_cast.hpp"
#include "rsGenQuery.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "getRescQuota.h"
#include "irods_children_parser.hpp"
#include "irods_hierarchy_parser.hpp"
#include "rsGlobalExtern.hpp"
#include "generalAdmin.h"
#include "phyBundleColl.h"
#include "miscServerFunct.hpp"
#include "genQuery.h"

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>
#include <vector>
#include <iterator>

// =-=-=-=-=-=-=-
// global singleton
irods::resource_manager resc_mgr;

namespace irods {
// =-=-=-=-=-=-=-
// public - Constructor
    resource_manager::resource_manager() {
    } // ctor

// =-=-=-=-=-=-=-
// public - Copy Constructor
    resource_manager::resource_manager( const resource_manager& ) {
    } // cctor

// =-=-=-=-=-=-=-
// public - Destructor
    resource_manager::~resource_manager( ) {
    } // cctor

// =-=-=-=-=-=-=-
// public - retrieve a resource given its key
    error resource_manager::resolve(
        std::string   _key,
        resource_ptr& _value ) {

        if ( _key.empty() ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "empty key" );
        }

        if ( resource_name_map_.has_entry( _key ) ) {
            _value = resource_name_map_[ _key ];
            return SUCCESS();

        }
        else {
            std::stringstream msg;
            msg << "no resource found for name ["
                << _key << "]";
            return ERROR( SYS_RESC_DOES_NOT_EXIST, msg.str() );

        }

    } // resolve

// =-=-=-=-=-=-=-
// public - retrieve a resource given its key
    error resource_manager::resolve(
        rodsLong_t    _resc_id,
        resource_ptr& _value ) {
        if ( resource_id_map_.has_entry( _resc_id ) ) {
            _value = resource_id_map_[ _resc_id ];
            return SUCCESS();

        }
        else {
            std::stringstream msg;
            msg << "no resource found for name ["
                << _resc_id << "]";
            return ERROR( SYS_RESC_DOES_NOT_EXIST, msg.str() );

        }

    } // resolve

    error resource_manager::load_resource_plugin(
        resource_ptr&     _plugin,
        const std::string _plugin_name,
        const std::string _inst_name,
        const std::string _context ) {
        resource* resc = 0;
        error ret = load_plugin< resource >(
                        resc,
                        _plugin_name,
                        PLUGIN_TYPE_RESOURCE,
                        _inst_name,
                        _context );
        if ( ret.ok() && resc ) {
            _plugin.reset( resc );
        }
        else {
            if ( ret.code() == PLUGIN_ERROR_MISSING_SHARED_OBJECT ) {
                rodsLog(
                    LOG_DEBUG,
                    "loading impostor resource for [%s] of type [%s] with context [%s] and load_plugin message [%s]",
                    _inst_name.c_str(),
                    _plugin_name.c_str(),
                    _context.c_str(),
                    ret.result().c_str());
                _plugin.reset(
                    new impostor_resource(
                        "impostor_resource", "" ) );
            } else {
                return PASS( ret );
            }
        }

        return SUCCESS();

    } // load_resource_plugin

// =-=-=-=-=-=-=-
// public - retrieve a resource given a vault path
    error resource_manager::validate_vault_path(
        std::string       _physical_path,
        rodsServerHost_t* _svr_host,
        std::string&      _out_path ) {
        // =-=-=-=-=-=-=-
        // simple flag to state a resource matching the prop and value is found
        bool found = false;

        // =-=-=-=-=-=-=-
        // quick check on the resource table
        if ( resource_name_map_.empty() ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "empty resource table" );
        }

        // =-=-=-=-=-=-=-
        // quick check on the path that it has something in it
        if ( _physical_path.empty() ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "empty property" );
        }

        // =-=-=-=-=-=-=-
        // iterate through the map and search for our path
        lookup_table< resource_ptr >::iterator itr = resource_name_map_.begin();
        for ( ; !found && itr != resource_name_map_.end(); ++itr ) {
            // =-=-=-=-=-=-=-
            // get the host pointer from the resource
            rodsServerHost_t* svr_host = 0;
            error ret = itr->second->get_property< rodsServerHost_t* >( RESOURCE_HOST, svr_host );
            if ( !ret.ok() ) {
                PASS( ret );
            }

            // =-=-=-=-=-=-=-
            // if this host matches the incoming host pointer then were good
            // otherwise continue searching
            if ( svr_host != _svr_host ) {
                continue;
            }

            // =-=-=-=-=-=-=-
            // query resource for the property value
            std::string path;
            ret = itr->second->get_property<std::string>( RESOURCE_PATH, path );

            // =-=-=-=-=-=-=-
            // if we get a good parameter and do not match non-storage nodes with an empty physical path
            if ( ret.ok() ) {
                // =-=-=-=-=-=-=-
                // compare incoming value and stored value
                // one may be a subset of the other so compare both ways
                if ( !path.empty() && ( _physical_path.find( path ) != std::string::npos ) ) {
                    found     = true;
                    _out_path = path;
                }

            }
            else {
                std::stringstream msg;
                msg << "resource_manager::resolve_from_physical_path - ";
                msg << "failed to get vault parameter from resource";
                msg << ret.code();
                irods::log( PASSMSG( msg.str(), ret ) );

            }

        } // for itr

        // =-=-=-=-=-=-=-
        // did we find a resource and is the ptr valid?
        if ( true == found ) {
            return SUCCESS();
        }
        else {
            std::stringstream msg;
            msg << "failed to find resource for path [";
            msg << _physical_path;
            msg << "]";
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
        }

    } // validate_vault_path

// =-=-=-=-=-=-=-
// public - connect to the catalog and query for all the
//          attached resources and instantiate them
    error resource_manager::init_from_catalog( rsComm_t* _comm ) {
        // =-=-=-=-=-=-=-
        // clear existing resource map and initialize
        resource_name_map_.clear();

        // =-=-=-=-=-=-=-
        // set up data structures for a gen query
        genQueryInp_t  genQueryInp;
        genQueryOut_t* genQueryOut = NULL;

        error proc_ret;

        memset( &genQueryInp, 0, sizeof( genQueryInp ) );

        addInxIval( &genQueryInp.selectInp, COL_R_RESC_ID,             1 );
        addInxIval( &genQueryInp.selectInp, COL_R_RESC_NAME,           1 );
        addInxIval( &genQueryInp.selectInp, COL_R_ZONE_NAME,           1 );
        addInxIval( &genQueryInp.selectInp, COL_R_TYPE_NAME,           1 );
        addInxIval( &genQueryInp.selectInp, COL_R_CLASS_NAME,          1 );
        addInxIval( &genQueryInp.selectInp, COL_R_LOC,                 1 );
        addInxIval( &genQueryInp.selectInp, COL_R_VAULT_PATH,          1 );
        addInxIval( &genQueryInp.selectInp, COL_R_FREE_SPACE,          1 );
        addInxIval( &genQueryInp.selectInp, COL_R_RESC_INFO,           1 );
        addInxIval( &genQueryInp.selectInp, COL_R_RESC_COMMENT,        1 );
        addInxIval( &genQueryInp.selectInp, COL_R_CREATE_TIME,         1 );
        addInxIval( &genQueryInp.selectInp, COL_R_MODIFY_TIME,         1 );
        addInxIval( &genQueryInp.selectInp, COL_R_RESC_STATUS,         1 );
        addInxIval( &genQueryInp.selectInp, COL_R_RESC_CHILDREN,       1 );
        addInxIval( &genQueryInp.selectInp, COL_R_RESC_CONTEXT,        1 );
        addInxIval( &genQueryInp.selectInp, COL_R_RESC_PARENT,         1 );
        addInxIval( &genQueryInp.selectInp, COL_R_RESC_PARENT_CONTEXT, 1 );

        genQueryInp.maxRows = MAX_SQL_ROWS;

        // =-=-=-=-=-=-=-
        // init continueInx to pass for first loop
        int continueInx = 1;

        // =-=-=-=-=-=-=-
        // loop until continuation is not requested
        while ( continueInx > 0 ) {
            // =-=-=-=-=-=-=-
            // perform the general query
            int status = rsGenQuery( _comm, &genQueryInp, &genQueryOut );

            // =-=-=-=-=-=-=-
            // perform the general query
            if ( status < 0 ) {
                freeGenQueryOut( &genQueryOut );
                clearGenQueryInp( &genQueryInp );
                if ( status != CAT_NO_ROWS_FOUND ) {
                    // actually an error
                    rodsLog( LOG_NOTICE, "initResc: rsGenQuery error, status = %d",
                             status );
                    return ERROR( status, "genQuery failed." );
                }

                break; // CAT_NO_ROWS_FOUND expected at the end of a query

            } // if

            // =-=-=-=-=-=-=-
            // given a series of rows, each being a resource, create a resource and add it to the table
            proc_ret = process_init_results( genQueryOut );

            // =-=-=-=-=-=-=-
            // if error is not valid, clear query and bail
            if ( !proc_ret.ok() ) {
                irods::error log_err = PASSMSG( "init_from_catalog - process_init_results failed", proc_ret );
                irods::log( log_err );
                freeGenQueryOut( &genQueryOut );
                break;
            }
            else {
                if ( genQueryOut != NULL ) {
                    continueInx = genQueryInp.continueInx = genQueryOut->continueInx;
                    freeGenQueryOut( &genQueryOut );
                }
                else {
                    continueInx = 0;
                }

            } // else

        } // while

        freeGenQueryOut( &genQueryOut );
        clearGenQueryInp( &genQueryInp );

        // =-=-=-=-=-=-=-
        // pass along the error if we are in an error state
        if ( !proc_ret.ok() ) {
            return PASSMSG( "process_init_results failed.", proc_ret );
        }

        // =-=-=-=-=-=-=-
        // Update child resource maps
        proc_ret = init_child_map();
        if ( !proc_ret.ok() ) {
            return PASSMSG( "init_child_map failed.", proc_ret );
        }

        // =-=-=-=-=-=-=-
        // gather the post disconnect maintenance operations
        error op_ret = gather_operations();
        if ( !op_ret.ok() ) {
            return PASSMSG( "gather_operations failed.", op_ret );
        }

        // =-=-=-=-=-=-=-
        // call start for plugins
        error start_err = start_resource_plugins();
        if ( !start_err.ok() ) {
            return PASSMSG( "start_resource_plugins failed.", start_err );
        }

        // =-=-=-=-=-=-=-
        // win!
        return SUCCESS();

    } // init_from_catalog

// =-=-=-=-=-=-=-
/// @brief call shutdown on resources before destruction
    error resource_manager::shut_down_resources( ) {
        // =-=-=-=-=-=-=-
        // iterate over all resources in the table
        lookup_table< boost::shared_ptr< resource > >::iterator itr;
        for ( itr =  resource_name_map_.begin();
                itr != resource_name_map_.end();
                ++itr ) {
            itr->second->stop_operation();

        } // for itr

        return SUCCESS();

    } // shut_down_resources

// =-=-=-=-=-=-=-
/// @brief create a list of resources who do not have parents ( roots )
    error resource_manager::get_root_resources(
        std::vector< std::string >& _list ) {
        // =-=-=-=-=-=-=-
        // iterate over all resources in the table
        lookup_table< boost::shared_ptr< resource > >::iterator itr;
        for ( itr =  resource_name_map_.begin();
                itr != resource_name_map_.end();
                ++itr ) {
            resource_ptr resc = itr->second;
            resource_ptr parent_ptr;
            error ret = itr->second->get_parent( parent_ptr );
            if ( !ret.ok() ) {
                std::string resc_name;
                ret = resc->get_property< std::string >( RESOURCE_NAME, resc_name );
                if ( !ret.ok() ) {
                    return PASS( ret );
                }

                _list.push_back( resc_name );

            }

        } // for itr

        return SUCCESS();

    } // get_root_resources

    error resource_manager::get_parent_name(
        resource_ptr _resc,
        std::string& _name ) {

        std::string my_name;
        error ret = _resc->get_property<std::string>(
                        RESOURCE_NAME,
                        my_name);
        if(!ret.ok()) {
            return PASS(ret);
        }

        std::string parent_id_str;
        ret = _resc->get_property<std::string>(
                        RESOURCE_PARENT,
                        parent_id_str);
        if(!ret.ok()) {
            return PASS(ret);
        }

        if(parent_id_str.empty()) {
            std::stringstream msg;
            return ERROR(
                    HIERARCHY_ERROR,
                    "empty parent string");
        }

        rodsLong_t parent_id;
        ret = lexical_cast<rodsLong_t>(
                  parent_id_str,
                  parent_id);
        if(!ret.ok()) {
            return PASS(ret);
        }

        resource_ptr parent_resc;
        ret = resolve(
                parent_id,
                parent_resc);
        if(!ret.ok()) {
            return PASS(ret);
        }

        return parent_resc->get_property<std::string>(
                   RESOURCE_NAME,
                   _name );
    } // get_parent_name

    error resource_manager::get_hier_to_root_for_resc(
        const std::string& _resc_name,
        std::string&       _hierarchy ) {

        _hierarchy = _resc_name;
        std::string parent_name = _resc_name;

        resource_ptr resc;
        while( !parent_name.empty() ) {
            error ret = resolve(
                            parent_name,
                            resc);
            if(!ret.ok()) {
                return PASS(ret);
            }

            ret = get_parent_name(
                      resc,
                      parent_name );
            if(!ret.ok()) {
                if(HIERARCHY_ERROR == ret.code()) {
                    break;
                }

                return PASS(ret);
            }

            if(!parent_name.empty()) {
                _hierarchy = parent_name +
                             irods::hierarchy_parser::delimiter() +
                             _hierarchy;
            }
        } // while

        return SUCCESS();

    } // get_hier_to_root_for_resc

    error resource_manager::gather_leaf_bundle_for_child(
        const std::string& _resc_name,
        leaf_bundle_t&     _bundle ) {
        resource_ptr resc;
        error ret = resolve(
                        _resc_name,
                        resc);
        if(!ret.ok()) {
            return PASS(ret);
        }

        if(resc->num_children() > 0 ) {
            // still more children to traverse
            std::vector<std::string> children;
            resc->children(children);

            for( size_t idx = 0;
                 idx < children.size();
                 ++idx ) {

                ret = gather_leaf_bundle_for_child(
                          children[idx],
                          _bundle );
                if(!ret.ok() ) {
                    return PASS(ret);
                }

            } // for idx
        }
        else {
            // we have found a leaf
            rodsLong_t resc_id;
            ret = resc->get_property<rodsLong_t>(
                    RESOURCE_ID,
                    resc_id);
            if(!ret.ok()) {
                return PASS(ret);
            }

            _bundle.push_back( resc_id );
        }

        return SUCCESS();

    } // gather_leaf_bundle_for_child

    error resource_manager::gather_leaf_bundles_for_resc(
        const std::string&          _resc_name,
        std::vector<leaf_bundle_t>& _bundles ) {
        resource_ptr resc;
        error ret = resolve(
                        _resc_name,
                        resc);
        if(!ret.ok()) {
            return PASS(ret);
        }

        std::vector<std::string> children;
        resc->children(children);

        _bundles.resize(children.size());

        for( size_t idx = 0;
             idx < children.size();
             ++idx ) {
            ret = gather_leaf_bundle_for_child(
                      children[idx],
                      _bundles[idx] );
            if(!ret.ok() ) {
                return PASS(ret);
            }
            // sort for increase in search speed
            // for rebalance operation
            std::sort(
                std::begin(_bundles[idx]),
                std::end(_bundles[idx]) );

        } // for idx

        return SUCCESS();

    } // gather_leaf_bundles_for_resc

// =-=-=-=-=-=-=-
// public - take results from genQuery, extract values and create resources
    error resource_manager::process_init_results( genQueryOut_t* _result ) {
        // =-=-=-=-=-=-=-
        // extract results from query
        if ( !_result ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "_result parameter is null" );
        }

        // =-=-=-=-=-=-=-
        // values to extract from query
        sqlResult_t *rescId       = 0, *rescName      = 0, *zoneName   = 0, *rescType   = 0, *rescClass = 0;
        sqlResult_t *rescLoc      = 0, *rescVaultPath = 0, *freeSpace  = 0, *rescInfo   = 0;
        sqlResult_t *rescComments = 0, *rescCreate    = 0, *rescModify = 0, *rescStatus = 0;
        sqlResult_t *rescChildren = 0, *rescContext   = 0, *rescParent = 0, *rescParentContext = 0;

        // =-=-=-=-=-=-=-
        // extract results from query
        if ( ( rescId = getSqlResultByInx( _result, COL_R_RESC_ID ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "getSqlResultByInx for COL_R_RESC_ID failed" );
        }

        if ( ( rescName = getSqlResultByInx( _result, COL_R_RESC_NAME ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "getSqlResultByInx for COL_R_RESC_NAME failed" );
        }

        if ( ( zoneName = getSqlResultByInx( _result, COL_R_ZONE_NAME ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "getSqlResultByInx for COL_R_ZONE_NAME failed" );
        }

        if ( ( rescType = getSqlResultByInx( _result, COL_R_TYPE_NAME ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "getSqlResultByInx for COL_R_TYPE_NAME failed" );
        }

        if ( ( rescClass = getSqlResultByInx( _result, COL_R_CLASS_NAME ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "getSqlResultByInx for COL_R_CLASS_NAME failed" );
        }

        if ( ( rescLoc = getSqlResultByInx( _result, COL_R_LOC ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "getSqlResultByInx for COL_R_LOC failed" );
        }

        if ( ( rescVaultPath = getSqlResultByInx( _result, COL_R_VAULT_PATH ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "getSqlResultByInx for COL_R_VAULT_PATH failed" );
        }

        if ( ( freeSpace = getSqlResultByInx( _result, COL_R_FREE_SPACE ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "getSqlResultByInx for COL_R_FREE_SPACE failed" );
        }

        if ( ( rescInfo = getSqlResultByInx( _result, COL_R_RESC_INFO ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "getSqlResultByInx for COL_R_RESC_INFO failed" );
        }

        if ( ( rescComments = getSqlResultByInx( _result, COL_R_RESC_COMMENT ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "getSqlResultByInx for COL_R_RESC_COMMENT failed" );
        }

        if ( ( rescCreate = getSqlResultByInx( _result, COL_R_CREATE_TIME ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "getSqlResultByInx for COL_R_CREATE_TIME failed" );
        }

        if ( ( rescModify = getSqlResultByInx( _result, COL_R_MODIFY_TIME ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "getSqlResultByInx for COL_R_MODIFY_TIME failed" );
        }

        if ( ( rescStatus = getSqlResultByInx( _result, COL_R_RESC_STATUS ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "getSqlResultByInx for COL_R_RESC_STATUS failed" );
        }

        if ( ( rescChildren = getSqlResultByInx( _result, COL_R_RESC_CHILDREN ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "getSqlResultByInx for COL_R_RESC_CHILDREN failed" );
        }

        if ( ( rescContext = getSqlResultByInx( _result, COL_R_RESC_CONTEXT ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "getSqlResultByInx for COL_R_RESC_CONTEXT failed" );
        }

        if ( ( rescParent = getSqlResultByInx( _result, COL_R_RESC_PARENT ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "getSqlResultByInx for COL_R_RESC_PARENT failed" );
        }

        if ( ( rescParentContext = getSqlResultByInx( _result, COL_R_RESC_PARENT_CONTEXT ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "getSqlResultByInx for COL_R_RESC_PARENT_CONTEXT failed" );
        }

        // =-=-=-=-=-=-=-
        // iterate through the rows, initialize a resource for each entry
        for ( int i = 0; i < _result->rowCnt; ++i ) {
            // =-=-=-=-=-=-=-
            // extract row values
            std::string tmpRescId        = &rescId->value[ rescId->len * i ];
            std::string tmpRescLoc       = &rescLoc->value[ rescLoc->len * i ];
            std::string tmpRescName      = &rescName->value[ rescName->len * i ];
            std::string tmpZoneName      = &zoneName->value[ zoneName->len * i ];
            std::string tmpRescType      = &rescType->value[ rescType->len * i ];
            std::string tmpRescInfo      = &rescInfo->value[ rescInfo->len * i ];
            std::string tmpFreeSpace     = &freeSpace->value[ freeSpace->len * i ];
            std::string tmpRescClass     = &rescClass->value[ rescClass->len * i ];
            std::string tmpRescCreate    = &rescCreate->value[ rescCreate->len * i ];
            std::string tmpRescModify    = &rescModify->value[ rescModify->len * i ];
            std::string tmpRescStatus    = &rescStatus->value[ rescStatus->len * i ];
            std::string tmpRescComments  = &rescComments->value[ rescComments->len * i ];
            std::string tmpRescVaultPath = &rescVaultPath->value[ rescVaultPath->len * i ];
            std::string tmpRescChildren  = &rescChildren->value[ rescChildren->len * i ];
            std::string tmpRescContext   = &rescContext->value[ rescContext->len * i ];
            std::string tmpRescParent    = &rescParent->value[ rescParent->len * i ];
            std::string tmpRescParentCtx = &rescParentContext->value[ rescParent->len * i ];

            // =-=-=-=-=-=-=-
            // create the resource and add properties for column values
            resource_ptr resc;
            error ret = load_resource_plugin( resc, tmpRescType, tmpRescName, tmpRescContext );
            if ( !ret.ok() ) {
                irods::log(PASS(ret));
                continue;
            }

            // =-=-=-=-=-=-=-
            // resolve the host name into a rods server host structure
            if ( tmpRescLoc != irods::EMPTY_RESC_HOST ) {
                rodsHostAddr_t addr;
                rstrcpy( addr.hostAddr, const_cast<char*>( tmpRescLoc.c_str() ), LONG_NAME_LEN );
                rstrcpy( addr.zoneName, const_cast<char*>( tmpZoneName.c_str() ), NAME_LEN );

                rodsServerHost_t* tmpRodsServerHost = 0;
                if ( resolveHost( &addr, &tmpRodsServerHost ) < 0 ) {
                    rodsLog( LOG_NOTICE, "procAndQueRescResult: resolveHost error for %s",
                             addr.hostAddr );
                }

                resc->set_property< rodsServerHost_t* >( RESOURCE_HOST, tmpRodsServerHost );

            }
            else {
                resc->set_property< rodsServerHost_t* >( RESOURCE_HOST, 0 );
            }

            rodsLong_t resource_id = strtoll( tmpRescId.c_str(), 0, 0 );
            resc->set_property<rodsLong_t>( RESOURCE_ID, resource_id );
            resc->set_property<long>( RESOURCE_QUOTA, RESC_QUOTA_UNINIT );

            resc->set_property<std::string>( RESOURCE_FREESPACE,      tmpFreeSpace );
            resc->set_property<std::string>( RESOURCE_ZONE,           tmpZoneName );
            resc->set_property<std::string>( RESOURCE_NAME,           tmpRescName );
            resc->set_property<std::string>( RESOURCE_LOCATION,       tmpRescLoc );
            resc->set_property<std::string>( RESOURCE_TYPE,           tmpRescType );
            resc->set_property<std::string>( RESOURCE_CLASS,          tmpRescClass );
            resc->set_property<std::string>( RESOURCE_PATH,           tmpRescVaultPath );
            resc->set_property<std::string>( RESOURCE_INFO,           tmpRescInfo );
            resc->set_property<std::string>( RESOURCE_COMMENTS,       tmpRescComments );
            resc->set_property<std::string>( RESOURCE_CREATE_TS,      tmpRescCreate );
            resc->set_property<std::string>( RESOURCE_MODIFY_TS,      tmpRescModify );
            resc->set_property<std::string>( RESOURCE_CHILDREN,       tmpRescChildren );
            resc->set_property<std::string>( RESOURCE_CONTEXT,        tmpRescContext );
            resc->set_property<std::string>( RESOURCE_PARENT,         tmpRescParent );
            resc->set_property<std::string>( RESOURCE_PARENT_CONTEXT, tmpRescParentCtx );

            if ( tmpRescStatus == std::string( RESC_DOWN ) ) {
                resc->set_property<int>( RESOURCE_STATUS, INT_RESC_STATUS_DOWN );
            }
            else {
                resc->set_property<int>( RESOURCE_STATUS, INT_RESC_STATUS_UP );
            }

            // =-=-=-=-=-=-=-
            // add new resource to the map
            resource_name_map_[ tmpRescName ] = resc;
            resource_id_map_[ resource_id ] = resc;

        } // for i

        return SUCCESS();

    } // process_init_results

// =-=-=-=-=-=-=-
// public - given a type, load up a resource plugin
    error resource_manager::init_from_type( std::string   _type,
                                            std::string   _key,
                                            std::string   _inst,
                                            std::string   _ctx,
                                            resource_ptr& _resc ) {
        // =-=-=-=-=-=-=-
        // create the resource and add properties for column values
        error ret = load_resource_plugin( _resc, _type, _inst, _ctx );
        if ( !ret.ok() ) {
            return PASSMSG( "Failed to load Resource Plugin", ret );
        }

        resource_name_map_[ _key ] = _resc;

        return SUCCESS();

    } // init_from_type

// =-=-=-=-=-=-=-
// private - walk the resource map and wire children up to parents
    error resource_manager::init_child_map( void ) {
        for( auto itr : resource_name_map_ ) {
            const std::string& child_name = itr.first;
            resource_ptr       child_resc = itr.second;

            std::string parent_id_str;
            error ret = child_resc->get_property<std::string>(
                            RESOURCE_PARENT,
                            parent_id_str );
            if(!ret.ok() || parent_id_str.empty()) {
                continue;
            }

            rodsLong_t parent_id = 0;
            ret = lexical_cast<rodsLong_t>(
                      parent_id_str,
                      parent_id );
            if(!ret.ok()) {
                irods::log(PASS(ret));
                continue;
            }

            if(!resource_id_map_.has_entry(parent_id)) {
                rodsLog(
                    LOG_ERROR,
                    "invalid parent resource id %ld",
                    parent_id );
                continue;
            }

            resource_ptr parent_resc = resource_id_map_[parent_id];

            std::string parent_child_context;
            ret = child_resc->get_property<std::string>(
                      RESOURCE_PARENT_CONTEXT,
                      parent_child_context );
            if(!ret.ok()) {
            }

            parent_resc->add_child(
                child_name,
                parent_child_context,
                child_resc);

            child_resc->set_parent(parent_resc);

            rodsLog(
                LOG_DEBUG,
                "%s - add [%s][%s] to [%ld]",
                __FUNCTION__,
                child_name.c_str(),
                parent_child_context.c_str(),
                parent_id);

        } // for itr

        return SUCCESS();

    } // init_child_map

// =-=-=-=-=-=-=-
// public - print the list of local resources out to stderr
    void resource_manager::print_local_resources() {
        lookup_table< boost::shared_ptr< resource > >::iterator itr;
        for ( itr = resource_name_map_.begin(); itr != resource_name_map_.end(); ++itr ) {
            std::string loc, path, name;
            error path_err = itr->second->get_property< std::string >( RESOURCE_PATH, path );
            error loc_err  = itr->second->get_property< std::string >( RESOURCE_LOCATION, loc );
            if ( path_err.ok() && loc_err.ok() && "localhost" == loc ) {
                rodsLog( LOG_NOTICE, "   RescName: %s, VaultPath: %s\n",
                         itr->first.c_str(), path.c_str() );
            }

        } // for itr

    } // print_local_resources

// =-=-=-=-=-=-=-
// private - gather the post disconnect maintenance operations
//           from the resource plugins
    error resource_manager::gather_operations() {
        // =-=-=-=-=-=-=-
        // vector of already processed resources
        std::vector< std::string > proc_vec;

        // =-=-=-=-=-=-=-
        // iterate over all of the resources
        lookup_table< boost::shared_ptr< resource > >::iterator resc_itr;
        for ( resc_itr = resource_name_map_.begin(); resc_itr != resource_name_map_.end(); ++resc_itr ) {
            resource_ptr& resc = resc_itr->second;

            // =-=-=-=-=-=-=-
            // skip if already processed
            std::string name;
            error get_err = resc->get_property< std::string >( RESOURCE_NAME, name );

            if ( get_err.ok() ) {
                std::vector< std::string >::iterator itr;
                itr = std::find< std::vector< std::string >::iterator, std::string >( proc_vec.begin(), proc_vec.end(), name );
                if ( proc_vec.end() != itr ) {
                    continue;
                }
            }
            else {
                std::stringstream msg;
                msg << "resource_manager::gather_operations - failed to get property ";
                msg << "[name] for resource";
                return PASSMSG( msg.str(), get_err );
            }

            // =-=-=-=-=-=-=-
            // vector which will hold this 'top level resource' ops
            std::vector< pdmo_type > resc_ops;

            // =-=-=-=-=-=-=-
            // cache the parent operator
            pdmo_type pdmo_op;
            error pdmo_err = resc->post_disconnect_maintenance_operation( pdmo_op );
            if ( pdmo_err.ok() ) {
                resc_ops.push_back( pdmo_op );
            }

            // =-=-=-=-=-=-=-
            // mark this resource done
            proc_vec.push_back( name );

            // =-=-=-=-=-=-=-
            // dive if children are present
            std::string child_str;
            error child_err = resc->get_property< std::string >( RESOURCE_CHILDREN, child_str );
            if ( child_err.ok() && !child_str.empty() ) {
                gather_operations_recursive( child_str, proc_vec, resc_ops );
            }

            // =-=-=-=-=-=-=-
            // if we got ops, add vector of ops to mgr's vector
            if ( !resc_ops.empty() ) {
                maintenance_operations_.push_back( resc_ops );
            }

        } // for itr

        return SUCCESS();

    } // gather_operations

// =-=-=-=-=-=-=-
/// private - lower level recursive call to gather the post disconnect
//            maintenance operations from the resources, in breadth first order
    error resource_manager::gather_operations_recursive( const std::string&          _children,
            std::vector< std::string >& _proc_vec,
            std::vector< pdmo_type  >&  _resc_ops ) {
        // =-=-=-=-=-=-=-
        // create a child parser to traverse the list
        children_parser parser;
        parser.set_string( _children );
        children_parser::children_map_t children_list;
        error ret = parser.list( children_list );
        if ( !ret.ok() ) {
            return PASSMSG( "gather_operations_recursive failed.", ret );
        }

        // =-=-=-=-=-=-=-
        // iterate over all of the children, cache the operators
        children_parser::children_map_t::const_iterator itr;
        for ( itr = children_list.begin(); itr != children_list.end(); ++itr ) {
            std::string child = itr->first;

            // =-=-=-=-=-=-=-
            // lookup the child resource pointer
            resource_ptr resc;
            error get_err = resource_name_map_.get( child, resc );
            if ( get_err.ok() ) {
                // =-=-=-=-=-=-=-
                // cache operation if there is one
                pdmo_type pdmo_op;
                error pdmo_ret = resc->post_disconnect_maintenance_operation( pdmo_op );
                if ( pdmo_ret.ok() ) {
                    _resc_ops.push_back( pdmo_op );
                }

                // =-=-=-=-=-=-=-
                // mark this child as done
                _proc_vec.push_back( child );

            }
            else {
                std::stringstream msg;
                msg << "failed to get resource for key [";
                msg << child;
                msg << "]";
                return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
            }

        } // for itr

        // =-=-=-=-=-=-=-
        // iterate over all of the children again, recurse if they have more children
        for ( itr = children_list.begin(); itr != children_list.end(); ++itr ) {
            std::string child = itr->first;

            // =-=-=-=-=-=-=-
            // lookup the child resource pointer
            resource_ptr resc;
            error get_err = resource_name_map_.get( child, resc );
            if ( get_err.ok() ) {
                std::string child_str;
                error child_err = resc->get_property< std::string >( RESOURCE_CHILDREN, child_str );
                if ( child_err.ok() && !child_str.empty() ) {
                    error gather_err = gather_operations_recursive( child_str, _proc_vec, _resc_ops );
                }

            }
            else {
                std::stringstream msg;
                msg << "failed to get resource for key [";
                msg << child;
                msg << "]";
                return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
            }

        } // for itr

        return SUCCESS();

    } // gather_operations_recursive

// =-=-=-=-=-=-=-
// public - call the start op on the resource plugins
    error resource_manager::start_resource_plugins( ) {
        // =-=-=-=-=-=-=-
        // iterate through resource plugins
        lookup_table< resource_ptr >::iterator itr;
        for ( itr  = resource_name_map_.begin();
                itr != resource_name_map_.end();
                ++itr ) {
            // =-=-=-=-=-=-=-
            // if any resources need a pdmo, return true;
            error ret = itr->second->start_operation( );
            if ( !ret.ok() ) {
                irods::log( ret );
            }

        } // for itr

        return SUCCESS();

    } // start_resource_operations


// =-=-=-=-=-=-=-
// public - exec the pdmos ( post disconnect maintenance operations ) in order
    bool resource_manager::need_maintenance_operations( ) {
        bool need_pdmo = false;

        // =-=-=-=-=-=-=-
        // iterate through resource plugins
        lookup_table< resource_ptr >::iterator itr;
        for ( itr  = resource_name_map_.begin();
                itr != resource_name_map_.end();
                ++itr ) {
            // =-=-=-=-=-=-=-
            // if any resources need a pdmo, return true;
            bool flg = false;
            itr->second->need_post_disconnect_maintenance_operation( flg );
            if ( flg ) {
                need_pdmo = true;
                break;
            }

        } // for itr

        return need_pdmo;

    } // need_maintenance_operations

// =-=-=-=-=-=-=-
// public - exec the pdmos ( post disconnect maintenance operations ) in order
    int resource_manager::call_maintenance_operations( rcComm_t* _comm ) {
        int result = 0;

        // =-=-=-=-=-=-=-
        // iterate through op vectors
        std::vector< std::vector< pdmo_type > >::iterator vec_itr;
        for ( vec_itr  = maintenance_operations_.begin();
                vec_itr != maintenance_operations_.end();
                ++vec_itr ) {
            // =-=-=-=-=-=-=-
            // iterate through ops
            std::vector< pdmo_type >::iterator op_itr;
            for ( op_itr  = vec_itr->begin();
                    op_itr != vec_itr->end();
                    ++op_itr ) {
                // =-=-=-=-=-=-=-
                // call the op
                try {
                    error ret = ( ( *op_itr ) )( _comm );
                    if ( !ret.ok() ) {
                        log( PASSMSG( "resource_manager::call_maintenance_operations - op failed", ret ) );
                        result = ret.code();
                    }
                }
                catch ( const boost::bad_function_call& ) {
                    rodsLog( LOG_ERROR, "maintenance operation threw boost::bad_function_call" );
                    result = SYS_INTERNAL_ERR;
                }

            } // for op_itr

        } // for vec_itr

        return result;
    } // call_maintenance_operations

    error resource_manager::hier_to_leaf_id(
        const std::string& _hier,
        rodsLong_t&        _id ) {
        if(_hier.empty()) {
            return ERROR(
                       HIERARCHY_ERROR,
                       "empty hierarchy string" );
        }

        hierarchy_parser p;
        p.set_string( _hier );

        std::string leaf;
        p.last_resc( leaf );

        if( !resource_name_map_.has_entry(leaf) ) {
            return ERROR(
                       SYS_RESC_DOES_NOT_EXIST,
                       leaf );
        }

        resource_ptr resc = resource_name_map_[leaf];

        rodsLong_t id = 0;
        error ret = resc->get_property<rodsLong_t>(
                        RESOURCE_ID,
                        id );
        if( !ret.ok() ) {
            return PASS(ret);
        }

        _id = id;

        return SUCCESS();

    } // hier_to_leaf_id

    error resource_manager::leaf_id_to_hier(
        const rodsLong_t& _id,
        std::string&      _hier ) {
        if( !resource_id_map_.has_entry(_id) ) {
            std::stringstream msg;
            msg << "invalid resource id: " << _id;
            return ERROR(
                       SYS_RESC_DOES_NOT_EXIST,
                       msg.str() );
        }

        resource_ptr resc = resource_id_map_[ _id ];

        std::string hier;
        error ret = resc->get_property<std::string>(
                        RESOURCE_NAME,
                        hier );
        if( !ret.ok() ) {
            return PASS(ret);
        }

        resc->get_parent(resc);
        while( resc.get() ) {
            std::string name;
            error ret = resc->get_property<std::string>(
                            RESOURCE_NAME,
                            name );
            if( !ret.ok() ) {
                return PASS(ret);
            }

            hier.insert( 0, ";" );
            hier.insert( 0, name);

            resc->get_parent(resc);
        }

        _hier = hier;

        return SUCCESS();

    } // leaf_id_to_hier

    error resource_manager::resc_id_to_name(
        const rodsLong_t& _id,
        std::string&      _name ) {
        // parent name might be 'empty'
        if(!_id) {
            return SUCCESS();
        }

        if( !resource_id_map_.has_entry(_id) ) {
            std::stringstream msg;
            msg << "invalid resource id: " << _id;
            return ERROR(
                       SYS_RESC_DOES_NOT_EXIST,
                       msg.str() );
        }

        resource_ptr resc = resource_id_map_[ _id ];

        std::string hier;
        error ret = resc->get_property<std::string>(
                        RESOURCE_NAME,
                        _name );
        if( !ret.ok() ) {
            return PASS(ret);
        }

        return SUCCESS();

    } // resc_id_to_name

    error resource_manager::resc_id_to_name(
        const std::string& _id_str,
        std::string&       _name ) {
        // parent name might be 'empty'
        if("0" == _id_str || _id_str.empty()) {
            return SUCCESS();
        }

        rodsLong_t resc_id = 0;
        error ret = lexical_cast<rodsLong_t>(
                       _id_str,
                       resc_id );
        if(!ret.ok()) {
            return PASS(ret);
        }

        if( !resource_id_map_.has_entry(resc_id) ) {
            std::stringstream msg;
            msg << "invalid resource id: " << _id_str;
            return ERROR(
                       SYS_RESC_DOES_NOT_EXIST,
                       msg.str() );
        }
        resource_ptr resc = resource_id_map_[ resc_id ];

        std::string hier;
        ret = resc->get_property<std::string>(
                        RESOURCE_NAME,
                        _name );
        if( !ret.ok() ) {
            return PASS(ret);
        }

        return SUCCESS();

    } // resc_id_to_name

    error resource_manager::is_coordinating_resource(
        const std::string& _resc_name,
        bool&              _ret ) {
        if( !resource_name_map_.has_entry(_resc_name) ) {
            return ERROR(
                       SYS_RESC_DOES_NOT_EXIST,
                       _resc_name );
        }

        resource_ptr resc = resource_name_map_[_resc_name];

        _ret = resc->num_children() > 0;

        return SUCCESS();
    } // is_coordinating_resource

}; // namespace irods
