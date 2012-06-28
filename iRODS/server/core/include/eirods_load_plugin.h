


#ifndef __EIRODS_LOAD_PLUGIN_H__
#define __EIRODS_LOAD_PLUGIN_H__

// =-=-=-=-=-=-=-
// STL Includes
#include <string>
#include <iostream>

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
		template<bool (T::*)(void*)> struct tester; 

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
	 * \param[in] _plugin_name - name of plugin you wish to load, as found in a file named "lib" + _plugin_name + ".so"
	 *
	 * \return PluginType*
	 * \retval non-null on success
	**/
	template< typename PluginType >
	PluginType* load_plugin(  const std::string _plugin_name, const std::string _dir  ) { 
		using namespace std;

        // =-=-=-=-=-=-=-
		// static assertion to determine if the PluginType supports the delay_load interface properly
        BOOST_STATIC_ASSERT( class_has_delay_load< PluginType >::value );

		// =-=-=-=-=-=-=-
		// quick parameter check
		if( _plugin_name.empty() ) {
			cout << "load_plugin :: empty _plugin_name parameter" << endl;
			return 0;
		}
		
		// =-=-=-=-=-=-=-
		// try to open the shared object
		string so_name = _dir  + string("/lib") + _plugin_name + string(".so");
		void*  handle  = dlopen( so_name.c_str(), RTLD_LAZY );
		if( !handle ) {
			cout << "load_plugin :: failed to open shared object file: " << so_name << endl;
			cout << "            :: dlerror is " << dlerror() << endl;
		    dlclose( handle );
			return 0;
		}

		// =-=-=-=-=-=-=-
		// clear any existing dlerrors
		dlerror();

		// =-=-=-=-=-=-=-
		// attempt to load the plugin version
		char* err = 0;
		int plugin_version = *static_cast< int* >( dlsym( handle, "EIRODS_PLUGIN_VERSION" ) );
		if( ( err = dlerror() ) != 0 ) {
			cout << "load_plugin :: failed to load sybol from shared object handle - " 
			     << "EIRODS_PLUGIN_VERSION" << endl;
			cout << "            :: dlerror is " << err << endl;
		    dlclose( handle );
	        return 0;	
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
		typedef PluginType* (*factory_type)(  );
		factory_type factory = reinterpret_cast< factory_type >( dlsym( handle, "plugin_factory" ) );
		if( ( err = dlerror() ) != 0 ) {
			cout << "load_plugin :: failed to load sybol from shared object handle - plugin_factory" << endl;
			cout << "            :: dlerror is " << err << endl;
		    dlclose( handle );
	        return 0;	
		}

		if( !factory ) {
			cout << "load_plugin :: failed to cast plugin factor" << endl;
		    dlclose( handle );
			return 0;
		}

		// =-=-=-=-=-=-=-
		// using the factory pointer create the plugin
		PluginType* plugin = factory();
		if( plugin ) {
			// =-=-=-=-=-=-=-
			// notify world of success
			// TODO :: add hash checking and provide hash value for log also
			#ifdef DEBUG
			cout << "load_plugin :: loaded " << _plugin_name << endl;
			#endif

			// =-=-=-=-=-=-=-
			// call the delayed loader to load any other symbols this plugin may need.
			if( !plugin->delay_load( handle ) ) {
				cout << "load_plugin :: failed on delayed load for [" << _plugin_name << "]" << endl;
				dlclose( handle );
				return 0;
			}
			
			return plugin;

		} else {
			cout << "load_plugin :: failed to create plugin object for " << _plugin_name << endl;
			dlclose( handle );
			return 0;
		}

		// =-=-=-=-=-=-=-
		// the code should never get here
		cout << "load_plugin :: ERROR - this shouldnt happen.  " << __FILE__ << ":" << __LINE__ << endl;
		dlclose( handle );
		return 0;

	} // load_plugin



}; // namespace eirods



#endif // __EIRODS_LOAD_PLUGIN_H__



