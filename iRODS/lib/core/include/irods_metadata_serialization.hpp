#include <string>
#include <vector>

namespace irods {
    std::string serialize_metadata( const std::vector<std::string>& metadata );
    std::vector<std::string> deserialize_metadata( const std::string& metadata );
}

extern "C" {
    char* serialize_metadata_c( const char** metadata, size_t metadata_len );
}
