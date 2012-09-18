


#ifndef __EIRODS_LOAD_PLUGIN_H__
#define __EIRODS_LOAD_PLUGIN_H__

// =-=-=-=-=-=-=-
// My Includes
#include "eirods_error.h"

// =-=-=-=-=-=-=-
// STL Includes
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>

// =-=-=-=-=-=-=-
// Boost Includes
#include <boost/static_assert.hpp>

// =-=-=-=-=-=-=-
// dlopen, etc
#include <dlfcn.h>

namespace eirods {

	// =-=-=-=-=-=-=-
	// machinery using SFINAE to determine if PluginType supports delay_load
	// yes / no types
	typedef char small_type;
	struct large_type {
			small_type dummy[2];
	};

	template<typename T>
	struct class_has_delay_load {
	    // =-=-=-=-=-=-=-
		// tester to determine if delay load function exists - 
		// bool (*)( void* ) signature.
		template<error (T::*)(void*)> struct tester; 

	    // =-=-=-=-=-=-=-
		// matching specialization of the determination function
		template<typename U>
		static small_type has_matching_member( tester<&U::delay_load>* );
		
	    // =-=-=-=-=-=-=-
		// SFINAE fall through if it doesnt match
		template<typename U>
		static large_type has_matching_member( ... ); 

	    // =-=-=-=-=-=-=-
		// flag variable for result on which to assert - small_type == yes
		static const bool value = ( sizeof( has_matching_member<T>(0) ) == sizeof( small_type ) );

	}; // class_has_delay_load

    // =-=-=-=-=-=-=-
	// predicate to determine if a char is not alphanumeric
    static bool not_allowed_char( char _c ) {
        return ( !std::isalnum( _c ) && !( '_' == _c ) );
	} // not_allowed_char

	/**
	 * \fn PluginType* load_plugin( const std::string _plugin_name, const std::string _dir );
	 *
	 * \brief load a plug in object from a given shared object / dll name
	 *
	 * \user developer
	 *
	 * \category core facility
	 *
	 * \since E-iRODS 3.0
	 *
	 * \author  Jason M. Coposky
	 * \date    June 2012
	 *
	 * \usage
	 * ms_table_entry* tab_entry;\n
	 * tab_entry = load_plugin( "some_microservice_name", "/var/lib/e-irods/iRODS/server/bin" );
	 *
	 * \param[in] _dir         - hard coded string which will house the shared object to be loaded
	 * \param[in] _plugin_name - name of plugin you wish to load, which will have all nonalnum characters removed, as found in a file named "lib" + clean_plugin_name + ".so"
	 *
	 * \return PluginType*
	 * \retval non-null on success
	**/
	template< typename PluginType >
	error load_plugin(  const std::string _plugin_name, const std::string _dir, 
	                    PluginType*& _plugin, std::string _context  ) { 
	
        // =-=-=-=-=-=-=-
		// strip out all non alphanumeric characters like spaces or such
		std::string clean_plugin_name = _plugin_name;
        clean_plugin_name.erase( std::remove_if( clean_plugin_name.begin(), 
		                                         clean_plugin_name.end(), 
												 not_allowed_char ),
								 clean_plugin_name.end() );

        // =-=-=-=-=-=-=-
		// static assertion to determine if the PluginType supports the delay_load interface properly
        BOOST_STATIC_ASSERT( class_has_delay_load< PluginType >::value );

		// =-=-=-=-=-=-=-
		// quick parameter check
		if( clean_plugin_name.empty() ) {
			return ERROR( false, -1, "load_plugin :: clean_plugin_name is empty" );
		}
		
		// =-=-=-=-=-=-=-
		// try to open the shared object
		std::string so_name = _dir  + std::string("/lib") + clean_plugin_name + std::string(".so");
		void*  handle  = dlopen( so_name.c_str(), RTLD_LAZY );
		if( !handle ) {
			std::stringstream msg;
			std::string       err( dlerror() );
			msg << "load_plugin :: failed to open shared object file: " << so_name 
			    << " :: dlerror is " << err;
			return ERROR( false, -1, msg.str() );
		}

		// =-=-=-=-=-=-=-
		// clear any existing dlerrors
		dlerror();

		// =-=-=-=-=-=-=-
		// attempt to load the plugin version
		char* err = 0;
		double plugin_version = *static_cast< double* >( dlsym( handle, "EIRODS_PLUGIN_INTERFACE_VERSION" ) );
		if( ( err = dlerror() ) != 0 ) {
			std::stringstream msg;
			msg << "load_plugin :: failed to load sybol from shared object handle - " 
			    << "EIRODS_PLUGIN_VERSION" << " :: dlerror is " << err;
		    dlclose( handle );
	        return ERROR( false, -1, msg.str() );	
		}

		// =-=-=-=-=-=-=-
        // Here is where we decide how to load the plugins based on the version...
        if( 1.0 == plugin_version ) {
			// do something particularly interesting here
		} else {
			// do something else even more interesting here
		}

		// =-=-=-=-=-=-=-
		// attempt to load the plugin factory function from the shared object
		typedef PluginType* (*factory_type)( std::string );
		factory_type factory = reinterpret_cast< factory_type >( dlsym( handle, "plugin_factory" ) );
		if( ( err = dlerror() ) != 0 ) {
			std::stringstream msg;
			msg << "load_plugin :: failed to load sybol from shared object handle - plugin_factory"
			    << " :: dlerror is " << err;
		    dlclose( handle );
	        return ERROR( false, -1, msg.str() );	
		}

		if( !factory ) {
		    dlclose( handle );
			return ERROR(  false, -1, "load_plugin :: failed to cast plugin factory" );
		}

		// =-=-=-=-=-=-=-
		// using the factory pointer create the plugin
		_plugin = factory( _context );
		if( _plugin ) {
			// =-=-=-=-=-=-=-
			// notify world of success
			// TODO :: add hash checking and provide hash value for log also
			#ifdef DEBUG
			std::cout << "load_plugin :: loaded [" << clean_plugin_name << "]" << std::endl;
			#endif

			// =-=-=-=-=-=-=-
			// call the delayed loader to load any other symbols this plugin may need.
			error ret = _plugin->delay_load( handle );
			if( !ret.ok() ) {
				std::stringstream msg;
				msg << "load_plugin :: failed on delayed load for [" << clean_plugin_name << "]";
				dlclose( handle );
				return ERROR( false, -1, msg.str() );
			}
			
			//dlclose( handle );
			return SUCCESS();

		} else {
			std::stringstream msg;
			msg << "load_plugin :: failed to create plugin object for [" << clean_plugin_name << "]";
			dlclose( handle );
			return ERROR( false, -1, msg.str() );
		}

		// =-=-=-=-=-=-=-
		// the code should never get here
		dlclose( handle );
		return ERROR( false, -1, "load_plugin - this shouldnt happen." );

	} // load_plugin

}; // namespace eirods



#endif // __EIRODS_LOAD_PLUGIN_H__



