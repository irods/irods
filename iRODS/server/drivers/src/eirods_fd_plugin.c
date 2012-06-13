
// =-=-=-=-=-=-=-
// STL Includes
#include <iostream>

// =-=-=-=-=-=-=-
// My Includes
#include "eirods_fd_plugin.h"

namespace eirods {

	// =-=-=-=-=-=-=-
	// given the name of a filedriver, try to load the shared object
	// and then register that ms with the table
	bool load_filedriver_plugin( fd_table& _table, const std::string _fd ) {
		using namespace std;

        typedef fileDriver_t (*fac_ptr)();

		if( _fd.empty() ) {
			cout << "load_filedriver_plugin :: empty filedriver name parameter" << endl;
			return false;
		}

		// =-=-=-=-=-=-=-
		// does that FD happen to be in the table already?
		if( _table.has_entry( _fd ) )  {
			cout << "load_filedriver_plugin :: filedriver already exists.  " << _fd << endl;
			// return true as this ms can be used, it was just already loaded....
			return true;
		}

		// =-=-=-=-=-=-=-
		// if not then try to load the shared object
		string so_name = EIRODS_FD_HOME + string("/lib") + _fd + string(".so");
		void*  handle  = dlopen( so_name.c_str(), RTLD_LAZY );

		if( !handle ) {
			cout << "load_filedriver_plugin :: failed to load filedriver plugin: " << so_name << endl;
			cout << "                       :: dlerror is " << dlerror() << endl;
			return false;
		}

		// =-=-=-=-=-=-=-
		// clear any existing dlerrors
		dlerror();

		// =-=-=-=-=-=-=-
		// attempt to load the plugin version m
		int plugin_version = *static_cast< int* >( dlsym( handle, "EIRODS_PLUGIN_VERSION" ) );

		char* err = 0;	
		if( ( err = dlerror() ) != 0 ) {
			cout << "load_filedriver_plugin :: failed to load sybol from shared object handle - " 
			     << "EIRODS_PLUGIN_VERSION" << endl;
			cout << "                       :: dlerror is " << err << endl;
		}

		// =-=-=-=-=-=-=-
        // Here is where we decide how to load the plugins based on the version...

		// =-=-=-=-=-=-=-
		// attempt to load the filedriver symbol from the handle
		fac_ptr fcn = reinterpret_cast< funcPtr >( dlsym( handle, "fd_factory" ) );

		if( ( err = dlerror() ) != 0 ) {
			cout << "load_filedriver_plugin :: failed to load sybol from shared object handle - " << _fd << endl;
			cout << "                       :: dlerror is " << err << endl;
		}

		// =-=-=-=-=-=-=-
		// using the factory pointer crate the filedriver table
        fileDriver_t drv = fac_ptr();

		// =-=-=-=-=-=-=-
		// should i test the driver struct somehow here?

		// =-=-=-=-=-=-=-
		// notify world of loading a filedriver
		// TODO :: add hash checking and provide hash value for log also
		cout << "load_filedriver_plugin :: loaded " << _fd << " with " << num_param << " parameters." << endl;

		// =-=-=-=-=-=-=-
		// add the new objects to the table via a new entry
		_table[ _fd ] = drv;

		// =-=-=-=-=-=-=-
		// and... were done.
		return true;

	} // load_filedriver_plugin

}; // namespace eirods
