#ifndef PLUGIN_TABLE_HPP
#define PLUGIN_TABLE_HPP

#include "rodsErrorTable.h"
#include "irods_hash.hpp"
#include "irods_error.hpp"
#include "irods_stacktrace.hpp"
#include "rodsLog.h"

#include <boost/unordered_map.hpp>
#include <boost/any.hpp>

namespace irods {

    // =-=-=-=-=-=-=-
    // class to manage tables of plugins.  employing a class in order to use
    // RAII for adding entries to the table now that it is not a static array
    template < typename ValueType,
               typename KeyType = std::string,
               typename HashType = irods_string_hash >
    class lookup_table {
        protected:
            using irods_hash_map = boost::unordered_map<KeyType, ValueType, HashType>;

            irods_hash_map table_;

        public:
            using iterator       = typename irods_hash_map::iterator;
            using const_iterator = typename irods_hash_map::const_iterator;

            lookup_table() {};

            virtual ~lookup_table() {}

            ValueType& operator[](const KeyType& _k) {
                return table_[ _k ];
            }

            int size() const noexcept {
                return table_.size();
            }

            bool has_entry(const KeyType& _k) const {
                return table_.end() != table_.find( _k );
            }

            size_t erase(const KeyType& _k) {
                return table_.erase( _k );
            }

            void clear() {
                table_.clear();
            }

            bool empty() const noexcept {
                return table_.empty();
            }

            iterator begin() {
                return table_.begin();
            }

            iterator end() {
                return table_.end();
            }

            const_iterator cbegin() const {
                return table_.cbegin();
            }

            const_iterator cend() const {
                return table_.cend();
            }

            iterator find(const KeyType& _k) {
                return table_.find( _k );
            }

            const_iterator find(const KeyType& _k) const {
                return table_.find( _k );
            }

            error get(const std::string& _key, ValueType& _val) const
            {
                if (auto iter = table_.find(_key); iter != table_.end()) {
                    _val = iter->second;
                    return SUCCESS();
                }

                return ERROR(KEY_NOT_FOUND, "key not found");
            }

            error set(const std::string& _key, const ValueType& _val) {
                table_[_key] = _val;
                return SUCCESS();
            }
    }; // class lookup_table

    // =-=-=-=-=-=-=-
    // partial specialization created to support templating the get/set
    // functions which need to manage exception handling etc from
    // a boost::any_cast
    template< typename KeyType, typename HashType >
    class lookup_table<boost::any, KeyType, HashType> {
        protected:
            using irods_hash_map = boost::unordered_map<KeyType, boost::any, HashType>;

            irods_hash_map table_;

        public:
            using iterator       = typename irods_hash_map::iterator;
            using const_iterator = typename irods_hash_map::const_iterator;

            lookup_table() {};

            virtual ~lookup_table() {}

            boost::any& operator[]( const KeyType& _k ) {
                return table_[_k];
            }

            int size() const noexcept {
                return table_.size();
            }

            bool has_entry(const KeyType& _k) const {
                return table_.end() != table_.find( _k );
            }

            size_t erase(const KeyType& _k) {
                return table_.erase( _k );
            }

            void clear() {
                table_.clear();
            }

            bool empty() const noexcept {
                return table_.empty();
            }

            iterator begin() {
                return table_.begin();
            }

            iterator end() {
                return table_.end();
            }

            const_iterator cbegin() const {
                return table_.cbegin();
            }

            const_iterator cend() const {
                return table_.cend();
            }

            iterator find(const KeyType& _k) {
                return table_.find( _k );
            }

            const_iterator find(const KeyType& _k) const {
                return table_.find( _k );
            }

            // =-=-=-=-=-=-=-
            // get a property from the table if it exists.  catch the exception in the case where
            // the template types may not match and return success/fail
            template< typename T >
            error get(const std::string& _key, T& _val) {
                // =-=-=-=-=-=-=-
                // check params
                if (_key.empty()) {
                    return ERROR( KEY_NOT_FOUND, "the key is empty" );
                }

                if ( !has_entry( _key ) ) {
                    std::stringstream msg;
                    msg << "failed to find key [";
                    msg << _key;
                    msg << "] in table.";
                    return ERROR( KEY_NOT_FOUND, msg.str() );
                }

                // =-=-=-=-=-=-=-
                // attempt to any_cast property value to given type.  catch exception and log
                // failure then exit
                try {
                    _val = boost::any_cast< T >( table_[ _key ] );
                    return SUCCESS();
                }
                catch ( const boost::bad_any_cast& ) {
                    std::stringstream msg;
                    msg << "type and property key [";
                    msg << _key;
                    msg << "] mismatch";
                    return ERROR( KEY_TYPE_MISMATCH, msg.str() );
                }

                // =-=-=-=-=-=-=-
                // invalid location in the code
                return ERROR( INVALID_LOCATION, "shouldn't get here." );
            } // get_property

            // =-=-=-=-=-=-=-
            // set a property in the table
            template< typename T >
            error set( const std::string& _key, const T& _val ) {
                // =-=-=-=-=-=-=-
                // check params
                if ( _key.empty() ) {
                    return ERROR( KEY_NOT_FOUND, "empty key" );
                }

                // =-=-=-=-=-=-=-
                // add property to map
                table_[ _key ] = _val;

                return SUCCESS() ;
            } // set_property
    }; // class lookup_table
    
    using plugin_property_map = lookup_table<boost::any>;
}; // namespace irods

#endif // PLUGIN_TABLE_HPP



