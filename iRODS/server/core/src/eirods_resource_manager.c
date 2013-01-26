/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_resource_manager.h"
#include "eirods_log.h"
#include "eirods_string_tokenize.h"

// =-=-=-=-=-=-=-
// irods includes
#include "getRescQuota.h"
#include "eirods_children_parser.h"
#include "rsGlobalExtern.h"

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>

// global
eirods::resource_manager resc_mgr;

namespace eirods {

    // =-=-=-=-=-=-=-
    // public - Constructor
    resource_manager::resource_manager() {
    } // ctor
        
    // =-=-=-=-=-=-=-
    // public - Copy Constructor
    resource_manager::resource_manager( const resource_manager& _rhs ) {
    } // cctor
    
    // =-=-=-=-=-=-=-
    // public - Destructor
    resource_manager::~resource_manager(  ) {
    } // cctor

    // =-=-=-=-=-=-=-
    // public - retrieve a resource given its key
    error resource_manager::resolve( std::string _key, resource_ptr& _value ) {

        if( _key.empty() ) {
            error ret;
            ret = resolve_from_property< std::string >( "type", "unix file system", _value );
            return ret;
        }

        if( resources_.has_entry( _key ) ) {
            _value = resources_[ _key ];
            return SUCCESS();   
                
        } else {
            std::stringstream msg;
            msg << "resource_manager::resolve - no resource found for name ["
                << _key << "]";
            return ERROR( -1, msg.str() );
                
        }

    } // resolve

    // =-=-=-=-=-=-=-
    // public - retrieve a resource given a vault path
    error resource_manager::resolve_from_physical_path( std::string   _physical_path, 
                                                        resource_ptr& _resc ) {
        // =-=-=-=-=-=-=-
        // simple flag to state a resource matching the prop and value is found
        bool found = false;     
                
        // =-=-=-=-=-=-=-
        // quick check on the resource table
        if( resources_.empty() ) {
            return ERROR( -1, "resource_manager::resolve_from_physical_path - empty resource table" );
        }
       
        // =-=-=-=-=-=-=-
        // quick check on the path that it has something in it
        if( _physical_path.empty() ) {
            return ERROR( -1, "resource_manager::resolve_from_physical_path - empty property" );
        }

        // =-=-=-=-=-=-=-
        // iterate through the map and search for our path
        lookup_table< resource_ptr >::iterator itr = resources_.begin();
        for( ; itr != resources_.end(); ++itr ) {
            // =-=-=-=-=-=-=-
            // query resource for the property value
            std::string value;
            error ret = itr->second->get_property<std::string>( "path", value );

            // =-=-=-=-=-=-=-
            // if we get a good parameter 
            if( ret.ok() ) {
                // =-=-=-=-=-=-=-
                // compare incoming value and stored value
                // one may be a subset of the other so compare both ways
                if( _physical_path.find( value ) != std::string::npos || 
                    value.find( _physical_path ) != std::string::npos ) {
                    // =-=-=-=-=-=-=-
                    // if we get a match, walk up the parents of the resource
                    // until we hit the root as this could be a resource composition
                    // and eirods should only be talking to the root of a composition.
                    resource_ptr resc2, resc1 = itr->second;
                    error        err  = resc1->get_parent( resc2 );
                    while( err.ok() ) {
                        resc1 = resc2;
                        err = resc1->get_parent( resc2 );

                    } // while

                    // =-=-=-=-=-=-=-
                    // finally set our flag and cache the resource pointer
                    found = true;
                    _resc = resc1;

                    // =-=-=-=-=-=-=-
                    // and... were done.
                    break;
                }
            } else {
                std::stringstream msg;
                msg << "resource_manager::resolve_from_physical_path - ";
                msg << "failed to get vault parameter from resource";
                msg << ret.code();
                eirods::error err = PASS( false, -1, msg.str(), ret );
            }

        } // for itr

        // =-=-=-=-=-=-=-
        // did we find a resource and is the ptr valid?
        if( true == found && _resc.get() ) {
            return SUCCESS();
        } else {
            std::stringstream msg;
            msg << "resource_manager::resolve_from_physical_path - ";
            msg << "failed to find resource for path [";
            msg << _physical_path;
            msg << "]";
            return ERROR( -1, msg.str() );
        }

    } // resolve_from_physical_path

    // =-=-=-=-=-=-=-
    // resolve a resource from a first_class_object
    error resource_manager::resolve( const eirods::first_class_object& _object, 
                                     resource_ptr&                     _resc ) {
        // =-=-=-=-=-=-=-
        // find a resource matching the vault path in the physical path
        error ret =  resolve_from_physical_path( _object.physical_path(), _resc );

        // =-=-=-=-=-=-=-
        // if we cant find a resource for a given path, find any unix file system resource
        // as this is necessary for some cases such as registration of a file which doesnt
        // have a proper vault path.  this issue will be fixed when we push fcos up through
        // the server api calls as we can treat them as a new class of fco
        if( !ret.ok() ) {
            ret = resolve_from_property< std::string >( "type", "unix file system", _resc );

        }

        return ret;

    } // resolve

    // =-=-=-=-=-=-=-
    // public - connect to the catalog and query for all the 
    //          attached resources and instantiate them
    error resource_manager::init_from_catalog( rsComm_t* _comm ) {
        // =-=-=-=-=-=-=-
        // clear existing resource map and initialize
        resources_.clear();

        // =-=-=-=-=-=-=-
        // set up data structures for a gen query
        genQueryInp_t  genQueryInp;
        genQueryOut_t* genQueryOut = NULL;

        error proc_ret;

        memset (&genQueryInp, 0, sizeof (genQueryInp));

        addInxIval( &genQueryInp.selectInp, COL_R_RESC_ID,       1 );
        addInxIval( &genQueryInp.selectInp, COL_R_RESC_NAME,     1 );
        addInxIval( &genQueryInp.selectInp, COL_R_ZONE_NAME,     1 );
        addInxIval( &genQueryInp.selectInp, COL_R_TYPE_NAME,     1 );
        addInxIval( &genQueryInp.selectInp, COL_R_CLASS_NAME,    1 );
        addInxIval( &genQueryInp.selectInp, COL_R_LOC,           1 );
        addInxIval( &genQueryInp.selectInp, COL_R_VAULT_PATH,    1 );
        addInxIval( &genQueryInp.selectInp, COL_R_FREE_SPACE,    1 );
        addInxIval( &genQueryInp.selectInp, COL_R_RESC_INFO,     1 );
        addInxIval( &genQueryInp.selectInp, COL_R_RESC_COMMENT,  1 );
        addInxIval( &genQueryInp.selectInp, COL_R_CREATE_TIME,   1 );
        addInxIval( &genQueryInp.selectInp, COL_R_MODIFY_TIME,   1 );
        addInxIval( &genQueryInp.selectInp, COL_R_RESC_STATUS,   1 );
        addInxIval( &genQueryInp.selectInp, COL_R_RESC_CHILDREN, 1 );
        addInxIval( &genQueryInp.selectInp, COL_R_RESC_CONTEXT,  1 );
        addInxIval( &genQueryInp.selectInp, COL_R_RESC_PARENT,   1 );
        addInxIval( &genQueryInp.selectInp, COL_R_RESC_OBJCOUNT, 1 );
        
        genQueryInp.maxRows = MAX_SQL_ROWS;

        // =-=-=-=-=-=-=-
        // init continueInx to pass for first loop
        int continueInx = 1;    
            
        // =-=-=-=-=-=-=-
        // loop until continuation is not requested
        while( continueInx > 0 ) {
            
            // =-=-=-=-=-=-=-
            // perform the general query
            int status = rsGenQuery( _comm, &genQueryInp, &genQueryOut );
                
            // =-=-=-=-=-=-=-
            // perform the general query
            if( status < 0 ) {
                if( status != CAT_NO_ROWS_FOUND ) {
                    rodsLog( LOG_NOTICE,"initResc: rsGenQuery error, status = %d",
                             status );
                }
                
                clearGenQueryInp( &genQueryInp );
                return ERROR( status, "init_from_catalog - genqery failed." );
            
            } // if
                
            // =-=-=-=-=-=-=-
            // given a series of rows, each being a resource, create a resource and add it to the table 
            proc_ret = process_init_results( genQueryOut );

            // =-=-=-=-=-=-=-
            // if error is not valid, clear query and bail
            if( !proc_ret.ok() ) {
                eirods::error log_err = PASS( false, -1, "init_from_catalog - process_init_results failed", proc_ret );
                eirods::log( log_err );
                freeGenQueryOut (&genQueryOut);
                break;
            } else {
                if( genQueryOut != NULL ) {
                    continueInx = genQueryInp.continueInx = genQueryOut->continueInx;
                    freeGenQueryOut( &genQueryOut );
                } else {
                    continueInx = 0;
                }
                
            } // else

        } // while

        clearGenQueryInp( &genQueryInp );

        // =-=-=-=-=-=-=-
        // pass along the error if we are in an error state
        if( !proc_ret.ok() ) {
            return PASS( false, -1, "process_init_results failed.", proc_ret );
        } 

        // =-=-=-=-=-=-=-
        // Update child resource maps
        proc_ret = init_child_map();
        if(!proc_ret.ok()) {
            return PASS(false, -1, "init_child_map failed.", proc_ret);
        }
 
        // =-=-=-=-=-=-=-
        // gather the post disconnect maintenance operations
        error op_ret = gather_operations();
        if( !op_ret.ok() ) {
            return PASS( false, -1, "gather_operations failed.", op_ret);
        }
        
        // =-=-=-=-=-=-=-
        // win!
        return SUCCESS();

    } // init_from_catalog
    
    // =-=-=-=-=-=-=-
    // public - take results from genQuery, extract values and create resources
    error resource_manager::process_init_results( genQueryOut_t* _result ) {
        // =-=-=-=-=-=-=-
        // extract results from query
        if ( !_result ) {
            return ERROR( -1, "resource ctor :: _result parameter is null" );
        }

        // =-=-=-=-=-=-=-
        // values to extract from query
        sqlResult_t *rescId       = 0, *rescName      = 0, *zoneName   = 0, *rescType   = 0, *rescClass = 0;
        sqlResult_t *rescLoc      = 0, *rescVaultPath = 0, *freeSpace  = 0, *rescInfo   = 0;
        sqlResult_t *rescComments = 0, *rescCreate    = 0, *rescModify = 0, *rescStatus = 0;
        sqlResult_t *rescChildren = 0, *rescContext   = 0, *rescParent = 0, *rescObjCount = 0;

        // =-=-=-=-=-=-=-
        // extract results from query
        if( ( rescId = getSqlResultByInx( _result, COL_R_RESC_ID ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX,"resource ctor :: getSqlResultByInx for COL_R_RESC_ID failed" );
        }

        if( ( rescName = getSqlResultByInx( _result, COL_R_RESC_NAME ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_RESC_NAME failed" );
        }

        if( ( zoneName = getSqlResultByInx( _result, COL_R_ZONE_NAME ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_ZONE_NAME failed" );
        }
            
        if( ( rescType = getSqlResultByInx(_result, COL_R_TYPE_NAME ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_TYPE_NAME failed" );
        }

        if( ( rescClass = getSqlResultByInx( _result, COL_R_CLASS_NAME ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_CLASS_NAME failed" );
        }
            
        if( ( rescLoc = getSqlResultByInx( _result, COL_R_LOC ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_LOC failed" );
        }

        if( ( rescVaultPath = getSqlResultByInx( _result, COL_R_VAULT_PATH ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_VAULT_PATH failed" );
        }

        if( ( freeSpace = getSqlResultByInx( _result, COL_R_FREE_SPACE ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_FREE_SPACE failed" );
        }

        if( ( rescInfo = getSqlResultByInx( _result, COL_R_RESC_INFO ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_RESC_INFO failed" );
        }

        if( ( rescComments = getSqlResultByInx( _result, COL_R_RESC_COMMENT ) ) == NULL ) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_RESC_COMMENT failed" );
        }

        if( ( rescCreate = getSqlResultByInx( _result, COL_R_CREATE_TIME ) ) == NULL) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_CREATE_TIME failed" );
        }

        if( ( rescModify = getSqlResultByInx( _result, COL_R_MODIFY_TIME ) ) == NULL) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_MODIFY_TIME failed" );
        }

        if( ( rescStatus = getSqlResultByInx( _result, COL_R_RESC_STATUS ) ) == NULL) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_RESC_STATUS failed" );
        }

        if( ( rescChildren = getSqlResultByInx( _result, COL_R_RESC_CHILDREN ) ) == NULL) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_RESC_CHILDREN failed" );
        }

        if( ( rescContext = getSqlResultByInx( _result, COL_R_RESC_CONTEXT ) ) == NULL) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_RESC_CONTEXT failed" );
        }

        if( ( rescParent = getSqlResultByInx( _result, COL_R_RESC_PARENT ) ) == NULL) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_RESC_PARENT failed" );
        }

        if( ( rescObjCount = getSqlResultByInx( _result, COL_R_RESC_OBJCOUNT ) ) == NULL) {
            return ERROR( UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_RESC_OBJCOUNT failed" );
        }

        // =-=-=-=-=-=-=-
        // iterate through the rows, initialize a resource for each entry
        for( int i = 0; i < _result->rowCnt; ++i ) {
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
            std::string tmpRescObjCount  = &rescObjCount->value[ rescObjCount->len * i ];
            
            // =-=-=-=-=-=-=-
            // resolve the host name into a rods server host structure
            rodsHostAddr_t addr;
            rstrcpy( addr.hostAddr, const_cast<char*>( tmpRescLoc.c_str()  ), LONG_NAME_LEN );
            rstrcpy( addr.zoneName, const_cast<char*>( tmpZoneName.c_str() ), NAME_LEN );

            rodsServerHost_t* tmpRodsServerHost = 0;
            if( resolveHost( &addr, &tmpRodsServerHost ) < 0 ) {
                rodsLog( LOG_NOTICE, "procAndQueRescResult: resolveHost error for %s", 
                         addr.hostAddr );
            }

            // =-=-=-=-=-=-=-
            // create the resource and add properties for column values
            resource_ptr resc;
            error ret = load_resource_plugin( resc, tmpRescType, tmpRescName, tmpRescContext );
            if( !ret.ok() ) {
                return PASS( false, -1, "Failed to load Resource Plugin", ret );        
            }

            resc->set_property< rodsServerHost_t* >( "host", tmpRodsServerHost );
                
            resc->set_property<long>( "id", strtoll( tmpRescId.c_str(), 0, 0 ) );
            resc->set_property<long>( "freespace", strtoll( tmpFreeSpace.c_str(), 0, 0 ) );
            resc->set_property<long>( "quota", RESC_QUOTA_UNINIT );
                
            resc->set_property<std::string>( "zone",     tmpZoneName );
            resc->set_property<std::string>( "name",     tmpRescName );
            resc->set_property<std::string>( "location", tmpRescLoc );
            resc->set_property<std::string>( "type",     tmpRescType );
            resc->set_property<std::string>( "class",    tmpRescClass );
            resc->set_property<std::string>( "path",     tmpRescVaultPath );
            resc->set_property<std::string>( "info",     tmpRescInfo );
            resc->set_property<std::string>( "comments", tmpRescComments );
            resc->set_property<std::string>( "create",   tmpRescCreate );
            resc->set_property<std::string>( "modify",   tmpRescModify );
            resc->set_property<std::string>( "children", tmpRescChildren );
            resc->set_property<std::string>( "parent",   tmpRescParent );
            resc->set_property<std::string>( "context",  tmpRescContext );
            
            if( tmpRescStatus ==  std::string( RESC_DOWN ) ) {
                resc->set_property<int>( "status", INT_RESC_STATUS_DOWN );
            } else {
                resc->set_property<int>( "status", INT_RESC_STATUS_UP );
            }

            // =-=-=-=-=-=-=-
            // add new resource to the map
            resources_[ tmpRescName ] = resc;

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
        if( !ret.ok() ) {
            return PASS( false, -1, "Failed to load Resource Plugin", ret );    
        }

        resources_[ _key ] = _resc;

        return SUCCESS();

    } // init_from_type

    // =-=-=-=-=-=-=-
    // private - walk the resource map and wire children up to parents
    error resource_manager::init_child_map(void) {
        error result = SUCCESS();

        // Iterate over all the resources
        lookup_table< boost::shared_ptr< resource > >::iterator it;
        for(it = resources_.begin(); it != resources_.end(); ++it) {
            resource_ptr resc = it->second;

            // Get the children string and resource name
            std::string children_string;
            error ret = resc->get_property<std::string>("children", children_string);
            if(!ret.ok()) {
                result = PASS(false, -1, "init_child_map failed.", ret);
            } else {
                std::string resc_name;
                error ret = resc->get_property<std::string>("name", resc_name);
                if(!ret.ok()) {
                    result = PASS(false, -1, "init_child_map failed.", ret);
                } else {

                    // Get the list of children and their contexts from the resource
                    children_parser parser;
                    parser.set_string(children_string);
                    children_parser::children_map_t children_list;
                    error ret = parser.list(children_list);
                    if(!ret.ok()) {
                        result = PASS(false, -1, "init_child_map failed.", ret);
                    } else {

                        // Iterate over all of the children
                        children_parser::children_map_t::const_iterator itr;
                        for(itr = children_list.begin(); itr != children_list.end(); ++itr) {
                            std::string child = itr->first;
                            std::string context = itr->second;

                            // Lookup the child resource pointer
                            lookup_table< boost::shared_ptr< resource > >::iterator child_itr = resources_.find(child);
                            if(child_itr == resources_.end()) {
                                std::stringstream msg;
                                msg << "init_child_map: Failed to find child \"" << child << "\" in resources.";
                                result = ERROR(-1, msg.str());
                            } else {

                                // Add a reference to the child resource pointer and its context to the resource
                                resource_ptr child_resc = child_itr->second;
                                error ret = resc->add_child(child, context, child_resc);
                                if(!ret.ok()) {
                                    result = PASS(false, -1, "init_child_map failed.", ret);
                                }

                                // set the parent for the child resource
                                child_resc->set_parent( resc );
                            }
                        } // for itr
                    } // else parse list
                } // else get name
            } // else get child string
        } // for it
        return result;
    } // init_child_map

    // =-=-=-=-=-=-=-
    // public - print the list of local resources out to stderr
    void resource_manager::print_local_resources() { 
        lookup_table< boost::shared_ptr< resource > >::iterator itr;
        for( itr = resources_.begin(); itr != resources_.end(); ++itr ) {
            std::string loc, path, name;
            error path_err = itr->second->get_property< std::string >( "path", path );
            error loc_err  = itr->second->get_property< std::string >( "location", loc );
            if( path_err.ok() && loc_err.ok() && "localhost" == loc ) {
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
        for( resc_itr = resources_.begin(); resc_itr != resources_.end(); ++resc_itr ) {
            resource_ptr& resc = resc_itr->second;

            // =-=-=-=-=-=-=-
            // skip if already processed
            std::string name;
            error get_err = resc->get_property< std::string >( "name", name );

            if( get_err.ok() ) {
                std::vector< std::string >::iterator itr;
                itr = std::find< std::vector< std::string >::iterator, std::string >( proc_vec.begin(), proc_vec.end(), name );
                if( proc_vec.end() != itr ) {
                    continue;
                }
            } else {
                std::stringstream msg;
                msg << "resource_manager::gather_operations - failed to get property ";
                msg << "[name] for resource";
                return PASSMSG( msg.str(), get_err );
            }

            // =-=-=-=-=-=-=-
            // vector which will hold this 'top level resource' ops
            vector< pdmo_type > resc_ops;

            // =-=-=-=-=-=-=-
            // cache the parent operator
            pdmo_type pdmo_op;
            error pdmo_err = resc->post_disconnect_maintenance_operation( pdmo_op );
            if( pdmo_err.ok() ) {
                resc_ops.push_back( pdmo_op );
            }

            // =-=-=-=-=-=-=-
            // mark this resource done
            proc_vec.push_back( name );

            // =-=-=-=-=-=-=-
            // dive if children are present
            std::string child_str;
            error child_err = resc->get_property< std::string >( "children", child_str );
            if( child_err.ok() && !child_str.empty() ) {
                gather_operations_recursive( child_str, proc_vec, resc_ops );
            }
            
            // =-=-=-=-=-=-=-
            // if we got ops, add vector of ops to mgr's vector
            if( !resc_ops.empty() ) {
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
        if(!ret.ok()) {
            return PASS(false, -1, "gather_operations_recursive failed.", ret);
        }

        // =-=-=-=-=-=-=-
        // iterate over all of the children, cache the operators
        children_parser::children_map_t::const_iterator itr;
        for( itr = children_list.begin(); itr != children_list.end(); ++itr ) {
            std::string child = itr->first;

            // =-=-=-=-=-=-=-
            // lookup the child resource pointer
            resource_ptr resc;
            error get_err = resources_.get( child, resc );
            if( get_err.ok() ) {
                // =-=-=-=-=-=-=-
                // cache operation if there is one
                pdmo_type pdmo_op;
                error pdmo_ret = resc->post_disconnect_maintenance_operation( pdmo_op ); 
                if( pdmo_ret.ok() ) {
                    _resc_ops.push_back( pdmo_op );
                }
                
                // =-=-=-=-=-=-=-
                // mark this child as done
                _proc_vec.push_back( child );

            } else {
                std::stringstream msg;
                msg << "gather_operations_recursive - failed to get resource for key [";
                msg << child;
                msg << "]";
                return ERROR( -1, msg.str() );
            }

        } // for itr

        // =-=-=-=-=-=-=-
        // iterate over all of the children again, recurse if they have more children
        for( itr = children_list.begin(); itr != children_list.end(); ++itr ) {
            std::string child = itr->first;

            // =-=-=-=-=-=-=-
            // lookup the child resource pointer
            resource_ptr resc;
            error get_err = resources_.get( child, resc );
            if( get_err.ok() ) {
                std::string child_str;
                error child_err = resc->get_property< std::string >( "children", child_str );
                if( child_err.ok() && !child_str.empty() ) {
                    error gather_err = gather_operations_recursive( child_str, _proc_vec, _resc_ops );
                }

            } else {
                std::stringstream msg;
                msg << "gather_operations_recursive - failed to get resource for key [";
                msg << child;
                msg << "]";
                return ERROR( -1, msg.str() );
            }

        } // for itr

        return SUCCESS();

    } // gather_operations_recursive

    // =-=-=-=-=-=-=-
    // public - exec the pdmos ( post disconnect maintenance operations ) in order
    void resource_manager::call_maintenance_operations(  ) {
        // =-=-=-=-=-=-=-
        // iterate through op vectors
        std::vector< std::vector< pdmo_type > >::iterator vec_itr;
        for( vec_itr  = maintenance_operations_.begin(); 
             vec_itr != maintenance_operations_.end();
             ++vec_itr ) {
            // =-=-=-=-=-=-=-
            // iterate through ops
            std::vector< pdmo_type >::iterator op_itr;
            for( op_itr  = vec_itr->begin(); 
                 op_itr != vec_itr->end(); 
                 ++op_itr ) {
                // =-=-=-=-=-=-=-
                // call the op
                error ret = ((*op_itr))();
                if( !ret.ok() ) {
                    log( PASSMSG( "resource_manager::call_maintenance_operations - op failed", ret ) );
                }

            } // for op_itr

        } // for vec_itr

    } // call_maintenance_operations

}; // namespace eirods



