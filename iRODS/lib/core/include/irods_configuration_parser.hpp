


#ifndef CONFIGURATION_PARSER_HPP
#define CONFIGURATION_PARSER_HPP

#include "irods_lookup_table.hpp"
#include <vector>
#include <string>
#include "jansson.h"

namespace irods {

    class configuration_parser {
        public:
            typedef lookup_table< boost::any > object_t;
            typedef std::vector< object_t >    array_t;
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
            error set(
                const std::string& _key,
                const T&           _val ) {
                irods::error ret = root_.set< T >(
                                       _key,
                                       _val );
                return ret;

            } // set

            template< typename T >
            error set(
                const key_path_t& _key,
                const T&          _val ) {

                return SUCCESS();

            } // set with path

            template< typename T >
            error get(
                const std::string& _key,
                T&                 _val ) {
                irods::error ret = root_.get< T >(
                                       _key,
                                       _val );
                return ret;

            } // get

            template< typename T >
            error get(
                const key_path_t& _key,
                T&                _val ) {

                return SUCCESS();

            } // get with path

        private:
            error load_json_object(
                const std::string& ); // file name

            error parse_json_object(
                json_t*,              // jansson object
                object_t& );          // parsing object

            error copy_and_swap(
                const object_t& );    // object to swap in

            std::string file_name_;   // full path to file
            object_t    root_;        // root config object

    }; // class configuraiton_parser

    std::string to_env( const std::string& );

}; // namespace irods

#endif // CONFIGURATION_PARSER_HPP



