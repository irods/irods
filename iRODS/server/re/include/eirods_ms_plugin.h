
#ifndef __EIRODS_MS_PLUGIN_H__
#define __EIRODS_MS_PLUGIN_H__

// =-=-=-=-=-=-=-
// STL Includes
#include <string>

// =-=-=-=-=-=-=-
// dlopen, etc
#include <dlfcn.h>

// =-=-=-=-=-=-=-
// My Includes
#include "eirods_lookup_table.h"
#include "eirods_plugin_base.h"

namespace eirods {

	// =-=-=-=-=-=-=-
	/**
	  * \class ms_table_entry - holds fcn call name, number of args for fcn and fcn pointer
	  * \author Jason M. Coposky 
	  * \date   June 2012
	  * \brief  This is to be used by a microservice developer to provide a dynamic plugin to the microservice table found in server/re/include/reActions.h.  Reference server/re/src/rules.c for loading and server/re/src/arithemetic.c for invokation.
	  * 
	  **/
	class ms_table_entry : public plugin_base {
        public:
		 
		typedef int (*ms_func_ptr)( ... ); 

        // =-=-=-=-=-=-=-
		// Attributes
		std::string  action_;
		int          numberOfStringArgs_;
		ms_func_ptr  callAction_;

		// =-=-=-=-=-=-=-
		// Constructors
		ms_table_entry( );
		ms_table_entry( std::string _s, int _n, ms_func_ptr _fp = 0 );

        // =-=-=-=-=-=-=-
		// copy ctor
		ms_table_entry( const ms_table_entry& _rhs ); 

		// =-=-=-=-=-=-=-
		// Assignment Operator - necessary for stl containers
		ms_table_entry& operator=( const ms_table_entry& _rhs );

		// =-=-=-=-=-=-=-
		// Destructor
		virtual ~ms_table_entry();

		// =-=-=-=-=-=-=-
		// Lazy Loader for MS Fcn Ptr
		error delay_load( void* _h );

	}; // class ms_table_entry

	// =-=-=-=-=-=-=-
    // create a lookup table for ms_table_entry value type
	typedef lookup_table<ms_table_entry*> ms_table;	
	
	// =-=-=-=-=-=-=-
	// given the name of a microservice, try to load the shared object
	// and then register that ms with the table
	error load_microservice_plugin( ms_table& _table, const std::string _ms );


}; // namespace eirods

#endif // __EIRODS_MS_PLUGIN_H__

