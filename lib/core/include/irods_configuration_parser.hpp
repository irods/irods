#ifndef CONFIGURATION_PARSER_HPP
#define CONFIGURATION_PARSER_HPP

#include "irods_exception.hpp"
#include "irods_error.hpp"
#include "rodsErrorTable.h"

#include <vector>
#include <string>

#include <unordered_map>
#include <boost/format.hpp>
#include <boost/any.hpp>
#include <boost/optional.hpp>

#include "json.hpp"

namespace irods {

    class configuration_parser {
        public:
            typedef std::vector< std::string > key_path_t;

            configuration_parser();
            ~configuration_parser();
            configuration_parser( const configuration_parser& );
            configuration_parser( const std::string& );
            configuration_parser& operator=( const configuration_parser& );
            void clear();
            error load( const std::string& );
            error write( const std::string& );
            error write( );

            bool has_entry(
                const std::string& ); // key

            template< typename T >
            T& set(
                const std::string& _key,
                const T&           _val ) {
                root_[_key] = boost::any(_val);
                return boost::any_cast<T&>(root_[_key]);

            } // set

            template< typename T >
            T& set(
                const key_path_t& _keys,
                const T&          _val ) {

                if ( _keys.empty() ) {
                    THROW( -1, "\"set\" requires at least one key");
                }
                boost::optional<boost::any&> cur_val;
                for ( const auto& key : _keys ) {
                    if ( !cur_val ) {
                        cur_val.reset(root_[key]);
                        continue;
                    }

                    if ( cur_val->empty()) {
                        *cur_val = std::unordered_map<std::string, boost::any>();
                    }
                    try {
                        cur_val.reset(boost::any_cast<std::unordered_map<std::string, boost::any>&>(*cur_val)[key]);
                    } catch ( const boost::bad_any_cast& ) {
                        THROW( INVALID_ANY_CAST, "value was not a map");
                    }
                }
                *cur_val = _val;
                return boost::any_cast<T&>(*cur_val);

            } // set with path

            template< typename T >
            T& get( const std::string& _key ) {
                try {
                    return boost::any_cast<T&>(root_.at(_key));
                } catch ( const boost::bad_any_cast& ) {
                    THROW( INVALID_ANY_CAST, (boost::format("value at %s was incorrect type") % _key).str());
                } catch ( const std::out_of_range& ) {
                    THROW ( KEY_NOT_FOUND, (boost::format("key \"%s\" not found in map.") % _key).str() );
                }

            } // get

            template< typename T >
            T& get(const key_path_t& _keys) {

                if ( _keys.empty() ) {
                    THROW( -1, "\"get\" requires at least one key");
                }
                boost::optional<boost::any&> cur_val;
                for ( const auto& key : _keys ) {
                    try {
                        if ( !cur_val ) {
                            cur_val.reset(root_.at(key));
                        } else {
                            try {
                                cur_val.reset(boost::any_cast<std::unordered_map<std::string, boost::any>&>(*cur_val).at(key));
                            } catch ( const boost::bad_any_cast& ) {
                                THROW( INVALID_ANY_CAST, "value was not a map");
                            }
                        }
                    } catch ( const std::out_of_range& ) {
                        THROW ( KEY_NOT_FOUND, (boost::format("key \"%s\" not found in map.") % key).str() );
                    }
                }
                try {
                    return boost::any_cast<T&>(*cur_val);
                } catch ( const boost::bad_any_cast& ) {
                    THROW(INVALID_ANY_CAST, "value was incorrect type");
                }

            } // get with path


            template< typename T >
            T remove(const std::string& _key) {
                auto find_it = root_.find(_key);
                if ( find_it == root_.end() ) {
                    THROW ( KEY_NOT_FOUND, (boost::format("key \"%s\" not found in map.") % _key).str() );
                }
                T val = find_it->second;
                root_.erase(find_it);
                return val;
            }

            void remove(const std::string& _key);

            std::unordered_map<std::string, boost::any>& map() {
                return root_;
            }

        private:
            error load_json_object(
                const std::string& ); // file name

            boost::any convert_json(const nlohmann::json&);

            error copy_and_swap(
                const std::unordered_map<std::string, boost::any>& );    // object to swap in

            std::string file_name_;   // full path to file
            std::unordered_map<std::string, boost::any> root_;        // root config object

    }; // class configuration_parser

    std::string to_env( const std::string& );

}; // namespace irods

#endif // CONFIGURATION_PARSER_HPP
