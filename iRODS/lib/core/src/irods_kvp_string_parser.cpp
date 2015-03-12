#include "irods_log.hpp"

// =-=-=-=-=-=-=-
#include "irods_kvp_string_parser.hpp"
#include "irods_stacktrace.hpp"

// =-=-=-=-=-=-=-
// rods includes
#include "rodsErrorTable.hpp"

namespace irods {
/// =-=-=-=-=-=-=-
/// @brief function to return defined delimiter
    std::string kvp_delimiter() {
        return KVP_DEF_DELIM;
    }

/// =-=-=-=-=-=-=-
/// @brief function to return defined delimiter
    std::string kvp_association() {
        return KVP_DEF_ASSOC;
    }

/// =-=-=-=-=-=-=-
/// @brief given a string, break the string along the kvp association and then
///        place the pair into the map
    static
    error parse_token_into_kvp(
        const std::string& _token,
        kvp_map_t&         _kvp,
        const std::string& _assoc ) {
        // =-=-=-=-=-=-=-
        // split along the associative delimiter and place into the map
        std::vector< std::string > token_vec;
        try {
            boost::split( token_vec, _token, boost::is_any_of( _assoc ), boost::token_compress_on );
        }
        catch ( const boost::bad_function_call& ) {
            rodsLog( LOG_ERROR, "boost::split threw boost::bad_function_call" );
            token_vec.clear();
        }
        if ( token_vec.size() != 2 ) {
            std::stringstream msg;
            msg << "token vector size != 2 during parsing of ["
                << _token
                << "]";
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       msg.str() );
        }

        _kvp[ token_vec[0] ] = token_vec[1];

        return SUCCESS();

    } // parse_token_into_kvp

/// =-=-=-=-=-=-=-
/// @brief given a string, break the string along the delimiter and then
///        break the tokens along the assignment for key-value pairs
    error parse_kvp_string(
        const std::string& _string,
        kvp_map_t&         _kvp,
        const std::string& _assoc,
        const std::string& _delim ) {
        // =-=-=-=-=-=-=-
        // test for the delim first, if there is none then
        // short circuit, test for association and place in map
        size_t pos = _string.find( _delim );
        if ( std::string::npos == pos ) {
            // =-=-=-=-=-=-=-
            // no delim, look for association
            pos = _string.find( kvp_association() );
            if ( std::string::npos == pos ) {
                // =-=-=-=-=-=-=-
                // no association, just add to the map
                rodsLog(
                    LOG_DEBUG,
                    "parse_kvp_string :: no kvp found [%s]",
                    _string.c_str() );
                return ERROR( -1, "" );

            }
            else {
                // =-=-=-=-=-=-=-
                // association found, break it into a kvp
                // and place it in the map
                return parse_token_into_kvp(
                           _string,
                           _kvp,
                           _assoc );
            }

        } // if no delim found

        // =-=-=-=-=-=-=-
        // otherwise parse the string into tokens split by the delimiter
        std::list< std::string > token_list;;
        try {
            boost::split( token_list, _string, boost::is_any_of( KVP_DEF_DELIM ), boost::token_compress_on );
        }
        catch ( const boost::bad_function_call& ) {
            rodsLog( LOG_ERROR, "boost::split threw boost::bad_function_call" );
            token_list.clear();
        }
        BOOST_FOREACH( std::string & token, token_list ) {
            // =-=-=-=-=-=-=-
            // now that the string is broken into tokens we need to
            // extract the key and value to put them into the map
            error ret = parse_token_into_kvp( token, _kvp, _assoc );
        }

        return SUCCESS();

    } // parse_kvp_string


    error kvp_string(
        const kvp_map_t& _kvp,
        std::string& _rtn_str ) {
        error result = SUCCESS();
        std::string str;
        bool first = true;
        for ( kvp_map_t::const_iterator it = _kvp.begin(); result.ok() && it != _kvp.end(); ++it ) {
            if ( !first ) {
                str.append( kvp_delimiter() );
            }
            else {
                first = false;
            }
            str.append( it->first );
            str.append( kvp_association() );
            str.append( it->second );
        }
        _rtn_str = str;
        return result;
    }
}; // namespace irods



