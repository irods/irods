#ifndef CONFIGURATION_PARSER_HPP
#define CONFIGURATION_PARSER_HPP

#include "irods_exception.hpp"
#include "irods_error.hpp"
#include "rodsErrorTable.h"

#include <vector>
#include <string>
#include "jansson.h"

#include <map>
#include <boost/format.hpp>
#include <boost/any.hpp>

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

            size_t erase(
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
                const key_path_t& _key,
                const T&          _val ) {

                if ( _key.empty() ) {
                    THROW( -1, "\"set\" requires at least one key");
                }
                key_path_t::iterator it = _key.begin();
                std::map<std::string, boost::any> * cur_map = &root_;
                while ( true ) {
                    std::string& cur_key = *it;
                    ++it;
                    if ( it == _key.end() ) {
                        (*cur_map)[cur_key] = _val;
                        return boost::any_cast<T&>((*cur_map)[cur_key]);
                    }
                    else {
                        std::map<std::string, boost::any>::iterator find_it = cur_map->find(cur_key);
                        if ( find_it == cur_map->end() ) {
                            (*cur_map)[cur_key] = boost::any(std::map<std::string, boost::any>());
                            find_it = cur_map->find(cur_key);
                        }
                        try {
                            cur_map = &boost::any_cast<std::map<std::string, boost::any>&>(find_it->second);
                        } catch ( const boost::bad_any_cast& ) {
                            THROW( INVALID_ANY_CAST, (boost::format("value at %s was not a map") % cur_key).str());
                        }
                    }
                }

            } // set with path

            template< typename T >
            T& get( const std::string& _key ) {
                std::map<std::string, boost::any>::iterator find_it = root_.find(_key);
                if ( find_it == root_.end() ) {
                    THROW ( KEY_NOT_FOUND, (boost::format("key \"%s\" not found in map.") % _key).str() );
                }
                try {
                    return boost::any_cast<T&>(find_it->second);
                } catch ( const boost::bad_any_cast& ) {
                    THROW( INVALID_ANY_CAST, (boost::format("value at %s was incorrect type") % _key).str());
                }

            } // get

            template< typename T >
            T& get(const key_path_t& _key) {

                if ( _key.empty() ) {
                    THROW( -1, "\"get\" requires at least one key");
                }
                key_path_t::iterator it = _key.begin();
                std::map<std::string, boost::any> * cur_map = &root_;
                while ( true ) {
                    std::string& cur_key = *it;
                    ++it;
                    if ( it == _key.end() ) {
                        std::map<std::string, boost::any>::iterator find_it = cur_map->find(cur_key);
                        if ( find_it == root_.end() ) {
                            THROW ( KEY_NOT_FOUND, (boost::format("key \"%s\" not found in map.") % cur_key).str() );
                        }
                        try {
                            return boost::any_cast<T&>(find_it->second);
                        } catch ( const boost::bad_any_cast& ) {
                            THROW( INVALID_ANY_CAST, (boost::format("value at %s was incorrect type") % cur_key).str());
                        }
                    }
                    else {
                        std::map<std::string, boost::any>::iterator find_it = cur_map->find(cur_key);
                        if ( find_it == cur_map->end() ) {
                            THROW( KEY_NOT_FOUND, (boost::format("%s not found in map for \"get\"") % cur_key).str());
                        }
                        try {
                            cur_map = &boost::any_cast<std::map<std::string, boost::any>&>(find_it->second);
                        } catch ( const boost::bad_any_cast& ) {
                            THROW( INVALID_ANY_CAST, (boost::format("value at %s was not a map") % cur_key).str());
                        }
                    }
                }

            } // get with path

        private:
            error load_json_object(
                const std::string& ); // file name

            boost::any convert_json(
                json_t* );            // jansson object

            error copy_and_swap(
                const std::map<std::string, boost::any>& );    // object to swap in

            std::string file_name_;   // full path to file
            std::map<std::string, boost::any> root_;        // root config object

    }; // class configuraiton_parser

    std::string to_env( const std::string& );

}; // namespace irods

#endif // CONFIGURATION_PARSER_HPP
