// =-=-=-=-=-=-=-
// My Includes
#include "eirods_ms_home.h"
#include "eirods_ms_plugin.h"
#include "eirods_load_plugin.h"
#include "eirods_log.h"

// =-=-=-=-=-=-=-
// STL Includes
#include <iostream>
#include <sstream>

namespace eirods {
	// =-=-=-=-=-=-=-
	// ms_table_entry definition
	ms_table_entry::ms_table_entry( ) : 
	    plugin_base( "" ),
		action_(""), 
		numberOfStringArgs_( 0 ), 
		callAction_( 0 ) { 
	} // def ctor

	ms_table_entry::ms_table_entry( std::string _s, int _n, ms_func_ptr _fp ) :
	    plugin_base(""),
		action_(_s), 
		numberOfStringArgs_( _n ), 
		callAction_( _fp ) {
	} // ctor

	ms_table_entry::ms_table_entry( const ms_table_entry& _rhs ) :
	    plugin_base( _rhs.context_ ),
		action_( _rhs.action_ ), 
		numberOfStringArgs_( _rhs.numberOfStringArgs_ ), 
		callAction_( _rhs.callAction_ ) {
	} // cctor

	ms_table_entry& ms_table_entry::operator=( const ms_table_entry& _rhs ) { 
		plugin_base::operator=( _rhs );
		action_             = _rhs.action_;
		numberOfStringArgs_ = _rhs.numberOfStringArgs_;
		callAction_         = _rhs.callAction_;
		return *this;
	} // operator=

	ms_table_entry::~ms_table_entry() {
	} // dtor

	error ms_table_entry::delay_load( void* _h ) {
		if( !_h ) {
			return ERROR( false, -1, "delay_load - null handle parameter" );
		}

		callAction_ = reinterpret_cast< ms_func_ptr >( dlsym( _h, action_.c_str() ) );
		
		if( !callAction_ ) {
			std::stringstream msg;
			msg << "delay_load :: failed to load msvc function [" << action_ << "]";
			return ERROR( false, -1, msg.str() );
		} else { 
			#ifdef DEBUG
			std::cout << "[+]\teirods::ms_table_entry::delay_load :: [" << action_ << "]" << std::endl;
			#endif
		}

		return SUCCESS();
	} // delay_load

	// =-=-=-=-=-=-=-
	// given the name of a microservice, try to load the shared object
	// and then register that ms with the table
	error load_microservice_plugin( ms_table& _table, const std::string _ms ) {

        ms_table_entry* entry;
		error load_err = load_plugin< ms_table_entry >( _ms, EIRODS_MS_HOME, entry, "" );
        if( load_err.ok() && entry ) {
            _table[ _ms ] = entry;
		    return SUCCESS();
		} else {
			error ret = PASS( false, -1, "load_microservice_plugin - Failed to create ms plugin entry.", load_err );
			// JMC :: spams the log - log( ret );
			return ret;
		}
	} // load_microservice_plugin

}; // namespace eirods
