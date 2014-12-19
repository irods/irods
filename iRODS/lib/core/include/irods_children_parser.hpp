#ifndef _CHILDREN_PARSER_HPP_
#define _CHILDREN_PARSER_HPP_

#include "irods_error.hpp"

#include <map>
#include <utility>
#include <string>

namespace irods {

    /**
     * @brief Class for managing the children string of a resource
     */
    class children_parser {
        public:
            typedef std::map<std::string, std::string> children_map_t;
            typedef children_map_t::const_iterator const_iterator;
            /**
             * @brief Constructor
             */
            children_parser( void );
            virtual ~children_parser( void );


            /// @brief Returns the list of children and context strings
            error list( children_map_t& list );

            /// @brief Returns the encoded children string
            error str( std::string& ret_string ) const;

            /// @brief Adds the specified child and context string to the children list
            error add_child( const std::string& child, const std::string& context );

            /// @brief Removes the specified child from the list
            error remove_child( const std::string& child );

            /// @brief Sets the children string to parse
            error set_string( const std::string& str );

            /// @brief Returns the name of the first child in the list
            error first_child( std::string& _child );

            /// @brief Returns the name of the last child in the list
            error last_child( std::string& _child );

            /// @brief Returns an iterator to the beginning of the list
            const_iterator begin( void ) const;

            /// @brief Returns an iterator to the end of the list
            const_iterator end( void ) const;

        private:
            //std::string children_string_;
            children_map_t children_list_;
    };
}; // namespace irods

#endif // _CHILDREN_PARSER_HPP_
