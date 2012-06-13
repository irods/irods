
#ifndef EIRODS_PLUGIN_TABLE_H
#define EIRODS_PLUGIN_TABLE_H

// =-=-=-=-=-=-=-
// My Includes
#include "eirods_hash.h"

namespace eirods {

	// =-=-=-=-=-=-=-
	// class to manage tables of plugins.  employing a class in order to use
	// RAII for adding entries to the table now that it is not a static array
	template< typename ValueType >
	class lookup_table {
		private:
			hash_map< std::string, ValueType, eirods_string_hash > table_;
			
		public:
			lookup_table();
			virtual ~lookup_table() {}
			ValueType& operator[]( std::string _s ) {
				return table_[ _s ];
			}
			int size() {
				return table_.size();
			}
			bool has_entry( std::string _s ) {
				return !( table_.end() == table_.find( _s ) );
			}
	}; // class lookup_table
		
};


#endif // EIRODS_PLUGIN_TABLE_H



