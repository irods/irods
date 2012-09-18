
#ifndef EIRODS_PLUGIN_TABLE_H
#define EIRODS_PLUGIN_TABLE_H

// =-=-=-=-=-=-=-
// boost includes
#include <boost/any.hpp>

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_hash.h"
#include "eirods_error.h"

namespace eirods {

	// =-=-=-=-=-=-=-
	// class to manage tables of plugins.  employing a class in order to use
	// RAII for adding entries to the table now that it is not a static array
	template< typename ValueType, typename KeyType=std::string, typename HashType=eirods_string_hash >
	class lookup_table {
		protected:
			hash_map< KeyType, ValueType, HashType > table_;
			
		public:
            typedef typename hash_map< KeyType, ValueType, HashType >::iterator iterator;
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
			size_t erase( KeyType _k ) {
		        return table_.erase( _k );	
			}
            void clear() {
				table_.clear();
			}
			bool empty() {
				return table_.empty();
			}
			iterator begin() { return table_.begin(); }
			iterator end()   { return table_.end();   }
 
			// =-=-=-=-=-=-=-
			// accessor function
            error get( std::string _key, ValueType& _val ) {
				if( !has_entry( _key ) ) {
                    return ERROR( false, -1, "key not found" );
				}

				_val = table_[ _key ];

				return SUCCESS();
			}
            
			// =-=-=-=-=-=-=-
			// mutator function 
			error set( std::string _key, const ValueType& _val ) {
                table_[ _key ] = _val;
			} // set

	}; // class lookup_table


	// =-=-=-=-=-=-=-
	// partial specialization created to support templating the get/set
	// functions which need to manage exception handling etc from 
	// a boost::any_cast
    template< typename KeyType, typename HashType >
	class lookup_table < boost::any, KeyType, HashType > {
		protected:
			hash_map< KeyType, boost::any, HashType > table_;
			
		public:
            typedef typename hash_map< KeyType, boost::any, HashType >::iterator iterator;
			lookup_table(){};
			virtual ~lookup_table() {}
			boost::any& operator[]( KeyType _k ) {
				return table_[ _k ];
			}
			int size() {
				return table_.size();
			}
			bool has_entry( KeyType _k ) {
				return !( table_.end() == table_.find( _k ) );
			}
			size_t erase( KeyType _k ) {
		        return table_.erase( _k );	
			}
            void clear() {
				table_.clear();
			}
			bool empty() {
				return table_.empty();
			}
			iterator begin() { return table_.begin(); }
			iterator end()   { return table_.end();   }
 
			// =-=-=-=-=-=-=-
			// get a property from the table if it exists.  catch the exception in the case where
			// the template types may not match and return sucess/fail
			template< typename T >
			error get( std::string _key, T& _val ) {
				// =-=-=-=-=-=-=-
				// check params
				#ifdef DEBUG
				if( _key.empty() ) {
					return ERROR( false, -1, "empty key" );
				}
				#endif
				
				// =-=-=-=-=-=-=-
				// attempt to any_cast property value to given type.  catch exception and log
				// failure then exit
				try {
					_val = boost::any_cast< T >( table_[ _key ] );
					return SUCCESS();
				} catch ( const boost::bad_any_cast & ) {
					return ERROR( false, -1, "lookup_table::get - type and property key mistmatch" );
				}
		 
				return ERROR( false, -1, "lookup_table::get - shouldn't get here." );

			} // get_property

			// =-=-=-=-=-=-=-
			// set a property in the table
			template< typename T >
			error set( std::string _key, const T& _val ) {
				// =-=-=-=-=-=-=-
				// check params	
				#ifdef DEBUG
				if( _key.empty() ) {
					return ERROR( false, -1, "lookup_table::set - empty key" );
				}
				
				if( table_.has_entry( _key ) ) {
					std::cout << "[!]\tlookup_table::set - overwriting entry for key [" 
							  << key << "]" << std::endl;
				}
				#endif	
			
				// =-=-=-=-=-=-=-
				// add property to map
				table_[ _key ] = _val;
					
				return SUCCESS() ;

			} // set_property
            
	}; // class lookup_table
	
}; // namepsace eirods

#endif // EIRODS_PLUGIN_TABLE_H



