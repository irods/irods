#include <string>
#include <sstream>
#include <vector>
#include <boost/algorithm/string/replace.hpp>

namespace irods {
    std::string serialize_metadata( const std::vector<std::string>& metadata ) {
        std::stringstream serialized_metadata;
        for ( std::vector<std::string>::const_iterator iter = metadata.begin(); iter != metadata.end(); ) {
            serialized_metadata << boost::replace_all_copy( boost::replace_all_copy( *iter, "\\", "\\\\" ), ";", "\\;" );
            ++iter;
            if ( iter != metadata.end() ) {
                serialized_metadata << ";";
            }
        }
        return serialized_metadata.str();
    }

    std::vector<std::string> deserialize_metadata( const std::string& metadata ) {
        std::vector<std::string> deserialized_metadata;
        std::stringstream current_string;
        for ( std::string::const_iterator iter = metadata.begin(); iter != metadata.end(); ++iter ) {
            if ( *iter == '\\' ) {
                ++iter;
            }
            else if ( *iter == ';' ) {
                deserialized_metadata.push_back( current_string.str() );
                current_string.str( "" );
                continue;
            }
            current_string << *iter;
        }
        return deserialized_metadata;
    }
}

extern "C" {
    char* serialize_metadata_c( const char** metadata, size_t metadata_len ) {
        std::vector<std::string> metadata_strings;
        for ( int i = 0; i < metadata_len; i++ ) {
            metadata_strings.push_back( metadata[i] );
        }
        return strdup( irods::serialize_metadata( metadata_strings ).c_str() );
    }
}
