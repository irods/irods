


// =-=-=-=-=-=-=-
// My Includes
#include "eirods_resource_plugin.h"
#include "eirods_load_plugin.h"
#include "eirods_ms_home.h"

// =-=-=-=-=-=-=-
// STL Includes
#include <iostream>
#include <sstream>
#include <algorithm>

// =-=-=-=-=-=-=-
// dlopen, etc
#include <dlfcn.h>

namespace eirods {

    // =-=-=-=-=-=-=-
	// public - ctor
    resource::resource( std::string _ctx  ) : plugin_base( _ctx ), 
	    start_operation_( eirods::resource::default_start_operation ), 
		stop_operation_( default_stop_operation ) {
	} // ctor
    
	// =-=-=-=-=-=-=-
	// public - dtor
    resource::~resource(  ) {
		
	} // dtor
    
    // =-=-=-=-=-=-=-
	// public - cctor
	resource::resource( const resource& _rhs ) : 
	    plugin_base( _rhs.context_ ) {
		children_           = _rhs.children_;
		operations_         = _rhs.operations_;
        ops_for_delay_load_ = _rhs.ops_for_delay_load_;
		
		if( properties_.size() > 0 ) {
			std::cout << "[!]\tresource cctor - properties map is not empty." 
			          << __FILE__ << ":" << __LINE__ << std::endl;
		}
		properties_ = _rhs.properties_; // NOTE:: memory leak repaving old containers?
	} // cctor
    
    // =-=-=-=-=-=-=-
	// public - assignment
	resource& resource::operator=( const resource& _rhs ) {
		if( &_rhs == this ) {
			return *this;
		}

        plugin_base::operator=( _rhs );

		children_           = _rhs.children_;
		operations_         = _rhs.operations_;
        ops_for_delay_load_ = _rhs.ops_for_delay_load_;

		if( properties_.size() > 0 ) {
			std::cout << "[!]\tresource cctor - properties map is not empty." 
			          << __FILE__ << ":" << __LINE__ << std::endl;
		}
		properties_ = _rhs.properties_; // NOTE:: memory leak repaving old containers?

		return *this;
	} // operator=
    
	// =-=-=-=-=-=-=-
	// public - function which pulls all of the symbols out of the shared object and 
	//          associates them with their keys in the operations table
	error resource::delay_load( void* _handle ) {

        // =-=-=-=-=-=-=-
		// check params
		if( ! _handle ) {
		    return ERROR( -1, "delay_load - void handle pointer" );	
		}

        if( ops_for_delay_load_.empty() ) {
			return ERROR( -1, "delay_load - empty operations list" );
		}


		// =-=-=-=-=-=-=-
		// check if we need to load a start function
		if( !start_opr_name_.empty() ) {
		    dlerror();
			resource_maintenance_operation start_op = reinterpret_cast< resource_maintenance_operation >( 
			                                     dlsym( _handle, start_opr_name_.c_str() ) );
			if( !start_op ) {
                std::stringstream msg;
				msg  << "delay_load - failed to load start function [" 
				     << start_opr_name_ << "]";
				return ERROR( -1, msg.str() );
			} else {
               start_operation_ = start_op; 
			}
		} // if load start
 

		// =-=-=-=-=-=-=-
		// check if we need to load a stop function
		if( !stop_opr_name_.empty() ) {
		    dlerror();
			resource_maintenance_operation stop_op = reinterpret_cast< resource_maintenance_operation >( 
			                                    dlsym( _handle, stop_opr_name_.c_str() ) );
			if( !stop_op ) {
				std::stringstream msg;
                msg << "delay_load - failed to load stop function [" 
				          << stop_opr_name_ << "]";
				return ERROR( -1, msg.str() );
			} else {
               stop_operation_ = stop_op; 
			}
		} // if load stop


        // =-=-=-=-=-=-=-
		// iterate over list and load function. then add it to the map via wrapper functor
		vector< std::pair< std::string, std::string > >::iterator itr = ops_for_delay_load_.begin();
		for( ; itr != ops_for_delay_load_.end(); ++itr ) {
			// =-=-=-=-=-=-=-
			// cache values in obvious variables
			std::string& key = itr->first;
			std::string& fcn = itr->second; 
            
            // =-=-=-=-=-=-=-
			// check key and fcn name before trying to load
			if( key.empty() ) {
				std::cout << "[!]\teirods::resource::delay_load - empty op key for [" 
				          << fcn << "], skipping." << std::endl;
				continue;
			}

            // =-=-=-=-=-=-=-
			// check key and fcn name before trying to load
			if( fcn.empty() ) {
				std::cout << "[!]\teirods::resource::delay_load - empty function name for [" 
				          << key << "], skipping." << std::endl;
				continue;
			}

            // =-=-=-=-=-=-=-
			// call dlsym to load and check results
		    dlerror();
            resource_operation res_op_ptr = reinterpret_cast< resource_operation >( dlsym( _handle, fcn.c_str() ) );
            if( !res_op_ptr ) {
                std::cout << "[!]\teirods::resource::delay_load - failed to load [" 
				          << fcn << "].  error - " << dlerror() << std::endl;
                continue;
			}

            // =-=-=-=-=-=-=-
			// add the operation via a wrapper to the operation map
            operations_[ key ] = operation_wrapper( res_op_ptr );

        } // for itr


        // =-=-=-=-=-=-=-
		// see if we loaded anything at all
		if( operations_.size() < 0 ) {
			return ERROR( -1, "delay_load - operations map is emtpy" );
		}


		return SUCCESS();

	} // delay_load
               
	// =-=-=-=-=-=-=-
	// public - add a child resource to this resource.  this is virtual in case a developer wants to
	//          do something fancier.
	error resource::add_child( std::string _name, std::string _data, resource_ptr _resc ) {
		// =-=-=-=-=-=-=-
		// check params	
		if( _name.empty() ) {
            return ERROR( -1, "add_child - empty name" );
		}

		if( 0 == _resc.get() ) {
            return ERROR( -1, "add_child - null resource pointer" );
		}

		// =-=-=-=-=-=-=-
		// add resource and data to the table
        children_[ _name ] = std::pair< std::string, resource_ptr >( _data, _resc );

		return SUCCESS();

	} // add_child 

	// =-=-=-=-=-=-=-
	// public - remove a child resource to this resource.  this is virtual in case a developer wants to
	//          do something fancier.
	error resource::remove_child( std::string _name ) {
		// =-=-=-=-=-=-=-
		// check params	
	    #ifdef DEBUG
		if( _name.empty() ) {
            return ERROR( false, -1, "remove_child - empty name" );
		}
        #endif
        
		// =-=-=-=-=-=-=-
		// if an entry exists, erase it otherwise issue a warning.
        if( children_.has_entry( _name ) ) {
		    children_.erase( _name );
			return SUCCESS();
		} else {
			std::stringstream msg;
            msg << "remove_child - resource has no child named [" << _name << "]";
			return ERROR( -1, msg.str() );
		}
	
	} // remove_child 

	// =-=-=-=-=-=-=-
	// public - add an operation to the map given a key.  provide the function name such that it
	//          may be loaded from the shared object later via delay_load
	error resource::add_operation( std::string _op, std::string _fcn_name ) {
		// =-=-=-=-=-=-=-
		// check params	
		if( _op.empty() ) {
			std::stringstream msg;
			msg << "add_operation - empty operation [" << _op << "]";
			return ERROR( -1, msg.str() );
		}
		
		if( _fcn_name.empty() ) {
			std::stringstream msg;
			msg << ":add_operation - empty function name [" 
			          << _fcn_name << "]";
			return ERROR( -1, msg.str() );
		}

		// =-=-=-=-=-=-=-
		// add operation string to the vector
	    ops_for_delay_load_.push_back( std::pair< std::string, std::string >( _op, _fcn_name ) );
		
		return SUCCESS();

	} //  add_operation

	// =-=-=-=-=-=-=-
	// public - set a name for the developer provided start op
	void resource::set_start_operation( std::string _op ) {
        start_opr_name_ = _op;
	} // resource::set_start_operation

	// =-=-=-=-=-=-=-
	// public - set a name for the developer provided stop op
	void resource::set_stop_operation( std::string _op ) {
        stop_opr_name_ = _op;
	} // resource::set_stop_operation
 
	// =-=-=-=-=-=-=-
	// private - helper function to check params for the operator() calls to reduce code duplication
	error resource::check_operation_params( std::string _op ) {
		// =-=-=-=-=-=-=-
		// check params
        #ifdef DEBUG
		if( _op.empty() ) {
			return ERROR( false, -1, "check_operation_params - empty operation key" );
		}
        #endif

        // =-=-=-=-=-=-=-
		// check if the operation exists
        if( !operations_.has_entry( _op ) ) {
			std::stringstream msg;
			msg << "check_operation_params - operation [" << _op << "] doesn't exist.";
			return ERROR( -1, msg.str() );
		}
		
		return SUCCESS();

	} // check_operation_params

    // END resource
	// =-=-=-=-=-=-=-

	// =-=-=-=-=-=-=-
	// function to load and return an initialized resource plugin
	error load_resource_plugin( resource_ptr& _plugin, const std::string _name, const std::string _context ) {
		
		resource* resc = 0;
        error ret = load_plugin< resource >( _name, EIRODS_MS_HOME, resc, _context );

        if( ret.ok() && resc ) {
			_plugin.reset( resc );
	        return SUCCESS();    	
		} else {
            return PASS( false, -1, "load_resource_plugin failed.", ret );
		}
	} // load_resource_plugin



}; // namespace eirods



