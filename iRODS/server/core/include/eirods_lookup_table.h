
#ifndef EIRODS_PLUGIN_TABLE_H
#define EIRODS_PLUGIN_TABLE_H

// =-=-=-=-=-=-=-
// My Includes
#include "eirods_hash.h"

namespace eirods {

	// =-=-=-=-=-=-=-
	// class to manage tables of plugins.  employing a class in order to use
	// RAII for adding entries to the table now that it is not a static array
	template< typename ValueType, typename KeyType=std::string, typename HashType=eirods_string_hash >
	class lookup_table {
		protected:
			hash_map< KeyType, ValueType, HashType > table_;
			
		public:
			lookup_table(){};
			virtual ~lookup_table() {}
			ValueType& operator[]( KeyType _k ) {
				return table_[ _k ];
			}
			int size() {
				return table_.size();
			}
			bool has_entry( KeyType _k ) {
				return !( table_.end() == table_.find( _k ) );
			}
	}; // class lookup_table
	
};


#endif // EIRODS_PLUGIN_TABLE_H



