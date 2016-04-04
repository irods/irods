#ifndef _HIERARCHY_PARSER_HPP_
#define _HIERARCHY_PARSER_HPP_

#include "irods_error.hpp"

#include <vector>
#include <string>

namespace irods {

    /**
     * @brief Class to manage resource hierarchy strings
     */
    class hierarchy_parser {
        public:
            typedef std::vector<std::string> resc_list_t;
            typedef resc_list_t::const_iterator const_iterator;

            /// @brief ctor doesn't do much, until it has a string
            hierarchy_parser( void );

            /// @brief copy constructor
            hierarchy_parser( const hierarchy_parser& parser );

            virtual ~hierarchy_parser();

            /// @brief assignment operator
            hierarchy_parser& operator=( const hierarchy_parser& parser );

            /// @brief Parses the specified hierarchy string
            error set_string( const std::string& _resc_hier );

            /** @brief Returns the resource hierarchy string.
             * This method returns a properly formatted hierarchy string, terminating
             * the string at the _term_resc resource if non-empty.
             */
            error str( std::string& _ret_string, const std::string& _term_resc = std::string( "" ) ) const;

            /// @brief Adds another level of hierarchy by adding the specified child resource
            error add_child( const std::string& _resc );

            /// @brief Returns the first resource
            error first_resc( std::string& _ret_resc ) const;

            /// @brief Returns the last resource in the hierarchy
            error last_resc( std::string& _ret_resc ) const;

            /// @brief Returns the next resource in the hierarchy after the specified resource
            error next( const std::string& _current, std::string& _ret_resc ) const;

            /// @brief Returns the number of levels in the resource hierarchy
            error num_levels( int& levels ) const;

            /// @brief Returns an iterator to the beginning of the list
            const_iterator begin( void ) const;

            /// @brief Returns an iterator to the end of the list
            const_iterator end( void ) const;

            /// @brief Returns the delimiter used for hierarchy strings.
            static const std::string& delimiter( void );

            /// @brief Returns true if the specified resource is in the hierarchy.
            bool resc_in_hier( const std::string& _resc ) const;

        private:
            resc_list_t resc_list_;
    };
}; // namespace irods

#endif // _HIERARCHY_PARSER_HPP_
