// =-=-=-=-=-=-=-
// My Includes
#include "eirods_ms_plugin.h"

namespace eirods {

	// =-=-=-=-=-=-=-
	// given the name of a microservice, try to load the shared object
	// and then register that ms with the table
	bool load_microservice_plugin( ms_table& _table, const std::string _ms ) {
		using namespace std;

		if( _ms.empty() ) {
			cout << "load_microservice_plugin :: empty microservice name parameter" << endl;
			return false;
		}

		// =-=-=-=-=-=-=-
		// does that MS happen to be in the table already?
		if( _table.has_entry( _ms ) )  {
			cout << "load_microservice_plugin :: microservice already exists.  " << _ms << endl;
			// return true as this ms can be used, it was just already loaded....
			return true;
		}

		// =-=-=-=-=-=-=-
		// if not then try to load the shared object
		string so_name = EIRODS_MS_HOME + string("/lib") + _ms + string(".so");
		void*  handle  = dlopen( so_name.c_str(), RTLD_LAZY );

		if( !handle ) {
			cout << "load_microservice_plugin :: failed to load microservice plugin: " << so_name << endl;
			cout << "                         :: dlerror is " << dlerror() << endl;
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
			cout << "load_microservice_plugin :: failed to load sybol from shared object handle - " 
			     << "EIRODS_PLUGIN_VERSION" << endl;
			cout << "                         :: dlerror is " << err << endl;
		}

		// =-=-=-=-=-=-=-
        // Here is where we decide how to load the plugins based on the version...

		// =-=-=-=-=-=-=-
		// attempt to load the microservice symbol from the handle
		funcPtr fcn = reinterpret_cast< funcPtr >( dlsym( handle, _ms.c_str() ) );

		if( ( err = dlerror() ) != 0 ) {
			cout << "load_microservice_plugin :: failed to load sybol from shared object handle - " << _ms << endl;
			cout << "                         :: dlerror is " << err << endl;
		}

		// =-=-=-=-=-=-=-
		// attempt to load the number of parameters symbol from the handle
		int num_param = *static_cast< int* >( dlsym( handle, "NUMBER_OF_PARAMETERS" ) );

		if( ( err = dlerror() ) != 0 ) {
			cout << "load_microservice_plugin :: failed to load symbol from shared object handle - " 
			     << "NUMBER_OF_PARAMETERS" << endl;
			cout << "                         :: dlerror is " << err << endl;
		}

		// =-=-=-=-=-=-=-
		// notify world of loading a microservice
		// TODO :: add hash checking and provide hash value for log also
		cout << "load_microservice_plugin :: loaded " << _ms << " with " << num_param << " parameters." << endl;

		// =-=-=-=-=-=-=-
		// add the new objects to the table via a new entry
		_table[ _ms ] = ms_table_entry( _ms, num_param, fcn );

		// =-=-=-=-=-=-=-
		// and... were done.
		return true;

	} // load_microservice_plugin

}; // namespace eirods
