// =-=-=-=-=-=-=-
// My Includes
#include "eirods_ms_home.h"
#include "eirods_ms_plugin.h"
#include "eirods_load_plugin.h"

// =-=-=-=-=-=-=-
// STL Includes
#include <iostream>

namespace eirods {
	// =-=-=-=-=-=-=-
	// ms_table_entry definition
	ms_table_entry::ms_table_entry( ) : 
		action_(""), 
		numberOfStringArgs_( 0 ), 
		callAction_( 0 ) { 
	} // def ctor

	ms_table_entry::ms_table_entry( std::string _s, int _n, ms_func_ptr _fp ) :
		action_(_s), 
		numberOfStringArgs_( _n ), 
		callAction_( _fp ) {
	} // ctor

	ms_table_entry::ms_table_entry( const ms_table_entry& _rhs ) :
		action_( _rhs.action_ ), 
		numberOfStringArgs_( _rhs.numberOfStringArgs_ ), 
		callAction_( _rhs.callAction_ ) {
	} // cctor

	ms_table_entry& ms_table_entry::operator=( const ms_table_entry& _rhs ) { 
		action_             = _rhs.action_;
		numberOfStringArgs_ = _rhs.numberOfStringArgs_;
		callAction_         = _rhs.callAction_;
		return *this;
	} // operator=

	ms_table_entry::~ms_table_entry() {
	} // dtor

	bool ms_table_entry::delay_load( void* _h ) {
		if( !_h ) {
			return false;
		}

		callAction_ = reinterpret_cast< ms_func_ptr >( dlsym( _h, action_.c_str() ) );
		
		if( !callAction_ ) {
			std::cout << "delay_load :: failed to load msvc function [" << action_ << "]" << std::endl;
			return false;
		} else { 
			#ifdef DEBUG
			std::cout << "delay_load :: [" << action_ << "]" << std::endl;
			#endif
		}

		return true;
	} // delay_load

	// =-=-=-=-=-=-=-
	// given the name of a microservice, try to load the shared object
	// and then register that ms with the table
	bool load_microservice_plugin( ms_table& _table, const std::string _ms ) {

        ms_table_entry* entry = load_plugin< ms_table_entry >( _ms, EIRODS_MS_HOME );
        if( entry ) {
            _table[ _ms ] = entry;
		    return true;
		} else {
			std::cout << "load_microservice_plugin - Failed to create ms plugin entry." << std::endl;
			return false;
		}
	} // load_microservice_plugin

}; // namespace eirods
