#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <boost/regex.hpp>
#include "irods_serialization.hpp"
#include "irods_exception.hpp"
#include "rodsErrorTable.h"

namespace irods {
    static const char default_escape_char = '\\';
    static const char default_delimiter_char = ';';
    static const std::string default_special_characters = std::string( 1, default_escape_char ) + default_delimiter_char;

    std::string get_format_string_for_escape(
        const char escape_char ) {
        std::stringstream str;
        switch ( escape_char ) {
        case '\0':
            break;
        case '$':
        case '(':
        case ')':
        case '?':
        case ':':
        case '\\':
            str << '\\' << escape_char;
            break;
        default:
            str << escape_char;
        }
        str << "$&";
        return str.str();
    }

    boost::regex character_set_regex(
        const std::set<char>& character_set ) {
        std::stringstream str;
        str << '[';
        for ( std::set<char>::const_iterator iter = character_set.begin(); iter != character_set.end(); ++iter ) {
            switch ( *iter ) {
            case ']':
            case '^':
            case '-':
            case '\\':
                str << '\\' << *iter;
                break;
            default:
                str << *iter;
            }
        }
        str << ']';
        return boost::regex( str.str() );
    }

    boost::regex character_set_regex(
        const std::string& character_set ) {
        std::set<char> set;
        for ( std::string::const_iterator iter = character_set.begin(); iter != character_set.end(); iter++ ) {
            set.insert( *iter );
        }
        return character_set_regex( set );
    }

    std::vector<std::string> escape_strings(
        const std::vector<std::string>& strs,
        const boost::regex& special_character_set_regex,
        const char escape_char ) {
        std::vector<std::string> escaped_strs;
        if ( escape_char ) {
            if ( !boost::regex_match( std::string( 1, escape_char ), special_character_set_regex ) ) {
                THROW( SYS_BAD_INPUT, "Regular expression passed to escape_string must match against the escape character." );
            }
            for ( std::vector<std::string>::const_iterator iter = strs.begin(); iter != strs.end(); ++iter ) {
                escaped_strs.push_back( boost::regex_replace( *iter, special_character_set_regex, get_format_string_for_escape( escape_char ) ) );
            }
        }
        else {
            for ( std::vector<std::string>::const_iterator iter = strs.begin(); iter != strs.end(); ++iter ) {
                escaped_strs.push_back( *iter );
            }
        }
        return escaped_strs;
    }

    std::vector<std::string> escape_strings(
        const std::vector<std::string>& strs,
        const std::set<char>& special_character_set,
        const char escape_char ) {
        return escape_strings( strs, character_set_regex( special_character_set ), escape_char );
    }

    std::vector<std::string> escape_strings(
        const std::vector<std::string>& strs,
        const std::string& special_character_set,
        const char escape_char ) {
        return escape_strings( strs, character_set_regex( special_character_set ), escape_char );
    }

    std::vector<std::string> escape_strings(
        const std::vector<std::string>& strs,
        const boost::regex& special_character_set_regex ) {
        return escape_strings( strs, special_character_set_regex, default_escape_char );
    }

    std::vector<std::string> escape_strings(
        const std::vector<std::string>& strs,
        const std::set<char>& special_character_set ) {
        return escape_strings( strs, character_set_regex( special_character_set ) );
    }

    std::vector<std::string> escape_strings(
        const std::vector<std::string>& strs,
        const std::string& special_character_set ) {
        return escape_strings( strs, character_set_regex( special_character_set ) );
    }

    std::vector<std::string> escape_strings(
        const std::vector<std::string>& strs ) {
        return escape_strings( strs, default_special_characters );
    }

    std::string escape_string(
        const std::string& str,
        const boost::regex& special_character_set_regex,
        const char escape_char ) {
        std::vector<std::string> v;
        v.push_back( str );
        return escape_strings( v, special_character_set_regex, escape_char )[0];
    }

    std::string escape_string(
        const std::string& str,
        const std::set<char>& special_character_set,
        const char escape_char ) {
        return escape_string( str, character_set_regex( special_character_set ), escape_char );
    }

    std::string escape_string(
        const std::string& str,
        const std::string& special_character_set,
        const char escape_char ) {
        return escape_string( str, character_set_regex( special_character_set ), escape_char );
    }

    std::string escape_string(
        const std::string& str,
        const boost::regex& special_character_set_regex ) {
        return escape_string( str, special_character_set_regex, default_escape_char );
    }

    std::string escape_string(
        const std::string& str,
        const std::set<char>& special_character_set ) {
        return escape_string( str, character_set_regex( special_character_set ) );
    }

    std::string escape_string(
        const std::string& str,
        const std::string& special_character_set ) {
        return escape_string( str, character_set_regex( special_character_set ) );
    }

    std::string escape_string(
        const std::string& str ) {
        return escape_string( str, default_special_characters );
    }

    std::string join( std::vector<std::string>& strs, const std::string& separator ) {
        std::stringstream joined;
        for ( std::vector<std::string>::const_iterator iter = strs.begin(); iter != strs.end(); ) {
            joined << *iter;
            ++iter;
            if ( iter != strs.end() ) {
                joined << separator;
            }
        }
        return joined.str();
    }

    std::string join( std::vector<std::string>& strs, const char separator ) {
        return join( strs, std::string( 1, separator ) );
    }

    std::string serialize_list(
        const std::vector<std::string>& list,
        const boost::regex& special_character_set_regex,
        const char delimiter_char,
        const char escape_char ) {
        if ( escape_char && !boost::regex_match( std::string( 1, delimiter_char ), special_character_set_regex ) ) {
            THROW( SYS_BAD_INPUT, "Regular expression passed to serialize_list must match against the delimiter character." );
        }
        std::vector<std::string> escaped_strings = escape_strings( list, special_character_set_regex, escape_char );
        return join( escaped_strings, delimiter_char );
    }

    std::string serialize_list(
        const std::vector<std::string>& list,
        const std::set<char>& special_character_set,
        const char delimiter_char,
        const char escape_char ) {
        return serialize_list( list, character_set_regex( special_character_set ), delimiter_char, escape_char );
    }

    std::string serialize_list(
        const std::vector<std::string>& list,
        const std::string& special_character_set,
        const char delimiter_char,
        const char escape_char ) {
        return serialize_list( list, character_set_regex( special_character_set ), delimiter_char, escape_char );
    }

    std::string serialize_list(
        const std::vector<std::string>& list,
        const boost::regex& special_character_set_regex,
        const char delimiter_char ) {
        return serialize_list( list, special_character_set_regex, delimiter_char, default_escape_char );
    }

    std::string serialize_list(
        const std::vector<std::string>& list,
        const std::set<char>& special_character_set,
        const char delimiter_char ) {
        return serialize_list( list, character_set_regex( special_character_set ), delimiter_char );
    }

    std::string serialize_list(
        const std::vector<std::string>& list,
        const std::string& special_character_set,
        const char delimiter_char ) {
        return serialize_list( list, character_set_regex( special_character_set ), delimiter_char );
    }

    std::string serialize_list(
        const std::vector<std::string>& list,
        const boost::regex& special_character_set_regex ) {
        return serialize_list( list, special_character_set_regex, default_delimiter_char );
    }

    std::string serialize_list(
        const std::vector<std::string>& list,
        const std::set<char>& special_character_set ) {
        return serialize_list( list, character_set_regex( special_character_set ) );
    }

    std::string serialize_list(
        const std::vector<std::string>& list,
        const std::string& special_character_set ) {
        return serialize_list( list, character_set_regex( special_character_set ) );
    }

    std::string serialize_list(
        const std::vector<std::string>& list ) {
        return serialize_list( list, default_special_characters );
    }

    std::vector<std::string> deserialize_list(
        const std::string& list,
        const std::string& delimiters,
        const char escape_char ) {
        std::vector<std::string> deserialized_list;
        std::stringstream current_string;
        const char delimiter_char = delimiters[0];
        const std::string remaining_delimiters = delimiters.size() ? delimiters.substr( 1 ) : "" ;
        for ( std::string::const_iterator iter = list.begin(); iter != list.end(); ++iter ) {
            if ( *iter == escape_char ) {
                ++iter;
                if ( iter == list.end() ) {
                    break;
                }
                if ( remaining_delimiters.size() && ( *iter == escape_char || remaining_delimiters.find( *iter ) != std::string::npos ) ) {
                    current_string << escape_char;
                }
            }
            else if ( *iter == delimiter_char ) {
                deserialized_list.push_back( current_string.str() );
                current_string.str( "" );
                continue;
            }
            current_string << *iter;
        }
        if ( !current_string.str().empty() ) {
            deserialized_list.push_back( current_string.str() );
        }

        return deserialized_list;
    }

    std::vector<std::string> deserialize_list(
        const std::string& list,
        const char delimiter_char,
        const char escape_char ) {
        return deserialize_list( list, std::string( 1, delimiter_char ), escape_char );
    }

    std::vector<std::string> deserialize_list(
        const std::string& list,
        const std::string& delimiters ) {
        return deserialize_list( list, delimiters, '\\' );
    }

    std::vector<std::string> deserialize_list(
        const std::string& list,
        const char delimiter_char ) {
        return deserialize_list( list, delimiter_char, '\\' );
    }

    std::vector<std::string> deserialize_list(
        const std::string& list ) {
        return deserialize_list( list, ";", '\\' );
    }

    std::vector<std::string> deserialize_metadata( const std::string& metadata ) {
        std::vector<std::string> deserialized_metadata = deserialize_list( metadata );
        if ( deserialized_metadata.size() % 3 == 2 ) {
            deserialized_metadata.push_back( "" );
        }
        else if ( deserialized_metadata.size() % 3 != 0 ) {
            THROW( SYS_BAD_INPUT, "Metadata strings must consist of triplets of semicolon-separated tokens" );
        }

        return deserialized_metadata;
    }

    std::string serialize_metadata( const std::vector<std::string>& metadata ) {
        if ( metadata.size() % 3 != 0 ) {
            THROW( SYS_BAD_INPUT, "Metadata must exist in triplets" );
        }
        return serialize_list( metadata );
    }

    std::vector<std::vector<std::string> > deserialize_acl( const std::string& acl ) {
        std::vector<std::string> shallow_deserialized_acl = deserialize_list( acl, "; ", '\\' );
        std::vector<std::vector<std::string> > deserialized_acl;
        for ( std::vector<std::string>::const_iterator iter = shallow_deserialized_acl.begin(); iter != shallow_deserialized_acl.end(); ++iter ) {
            std::vector<std::string> current_acl = deserialize_list( *iter, " ", '\\' );
            if ( current_acl.size() != 2 ) {
                THROW( SYS_BAD_INPUT, "ACLs must be a space-separated tuple of \"user permission\"" );
            }
            deserialized_acl.push_back( current_acl );
        }

        return deserialized_acl;
    }

    std::string serialize_acl( const std::vector<std::vector<std::string> >& acl ) {
        std::vector<std::string> shallow_serialized_acl;
        for ( std::vector<std::vector<std::string> >::const_iterator iter = acl.begin(); iter != acl.end(); ++iter ) {
            if ( iter->size() != 2 ) {
                THROW( SYS_BAD_INPUT, "ACLs must be a tuple of user and permission" );
            }
            shallow_serialized_acl.push_back( serialize_list( *iter, boost::regex( "[\\\\ ;]" ), ' ', '\\' ) );
        }
        return join( shallow_serialized_acl, ';' );
    }
}

extern "C" {
    char* serialize_list_c( const char** list, size_t list_len ) {
        std::vector<std::string> list_strings;
        for ( size_t i = 0; i < list_len; i++ ) {
            list_strings.push_back( list[i] );
        }
        return strdup( irods::serialize_list( list_strings ).c_str() );
    }

    char* serialize_metadata_c( const char** metadata, size_t metadata_len ) {
        std::vector<std::string> metadata_strings;
        for ( size_t i = 0; i < metadata_len; i++ ) {
            metadata_strings.push_back( metadata[i] );
        }
        try {
            return strdup( irods::serialize_metadata( metadata_strings ).c_str() );
        }
        catch ( const irods::exception& ) {
            return NULL;
        }
    }

    char* serialize_acl_c( const char** acl, size_t acl_len ) {
        std::vector<std::vector<std::string> > acl_strings;
        for ( size_t i = 0; i < acl_len; i++ ) {
            if ( !( i & 1 ) ) {
                std::vector<std::string> v;
                v.push_back( acl[i] );
                acl_strings.push_back( v );
            }
            else {
                acl_strings.back().push_back( acl[i] );
            }
        }
        try {
            return strdup( irods::serialize_acl( acl_strings ).c_str() );
        }
        catch ( const irods::exception& ) {
            return NULL;
        }
    }
}
