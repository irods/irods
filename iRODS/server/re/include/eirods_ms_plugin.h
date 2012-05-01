
#ifndef __EIRODS_MS_PLUGIN_H__
#define __EIRODS_MS_PLUGIN_H__

// =-=-=-=-=-=-=-
// STL Includes
#include <unordered_map>
#include <string>
#include <iostream>
using namespace std;

// =-=-=-=-=-=-=-
// dlopen, etc
#include <dlfcn.h>

// =-=-=-=-=-=-=-
// My Includes
#include "eirods_ms_home.h"

namespace eirods {

	// =-=-=-=-=-=-=-
	// MicroService Table Entry - holds fcn call name, number of args for fcn and fcn pointer
	typedef int((*funcPtr)(...));
	struct ms_table_entry {

		string  action;
		int     numberOfStringArgs;
		funcPtr callAction;

		// =-=-=-=-=-=-=-
		// Constructors 
		ms_table_entry( ) : 
			action(""), 
			numberOfStringArgs( 0 ), 
			callAction( nullptr ) { 
		} // def ctor

		ms_table_entry( string _s, int _n, funcPtr _fp ) : 
			action(_s), 
			numberOfStringArgs( _n ), 
			callAction( _fp ) {
		} // ctor

		ms_table_entry( const ms_table_entry& _rhs ) :
			action(_rhs.action ), 
			numberOfStringArgs( _rhs.numberOfStringArgs ), 
			callAction( _rhs.callAction ) {
		} // cctor

		// =-=-=-=-=-=-=-
		// Assignment Operator - necessary for stl containers
		ms_table_entry& operator=( const ms_table_entry& _rhs ) { 
			action             = _rhs.action;
			numberOfStringArgs = _rhs.numberOfStringArgs;
			callAction         = _rhs.callAction;
			return *this;
		} // operator=

		// =-=-=-=-=-=-=-
		// Destructor
		~ms_table_entry() {
		} // dtor

	}; // ms_table_entry

	typedef unordered_map< string, ms_table_entry > ms_table;

	// =-=-=-=-=-=-=-
	// given the name of a microservice, try to load the shared object
	// and then register that ms with the table
	bool load_microservice_plugin( ms_table& _table, const string _ms );

}; // namespace eirods

#endif // __EIRODS_MS_PLUGIN_H__

