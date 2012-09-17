


// =-=-=-=-=-=-=-
// E-iRODS Includes
#include "eirods_resource_manager.h"
#include "eirods_log.h"
#include "getRescQuota.h"

// =-=-=-=-=-=-=-
// STL Includes
#include <iostream>

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
		// =-=-=-=-=-=-=-
		// quick check on the resource table
		if( resources_.empty() ) {
			return ERROR( false, -1, "resource_manager::resolve_from_path - empty resource table" );
		}

		if( _key.empty() ) {
			return ERROR( false, -1, "resource_manager::resolve - empty key" );
		}

        if( resources_.has_entry( _key ) ) {
		    _value = resources_[ _key ];
			return SUCCESS();	
		} else {
            std::stringstream msg;
			msg << "resource_manager::resolve - no resource found for name ["
			    << _key << "]";
			return ERROR( false, -1, msg.str() );
		}
	} // resolve

	// =-=-=-=-=-=-=-
	// public - retrieve a resource given a vault path
    error resource_manager::resolve_from_path( std::string _path, resource_ptr& _resc ) {
        // =-=-=-=-=-=-=-
		// simple flag to state a resource matching the vault path is found 
	    bool found = false;	
		
		// =-=-=-=-=-=-=-
		// quick check on the resource table
		if( resources_.empty() ) {
			return ERROR( false, -1, "resource_manager::resolve_from_path - empty resource table" );
		}
		
		// =-=-=-=-=-=-=-
		// quick check on the path that it has something in it
		if( _path.empty() ) {
			return ERROR( false, -1, "resource_manager::resolve_from_path - empty path" );
		}

		// =-=-=-=-=-=-=-
		// iterate through the map and search for our path
        lookup_table< resource_ptr >::iterator itr = resources_.begin();
		for( ; itr != resources_.end(); ++itr ) {
			// =-=-=-=-=-=-=-
			// query resource for the vault value
            std::string vault;
			error ret = itr->second->get_property<std::string>( "path", vault );

			// =-=-=-=-=-=-=-
			// if we get a good parameter 
			if( ret.ok() ) {
			    // =-=-=-=-=-=-=-
			    // compare path and vault
                if( vault.find_first_of( _path ) != std::string::npos ) {
			        // =-=-=-=-=-=-=-
			        // if we geta match, cache the resource pointer
					// in the given out variable
					found = true;
                    _resc = itr->second; 
					break;
				}
			} else {
				eirods::error err = PASS( false, -1, 
				    "resource_manager::resolve_from_path - failed to get vault parameter from resource", ret );
                eirods::log( ret );
			}

		} // for itr

        // =-=-=-=-=-=-=-
		// did we find a resource and is the ptr valid?
        if( found = true && _resc.get() ) {
		    return SUCCESS();
		} else {
            std::string msg( "resource_manager::resolve_from_path - failed to find resource for path [" );
			msg += _path; 
			msg += "]";
			return ERROR( false, -1, msg );
        }

	} // resolve_from_path
 
	// =-=-=-=-=-=-=-
	// resolve a resource from a first_class_object
	error resource_manager::resolve( const eirods::first_class_object& _object, resource_ptr& _resc ) {
        return resolve_from_path( _object.physical_path(), _resc );
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

		addInxIval( &genQueryInp.selectInp, COL_R_RESC_ID,      1 );
		addInxIval( &genQueryInp.selectInp, COL_R_RESC_NAME,    1 );
		addInxIval( &genQueryInp.selectInp, COL_R_ZONE_NAME,    1 );
		addInxIval( &genQueryInp.selectInp, COL_R_TYPE_NAME,    1 );
		addInxIval( &genQueryInp.selectInp, COL_R_CLASS_NAME,   1 );
		addInxIval( &genQueryInp.selectInp, COL_R_LOC,          1 );
		addInxIval( &genQueryInp.selectInp, COL_R_VAULT_PATH,   1 );
		addInxIval( &genQueryInp.selectInp, COL_R_FREE_SPACE,   1 );
		addInxIval( &genQueryInp.selectInp, COL_R_RESC_INFO,    1 );
		addInxIval( &genQueryInp.selectInp, COL_R_RESC_COMMENT, 1 );
		addInxIval( &genQueryInp.selectInp, COL_R_CREATE_TIME,  1 );
		addInxIval( &genQueryInp.selectInp, COL_R_MODIFY_TIME,  1 );
		addInxIval( &genQueryInp.selectInp, COL_R_RESC_STATUS,  1 );

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
				return ERROR( false, status, "init_from_catalog - genqery failed." );
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
		// win!
		return SUCCESS();

    } // init_from_catalog

	// =-=-=-=-=-=-=-
	// public - take results from genQuery, extract values and create resources
    error resource_manager::process_init_results( genQueryOut_t* _result ) {
        // =-=-=-=-=-=-=-
		// extract results from query
		if ( !_result ) {
			return ERROR( false, -1, "resource ctor :: _result parameter is null" );
		}

        // =-=-=-=-=-=-=-
		// values to extract from query
		sqlResult_t *rescId = 0, *rescName = 0, *zoneName = 0, *rescType = 0, *rescClass = 0;
		sqlResult_t *rescLoc = 0, *rescVaultPath = 0, *freeSpace = 0, *rescInfo = 0;
		sqlResult_t *rescComments = 0, *rescCreate, *rescModify = 0, *rescStatus = 0;


        // =-=-=-=-=-=-=-
		// extract results from query
		if( ( rescId = getSqlResultByInx( _result, COL_R_RESC_ID ) ) == NULL ) {
            return ERROR( false, UNMATCHED_KEY_OR_INDEX,"resource ctor :: getSqlResultByInx for COL_R_RESC_ID failed" );
		}

		if( ( rescName = getSqlResultByInx( _result, COL_R_RESC_NAME ) ) == NULL ) {
            return ERROR( false, UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_RESC_NAME failed" );
		}

		if( ( zoneName = getSqlResultByInx( _result, COL_R_ZONE_NAME ) ) == NULL ) {
            return ERROR( false, UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_ZONE_NAME failed" );
		}
		
		if( ( rescType = getSqlResultByInx(_result, COL_R_TYPE_NAME ) ) == NULL ) {
            return ERROR( false, UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_TYPE_NAME failed" );
		}

		if( ( rescClass = getSqlResultByInx( _result,COL_R_CLASS_NAME ) ) == NULL ) {
            return ERROR( false, UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_CLASS_NAME failed" );
		}
		
		if( ( rescLoc = getSqlResultByInx( _result, COL_R_LOC ) ) == NULL ) {
            return ERROR( false, UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_LOC failed" );
		}

		if( ( rescVaultPath = getSqlResultByInx( _result, COL_R_VAULT_PATH ) ) == NULL ) {
            return ERROR( false, UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_VAULT_PATH failed" );
		}

		if( ( freeSpace = getSqlResultByInx( _result, COL_R_FREE_SPACE ) ) == NULL ) {
            return ERROR( false, UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_FREE_SPACE failed" );
		}

		if( ( rescInfo = getSqlResultByInx( _result, COL_R_RESC_INFO ) ) == NULL ) {
            return ERROR( false, UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_RESC_INFO failed" );
		}

		if( ( rescComments = getSqlResultByInx( _result, COL_R_RESC_COMMENT ) ) == NULL ) {
            return ERROR( false, UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_RESC_COMMENT failed" );
		}

		if( ( rescCreate = getSqlResultByInx( _result, COL_R_CREATE_TIME ) ) == NULL) {
            return ERROR( false, UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_CREATE_TIME failed" );
		}

		if( ( rescModify = getSqlResultByInx( _result, COL_R_MODIFY_TIME ) ) == NULL) {
            return ERROR( false, UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_MODIFY_TIME failed" );
		}

		if( ( rescStatus = getSqlResultByInx( _result, COL_R_RESC_STATUS ) ) == NULL) {
            return ERROR( false, UNMATCHED_KEY_OR_INDEX, "resource ctor: getSqlResultByInx for COL_R_RESC_STATUS failed" );
		}

        // =-=-=-=-=-=-=-
		// iterate through the rows, initialize a resource for each entry
		for( size_t i = 0; i < _result->rowCnt; ++i ) {

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

            // =-=-=-=-=-=-=-
			// resolve the host name into a rods server host structure
		    rodsHostAddr_t addr;
			rstrcpy( addr.hostAddr, const_cast<char*>( tmpRescLoc.c_str()  ), LONG_NAME_LEN );
			rstrcpy( addr.zoneName, const_cast<char*>( tmpZoneName.c_str() ), NAME_LEN );
		
		    rodsServerHost_t* tmpRodsServerHost = 0;
			if( resolveHost( &addr, &tmpRodsServerHost ) < 0 ) {
				rodsLog( LOG_NOTICE, "procAndQueRescResult: resolveHost error for %s",addr.hostAddr );
			}
            
			// =-=-=-=-=-=-=-
			// create the resource and add properties for column values
			resource_ptr resc;
			error ret = load_resource_plugin( resc, tmpRescType );
		    if( !ret.ok() ) {
			    return PASS( false, -1, "Failed to load Resource Plugin", ret );	
			}

			resc->set_property< boost::shared_ptr< rodsServerHost_t > >( 
			    "host", boost::shared_ptr< rodsServerHost_t >( tmpRodsServerHost ) );
            
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


}; // namespace eirods



