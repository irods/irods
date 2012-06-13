
#ifndef __EIRODS_MS_PLUGIN_H__
#define __EIRODS_MS_PLUGIN_H__

// =-=-=-=-=-=-=-
// STL Includes
#include <string>
#include <iostream>

// =-=-=-=-=-=-=-
// dlopen, etc
#include <dlfcn.h>

// =-=-=-=-=-=-=-
// My Includes
#include "eirods_ms_home.h"
#include "eirods_lookup_table.h"

namespace eirods {

	// =-=-=-=-=-=-=-
	// MicroService Table Entry - holds fcn call name, number of args for fcn and fcn pointer
	typedef int((*funcPtr)(...));
	struct ms_table_entry {

		std::string  action_;
		int          numberOfStringArgs_;
		funcPtr      callAction_;

		// =-=-=-=-=-=-=-
		// Constructors 
		ms_table_entry( ) : 
			action_(""), 
			numberOfStringArgs_( 0 ), 
			callAction_( 0 ) { 
		} // def ctor

		ms_table_entry( std::string _s, int _n, funcPtr _fp ) : 
			action_(_s), 
			numberOfStringArgs_( _n ), 
			callAction_( _fp ) {
		} // ctor

		ms_table_entry( const ms_table_entry& _rhs ) :
			action_(_rhs.action_ ), 
			numberOfStringArgs_( _rhs.numberOfStringArgs_ ), 
			callAction_( _rhs.callAction_ ) {
		} // cctor

		// =-=-=-=-=-=-=-
		// Assignment Operator - necessary for stl containers
		ms_table_entry& operator=( const ms_table_entry& _rhs ) { 
			action_             = _rhs.action_;
			numberOfStringArgs_ = _rhs.numberOfStringArgs_;
			callAction_         = _rhs.callAction_;
			return *this;
		} // operator=

		// =-=-=-=-=-=-=-
		// Destructor
		~ms_table_entry() {
		} // dtor

	}; // ms_table_entry

	// =-=-=-=-=-=-=-
    // create a lookup table for ms_table_entry value type
	typedef lookup_table<ms_table_entry> ms_table;	
	
	// =-=-=-=-=-=-=-
	// given the name of a microservice, try to load the shared object
	// and then register that ms with the table
	bool load_microservice_plugin( ms_table& _table, const std::string _ms );


}; // namespace eirods

#endif // __EIRODS_MS_PLUGIN_H__

