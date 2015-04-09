#ifndef __IRODS_KVP_STRING_PARSER_HPP__
#define __IRODS_KVP_STRING_PARSER_HPP__

// =-=-=-=-=-=-=-
#include "irods_error.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <string>

// =-=-=-=-=-=-=-
// boost includes
#include <iostream>
#include <string>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <map>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

namespace irods {
/// =-=-=-=-=-=-=-
/// @brief default delimiter and key-value association
    static const std::string KVP_DEF_DELIMITER( ";" );
    static const std::string KVP_DEF_ASSOCIATION( "=" );
    static const std::string KVP_DEF_ESCAPE( "\\" );

/// =-=-=-=-=-=-=-
/// @brief typedef of key-value map
    typedef std::map< std::string, std::string > kvp_map_t;

/// =-=-=-=-=-=-=-
/// @brief function to return defined delimiter token
    std::string kvp_delimiter();

/// =-=-=-=-=-=-=-
/// @brief function to return defined association token
    std::string kvp_association();

/// =-=-=-=-=-=-=-
/// @brief function to return defined escape token
    std::string kvp_escape();

/// =-=-=-=-=-=-=-
/// @brief given a string, break the string along the delimiter and then
///        break the tokens along the assignment for key-value pairs
    error parse_kvp_string(
        const std::string& _str,                                // string to be parsed
        kvp_map_t&         _kvp,                                // map of kvp
        const std::string& _association = KVP_DEF_ASSOCIATION,  // association token, defaults
        const std::string& _delimeter = KVP_DEF_DELIMITER );    // delimiter token, defaults

/// =-=-=-=-=-=-=-
/// @brief given a string, break the escaped string along the delimiter and then
///        break the tokens along the assignment for key-value pairs
    error parse_escaped_kvp_string(
        const std::string& _str,                                // string to be parsed
        kvp_map_t&         _kvp,                                // map of kvp
        const std::string& _association = KVP_DEF_ASSOCIATION,  // association token, defaults
        const std::string& _delimeter = KVP_DEF_DELIMITER,      // delimiter token, defaults
        const std::string& _escape = KVP_DEF_ESCAPE );          // escape token, defaults

/// @brief Given a kvp map, generate a properly delimited string.
    std::string kvp_string(
        const kvp_map_t& _kvp );    // The map from which to generate the string

/// @brief Given a kvp map, generate a properly delimited, escaped string.
    std::string escaped_kvp_string(
        const kvp_map_t& _kvp );    // The map from which to generate the string

}; // namespace irods


#endif // __IRODS_KVP_STRING_PARSER_HPP__
