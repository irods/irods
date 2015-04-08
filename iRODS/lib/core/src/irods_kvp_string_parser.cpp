#include "irods_log.hpp"

// =-=-=-=-=-=-=-
#include "irods_kvp_string_parser.hpp"
#include "irods_stacktrace.hpp"

// =-=-=-=-=-=-=-
// rods includes
#include "rodsErrorTable.hpp"
#include <sstream>
#include <set>
#include <boost/algorithm/string/predicate.hpp>

namespace irods {
/// =-=-=-=-=-=-=-
/// @brief function to return defined delimiter
    std::string kvp_delimiter() {
        return KVP_DEF_DELIMITER;
    }

/// =-=-=-=-=-=-=-
/// @brief function to return defined delimiter
    std::string kvp_association() {
        return KVP_DEF_ASSOCIATION;
    }

/// =-=-=-=-=-=-=-
/// @brief function to return defined delimiter
    std::string kvp_escape() {
        return KVP_DEF_ESCAPE;
    }

/// =-=-=-=-=-=-=-
/// @brief given a string, break the string along the delimiter and then
///        break the tokens along the assignment for key-value pairs
    error parse_kvp_string(
        const std::string& _string,
        kvp_map_t&         _kvp,
        const std::string& _assoc,
        const std::string& _delim,
        const std::string& _escape ) {
        if ( _delim.empty() || _assoc.empty() || _escape.empty() ) {
            return ERROR( SYS_BAD_INPUT, "_delim, _assoc, and _escape may not be empty" );
        }
        if ( _delim == _escape || _delim == _assoc || _assoc == _escape ||
                boost::starts_with( _assoc, _delim ) || boost::starts_with( _delim, _assoc ) ) {
            return ERROR( SYS_BAD_INPUT, "_delim, _assoc, and _escape must all be distinct in calls to parse_kvp_string" );
        }
        if ( _assoc.find( _delim ) != std::string::npos || _assoc.find( _escape ) != std::string::npos ||
                _delim.find( _assoc ) != std::string::npos || _delim.find( _escape ) != std::string::npos ||
                _escape.find( _assoc ) != std::string::npos || _escape.find( _delim ) != std::string::npos ) {
            return ERROR( SYS_BAD_INPUT, "_delim, _assoc, and _escape may not contain each other in calls to parse_kvp_string" );
        }
        std::stringstream key;
        std::stringstream value;
        bool assoc_encountered = false;
        for ( size_t i = 0; i < _string.size(); ) {
            if ( boost::starts_with( _string.substr( i ), _escape ) ) {
                i += _escape.size();
                if ( i >= _string.size() ) {
                    return ERROR( SYS_BAD_INPUT, "kvp ended in an unescaped _escape token" );
                }
            }
            else if ( boost::starts_with( _string.substr( i ), _assoc ) ) {
                if ( assoc_encountered ) {
                    return ERROR( SYS_BAD_INPUT, "unescaped _assoc token encountered in key or value" );
                }
                assoc_encountered = true;
                i += _assoc.size();
                continue;
            }
            else if ( boost::starts_with( _string.substr( i ), _delim ) ) {
                if ( !assoc_encountered ) {
                    return ERROR( SYS_BAD_INPUT,
                            "no unescaped _assoc token encountered before unescaped delimiter when parsing kvp" );
                }
                _kvp[ key.str() ] = value.str();
                assoc_encountered = false;
                key.clear();
                value.clear();
                i += _delim.size();
                continue;
            }

            ( assoc_encountered ? value : key ) << _string[i];
            ++i;
        }

        if ( !key.str().empty() && !assoc_encountered ) {
            return ERROR( SYS_BAD_INPUT,
                    "no unescaped _assoc token encountered before end of string when parsing kvp" );
        }
        else if ( assoc_encountered ) {
            _kvp[ key.str() ] = value.str();
        }
        return SUCCESS();
    }

    std::string escape_string( const std::string& _string,
            const std::string& _escape_token,
            const std::set<std::string>& _special_tokens ) {
        std::stringstream escaped_str;
        for ( size_t i = 0; i < _string.size(); ) {
            std::set<std::string>::const_iterator tok;
            for ( tok = _special_tokens.begin(); tok != _special_tokens.end(); ++tok ) {
                if ( boost::starts_with( _string.substr( i ), *tok ) ) {
                    escaped_str << _escape_token << *tok;
                    i += tok->size();
                    break;
                }
            }
            if ( tok != _special_tokens.end() ) {
                continue;
            }
            else if ( _special_tokens.count( _escape_token ) == 0 &&
                    boost::starts_with( _string.substr( i ), _escape_token ) ) {
                escaped_str << _escape_token << _escape_token;
                i += _escape_token.size();
                continue;
            }
            escaped_str << _string[i];
        }
        return escaped_str.str();
    }


    std::string kvp_string(
            const kvp_map_t& _kvp ) {
        std::stringstream str;
        std::set<std::string> special_tokens;
        special_tokens.insert( kvp_delimiter() );
        special_tokens.insert( kvp_association() );
        bool first = true;
        for ( kvp_map_t::const_iterator it = _kvp.begin(); it != _kvp.end(); ++it ) {
            first ?
                first = false :
                str << kvp_delimiter();
            str << escape_string( it->first, kvp_escape(), special_tokens );
            str << kvp_association();
            str << escape_string( it->second, kvp_escape(), special_tokens );
        }
        return str.str();
    }
}; // namespace irods
