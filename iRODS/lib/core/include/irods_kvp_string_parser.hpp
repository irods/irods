


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
    static const std::string KVP_DEF_DELIM(";");
    static const std::string KVP_DEF_ASSOC("=");

    /// =-=-=-=-=-=-=-
    /// @brief typeedef of key-value map
    typedef std::map< std::string, std::string > kvp_map_t;
    
    /// =-=-=-=-=-=-=-
    /// @brief function to return defined delimiter
    std::string kvp_delimiter();
 
    /// =-=-=-=-=-=-=-
    /// @brief function to return defined delimiter
    std::string kvp_association();

    /// =-=-=-=-=-=-=-
    /// @brief given a string, break the string along the delimiter and then
    ///        break the tokens along the assignment for key-value pairs
    error parse_kvp_string( 
        const std::string& _str,                    // string to be parsed
        kvp_map_t&         _kvp,                    // map of kvp
        const std::string& _assoc = KVP_DEF_ASSOC,  // associative token, defaults
        const std::string& _delim = KVP_DEF_DELIM); // delimiter, defaults
     
}; // namespace irods


#endif // __IRODS_KVP_STRING_PARSER_HPP__



