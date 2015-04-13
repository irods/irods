#ifndef IRODS_METADATA_SERIALIZATION_HPP__
#define IRODS_METADATA_SERIALIZATION_HPP__
#include <string>
#include <vector>

#ifdef __cplusplus
namespace irods {
    std::string serialize_metadata( const std::vector<std::string>& metadata );
    std::vector<std::string> deserialize_metadata( const std::string& metadata );
}
#endif

extern "C" {
    char* serialize_metadata_c( const char** metadata, size_t metadata_len );
}
#endif
